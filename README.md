# 9front-gcc

GCC 16.2.0 and Binutils 2.47 for 9front amd64, 386 and riscv64 (targets
`x86_64-pc-plan9`, `i686-pc-plan9`, `riscv64-unknown-plan9`),
built on Linux as a Canadian cross against the gcc-built 9front tree
(`PLAN9`, default `../9front`; for riscv64 `../9front-riscv`, the tree's
branch `riscv`). The result, `out/$objtype`, is the tree
`/$objtype/gnu` of an installed system; `../9front`'s `scripts/guest/mkiso.sh`
puts it on the install CD (`scripts/guest/boot/gnuproto`).

Log of the port (German): `NOTES.md`.

## Build

    ./fetch.sh            # sources to src/ (GNU's signatures; sha256.txt), patches applied
    ./build.sh cross      # Linux -> 9front: binutils, gcc (C, C++), libgcc, libstdc++ -> build/$objtype/cross
    ./build.sh hostlibs   # gmp, mpfr, mpc for 9front                                   -> build/$objtype/hostlibs
    ./build.sh native     # binutils and gcc (C, C++) that run on 9front                -> out/$objtype
    ./build.sh plan9      # gccmk and its wrappers (plan9/), flags from the tree's build -> out/$objtype

`objtype=386` or `objtype=riscv64` before any of them for those (default amd64); build directories
are `build/$objtype`. Needs the 9front tree built for that architecture
(`CC=gcc ARCH=amd64 sys/src/cmd/build/build.exe`), with gcc's APE (the branch
`gcc`: C99, POSIX.1-2008 and pthreads in APE itself, `sys/src/ape/lib/ap/gnu`,
`sys/src/ape/lib/musl`).

## Tests

    [objtype=386|riscv64] tests/cross.sh        # cross-compiled C (APE, native Plan 9, C99/POSIX: c99*.c, pthread.c) and C++ (cxx*.cc, eh.cc) run in the 9front guest
    [objtype=386|riscv64] tests/native.sh       # /$objtype/gnu/bin/gcc and g++ compile and run them in the guest; same code as the cross compilers
    [objtype=386|riscv64] tests/mk.sh           # gccmk in the guest: libbio, hoc (yacc), ape/cmd/expr (pcc), libc's assembly; same code as the tree's build

## Layout

- `patches/{gcc,binutils}.patch`: the target. `src/gcc` and `src/binutils` are
  git repositories with the tag `pristine` (the tarball); change them there and
  write the patches with `./patches.sh`.
- `plan9/`: `gccmk` and the wrappers it puts in front of mk (installed as
  `/$objtype/gnu/bin/gccmk` and `/$objtype/gnu/plan9`).
- What APE lacks of C99/C11/POSIX.1-2008 for gcc (the former `compat/`,
  `libapecompat.a`) is in the 9front tree's APE now: its headers' parts for gcc
  (`#if defined(__GNUC__)`), `sys/src/ape/lib/ap/gnu` and musl's part
  (`sys/src/ape/lib/musl`) in `libap.a`. The driver links `-lbsd -lap`.

## Using it on 9front

    bind -b /amd64/gnu/bin /bin
    gcc -o hello hello.c                       # POSIX over APE: -D_POSIX_SOURCE, -lbsd -lap
    gcc -nostdinc -I/amd64/include -I/sys/include -fplan9-extensions -fno-builtin -c prog.c
    gcc -nostdlib -o prog prog.o -L/amd64/lib.gcc -lc -lgcc     # native Plan 9 C
    g++ -o prog prog.cc                        # C++: libstdc++ over APE, exceptions

What gcc's APE has beyond kencc's: C99's `<math.h>` (float,
long double, `round`, `fma`, ...), `<fenv.h>`, `<complex.h>`, `<float.h>` with
gcc's values; C99's printf and scanf (`%Lf`, `%a`, `hh`/`j`/`z`/`t`, C99's
`snprintf` result), `strtof`, `strtold`; `<wchar.h>` and `<wctype.h>` with a 32-bit
`wchar_t` in UTF-8, wide I/O; `clock_gettime` (`CLOCK_MONOTONIC` from
`/dev/bintime`), `nanosleep`, `usleep`, `pread`; `<pthread.h>` (a thread is a
proc, `rfork(RFMEM)`), `poll`, `mmap`, `aligned_alloc`. A program starts like on other POSIX systems: x87 and SSE
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
