# 9front-gcc

GCC 16.2.0 and Binutils 2.47 for 9front amd64 and 386 (targets
`x86_64-pc-plan9`, `i686-pc-plan9`),
built on Linux as a Canadian cross against the gcc-built 9front tree
(`PLAN9`, default `../9front`). The result, `out/$objtype`, is the tree
`/$objtype/gnu` of an installed system; `../9front`'s `scripts/guest/mkiso.sh`
puts it on the install CD (`scripts/guest/boot/gnuproto`).

Log of the port (German): `NOTES.md`.

## Build

    ./fetch.sh            # sources to src/ (GNU signatures, sha256.txt), patches applied
    ./build.sh cross      # Linux -> 9front: binutils, gcc (C, C++), libgcc, libstdc++ -> build/$objtype/cross
    ./build.sh hostlibs   # gmp, mpfr, mpc for 9front                                   -> build/$objtype/hostlibs
    ./build.sh native     # binutils and gcc that run on 9front                         -> out/$objtype
    ./build.sh compat     # only the toolchain's additions to APE into build/$objtype/cross

`objtype=386` before any of them for 386 (default amd64); build directories
are `build/$objtype`. Needs the 9front tree built for that architecture
(`CC=gcc ARCH=amd64 sys/src/cmd/build/build.exe`).

## Tests

    [objtype=386] tests/cross.sh        # cross-compiled C (APE, native Plan 9) and C++ run in the 9front guest
    [objtype=386] tests/native.sh       # /$objtype/gnu/bin/gcc compiles and runs them in the guest; same code as the cross gcc

## Layout

- `patches/{gcc,binutils}.patch`: the target. `src/gcc` and `src/binutils` are
  git repositories with the tag `pristine` (the tarball); change them there and
  write the patches with `./patches.sh`.
- `compat/include`, `compat/lib`: what APE lacks of C99/C11/POSIX.1-2008, as the
  toolchain's own headers (`$prefix/x86_64-pc-plan9/include`, searched before
  APE's, `#include_next`) and `libapecompat.a`; 9front's APE stays as it is.

## Using it on 9front

    bind -b /amd64/gnu/bin /bin
    gcc -o hello hello.c                       # POSIX over APE: -D_POSIX_SOURCE, -lapecompat -lbsd -lap
    gcc -nostdinc -I/amd64/include -I/sys/include -fplan9-extensions -fno-builtin -c prog.c
    gcc -nostdlib -o prog prog.o -L/amd64/lib.gcc -lc -lgcc     # native Plan 9 C

C defaults to gnu17 (APE's headers are K&R in places). BSD functions and
sockets: `-D_BSD_EXTENSION`, as with pcc. gcc ignores `#pragma lib`: name the
libraries.
