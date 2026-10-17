# 9front-gcc

GCC 16.2.0 and Binutils 2.47 for 9front amd64 and 386 (targets
`x86_64-pc-plan9`, `i686-pc-plan9`),
built on Linux as a Canadian cross against the gcc-built 9front tree
(`PLAN9`, default `../9front`). The result, `out/$objtype`, is the tree
`/$objtype/gnu` of an installed system; `../9front`'s `scripts/guest/mkiso.sh`
puts it on the install CD (`scripts/guest/boot/gnuproto`).

Log of the port (German): `NOTES.md`.

## Build

    ./fetch.sh            # sources to src/ (signatures: GNU's, keys/musl.pub; sha256.txt), patches applied
    ./build.sh cross      # Linux -> 9front: binutils, gcc (C, C++), libgcc, libstdc++ -> build/$objtype/cross
    ./build.sh hostlibs   # gmp, mpfr, mpc for 9front                                   -> build/$objtype/hostlibs
    ./build.sh native     # binutils and gcc (C, C++) that run on 9front                -> out/$objtype
    ./build.sh plan9      # gccmk and its wrappers (plan9/), flags from the tree's build -> out/$objtype
    ./build.sh compat     # only the toolchain's additions to APE (compat/, musl) into build/$objtype/cross

`objtype=386` before any of them for 386 (default amd64); build directories
are `build/$objtype`. Needs the 9front tree built for that architecture
(`CC=gcc ARCH=amd64 sys/src/cmd/build/build.exe`).

## Tests

    [objtype=386] tests/cross.sh        # cross-compiled C (APE, native Plan 9, compat: c99*.c, pthread.c) and C++ (cxx*.cc, eh.cc) run in the 9front guest
    [objtype=386] tests/native.sh       # /$objtype/gnu/bin/gcc and g++ compile and run them in the guest; same code as the cross compilers
    [objtype=386] tests/mk.sh           # gccmk in the guest: libbio, hoc (yacc), ape/cmd/expr (pcc), libc's assembly; same code as the tree's build

## Layout

- `patches/{gcc,binutils}.patch`: the target. `src/gcc` and `src/binutils` are
  git repositories with the tag `pristine` (the tarball); change them there and
  write the patches with `./patches.sh`.
- `plan9/`: `gccmk` and the wrappers it puts in front of mk (installed as
  `/$objtype/gnu/bin/gccmk` and `/$objtype/gnu/plan9`).
- `compat/include`, `compat/lib`: what APE lacks of C99/C11/POSIX.1-2008, as the
  toolchain's own headers (`$prefix/x86_64-pc-plan9/include`, searched before
  APE's, `#include_next`) and `libapecompat.a`; 9front's APE stays as it is.
  `libapecompat.a` also holds musl's libm, fenv, complex, wide string and
  `<wctype.h>` functions (`src/musl`, built by musl's own makefile; objects whose
  functions libap has are left out) and musl's printf/scanf engine, bridged to
  APE's FILE (`compat/musl/bridge.c`, `compat/lib/stdio.c`, `wstdio.c`).
- `keys/`: signing keys of non-GNU sources (musl), checked in (`sources.txt`).

## Using it on 9front

    bind -b /amd64/gnu/bin /bin
    gcc -o hello hello.c                       # POSIX over APE: -D_POSIX_SOURCE, -lapecompat -lbsd -lap
    gcc -nostdinc -I/amd64/include -I/sys/include -fplan9-extensions -fno-builtin -c prog.c
    gcc -nostdlib -o prog prog.o -L/amd64/lib.gcc -lc -lgcc     # native Plan 9 C
    g++ -o prog prog.cc                        # C++: libstdc++ over APE, exceptions

What the driver's POSIX mode adds to APE (compat): C99's `<math.h>` (float,
long double, `round`, `fma`, ...), `<fenv.h>`, `<complex.h>`, `<float.h>` with
gcc's values; C99's printf and scanf (`%Lf`, `%a`, `hh`/`j`/`z`/`t`, C99's
`snprintf` result), `strtof`, `strtold`; `<wchar.h>` and `<wctype.h>` with a 32-bit
`wchar_t` in UTF-8, wide I/O; `clock_gettime` (`CLOCK_MONOTONIC` from
`/dev/bintime`), `nanosleep`, `usleep`, `pread`; `<pthread.h>` (a thread is a
proc, `rfork(RFMEM)`). A program starts like on other POSIX systems: x87 and SSE
exceptions masked, x87 at 64-bit precision (9front's kernel traps on divide by
zero and rounds to 53 bits). APE's stdio is not thread-safe: threads that share
a FILE serialize themselves.

C++: libstdc++ over that, with threads (`std::thread`, `thread_local` by
emutls), wide streams, `<format>`, `steady_clock`; only the "C" locale. Static
constructors run from `__wrap_main`, and `crtbegin.o` registers the unwind tables
for exceptions.

C defaults to gnu17 (APE's headers are K&R in places). BSD functions and
sockets: `-D_BSD_EXTENSION`, as with pcc. gcc ignores `#pragma lib`: name the
libraries.

## mk with gcc: gccmk

    bind -b /amd64/gnu/bin /bin
    cd /sys/src/cmd/hoc && gccmk install        # 9front's mkfile, gcc instead of 6c/6a/6l

`gccmk` runs mk in a namespace where `/$objtype/mkfile` names the wrappers
`gcc-cc`, `gcc-as`, `gcc-ld` (and `pcc`, `ar` come before `/bin`), and
`/$objtype/lib` is `/$objtype/lib.gcc`. They compile, assemble and link as the
tree's build tool does, with its flags (`$PLAN9/$objtype/*.json`, written by
`build.sh plan9` into `/$objtype/gnu/plan9`): the same objects. Programs link
with every library of `/$objtype/lib.gcc` (gcc ignores `#pragma lib`); `ar`
names the members `x.o` like the build tool, so `gccmk install` in a library
replaces the build's objects.

Not with gccmk: the kernel and libc as a whole (libc's `9syscall` writes 6a
assembly). `x.s` assembles through its GAS port `x.S` beside it (libc, libmp,
libsec, lib387 have them).
