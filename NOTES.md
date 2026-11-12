# 9front-gcc — Log

Goal: GCC 16.2.0 + Binutils 2.47 run natively on 9front amd64, are on the ISO,
and the system builds itself in the guest (plan: phases 1–6, README.md).

## Target x86_64-pc-plan9 (phase 1)

- `config.sub` already knows `plan9*` (gcc and binutils).
- binutils: `bfd/config.bfd` (x86_64_elf64_vec), `ld/configure.tgt` (elf_x86_64),
  `gas/configure.tgt` (`i386-*-plan9*` → fmt=elf). Nothing else: gcc passes `-T plan9.ld`.
- gcc: `config.gcc` (`*-*-plan9*`: gas, gnu_ld, native_system_header_dir=/sys/include/ape;
  `x86_64-*-plan9*`: tm_file like x86_64-elf, but `glibc-stdint.h` like Linux, + `i386/plan9.h`).
- `i386/plan9.h`:
  - Codegen = x86_64-linux-gnu with the same options (no default PIE/SSP — the tree
    passes `-fno-pie -fno-stack-protector` anyway). Builtins `__plan9__`, `__unix__`/`unix`
    (the Linux gcc defines `__unix__` too; `__linux__` is checked in the tree only by PPC/foreign code).
  - `INCLUDE_DEFAULTS`: C++ directories, `/amd64/include/ape`, `/sys/include/ape`, then gcc's
    own. APE directories are not `cxx_aware` → cpp wraps them in `extern "C"` (APE headers
    have no `__cplusplus` guards; no wrapper directory needed). They come after the
    C++ directories because `<cstdlib>` & co. do `#include_next <stdlib.h>`.
  - Libraries via `STANDARD_STARTFILE_PREFIX_1/2` (`/amd64/lib.gcc/ape/`, `/amd64/lib.gcc/`,
    sysroot-relative, after the user's `-L`); `LIB_SPEC -lap`; no crt files
    (`_main` is in libap/libc).
  - `LINK_SPEC`: `-static -z noseparate-code -z max-page-size=0x1000`, `-T %R/amd64/plan9.ld`
    (if no `-T`), `--wrap=main`.
- Constructors: 9front's `_main` does not know `.init_array`. `amd64/plan9.ld` (9front repo) now
  collects it with `__init_array_start/end`; libgcc gets `config/i386/plan9-main.c`
  (`__wrap_main` calls the constructors, then `__real_main`). Symbols weak.
  `DEFAULT_USE_CXA_ATEXIT 0` (APE has atexit, no `__cxa_atexit`).
- fixincludes off (`--disable-fixincludes`): the APE headers are not copied/modified.

## Way of working

- `src/gcc`, `src/binutils` are git repos with tag `pristine` (= tarball); make changes
  there, `./patches.sh` writes `patches/*.patch`; `fetch.sh` applies them to fresh sources.
- Signatures of all tarballs checked against `gnu-keyring.gpg` (gpgv), sha256 in `sha256.txt`.

## Cross toolchain (phase 2)

- `build.sh cross`: binutils + gcc (c, c++) + libgcc + libstdc++ to `build/cross`, sysroot =
  symlinks into the built 9front tree (`sysroot.sh`).
- APE feature macros: `unistd.h` aborts without `_POSIX_SOURCE`. The driver sets
  `-D_POSIX_SOURCE= -D_BSD_EXTENSION= -D_LIMITS_EXTENSION=` (empty like `#define _POSIX_SOURCE`
  in programs → no redefinition warning), but not with `-nostdinc` (tree builds).
- `_Noreturn` (C11, in APE `stdlib.h`) does not exist in C++: in C++ mode predefined as
  `__attribute__((__noreturn__))` (like glibc `sys/cdefs.h`).
- `.note.GNU-stack`/`.note.gnu.property` like Linux (`file_end_indicate_exec_stack_and_gnu_property`,
  `gnu-property.o`); `-z noexecstack` in LINK_SPEC, because APE's asm stubs have no note.
- `MATH_LIBRARY ""`: math is in libap, g++ should not append `-lm`.
- libstdc++: `crossconfig.m4` got `*-plan9*` (link tests against the sysroot); configure regenerated with
  autoconf **2.69** (host has 2.73; `build/tools`, source in sources.txt).
  `system_error.cc`: `EPROTOTYPE == EPROTONOSUPPORT` (both 37) in APE → `duplicate case`;
  guard in the style of the existing `EWOULDBLOCK != EAGAIN` check.
- **compat/include** (→ `$prefix/x86_64-pc-plan9/include`, `TOOL_INCLUDE_DIR`, in
  `INCLUDE_DEFAULTS` before the APE directories): what C99/C11/POSIX.1-2008 APE lacks, via
  `#include_next` to APE: `errno.h` (EILSEQ, EALREADY, ECONNRESET, ELOOP, ENOMSG, ENOTSUP,
  EOVERFLOW; values from 1100, libap never sets them), `math.h` (fpclassify/FP_*, isfinite,
  isnormal, signbit via builtins), `stddef.h` (max_align_t). Deliberately not in 9front's APE:
  the tree and its builds stay unchanged.
- The C++23 module `std` does not compile (APE has no C99 libm: `sinf`, `strtold`, …);
  libstdc++ catches that itself (empty module, „Cannot compile std module“). Not a bug.
- `gcc_cv_initfini_array=no` in the cross build (configure cannot check it) → static
  constructors were not registered anywhere. `--enable-initfini-array` forces `.init_array`.
- Decimal floating point: the Linux gcc has `__DEC*` macros, our target does not
  („decimal float is not supported for this target“). The tree does not use `_Decimal*`.
- In the guest (boot-cli.sh, `UFS_EXTRA`): APE hello world and a native Plan 9 program
  (`-nostdinc -I…`, `-nostdlib … -lc -lgcc`) from the cross gcc run.

## Host libraries (phase 3)

- gmp 6.3.0: configure tests with K&R declarations fail under gcc 16 (C23 default) →
  `CFLAGS=-std=gnu17` for the host libraries. Duplicate `-D_POSIX_SOURCE` (driver + CFLAGS)
  gave redefinition warnings, which gmp's compiler test counts as errors → the driver only.
- mpfr: `static_assert(MPFR_EXP_MAX > MPFR_PREC_MAX)` failed: **APE's `limits.h` had
  `LONG_MAX`/`ULONG_MAX` fixed at 32 bits**, under gcc long is 64 bits. A real port bug in the
  9front tree (libap `strtol`/`strtoul` limited to 2^31-1) → fixed in 9front:
  `__LONG_MAX__` (only gcc defines it; 6c/8c keep front's values).
- mpfr needs `wint_t` (`<wchar.h>`, missing in APE) → `compat/include/wchar.h` with only types
  and limits (`wint_t`, `mbstate_t`, `WINT_MIN/MAX`, `WEOF`), no functions.
  `wchar_t` stays 32 bits (int) like with the Linux gcc: 9front's `Rune` is `uint`, `L"…"` in the tree.
  (APE's `stddef.h` says `unsigned short wchar_t` — legacy, not touched.)
- mpc 1.4 needs `<complex.h>` → `compat/include/complex.h` (type macros, creal/cimag/conj via
  builtins, CMPLX). mpc `radius.c` uses `PRIi64` → `compat/include/inttypes.h` adds all
  C99 PRI/SCN macros that APE lacks (64 bit/MAX/PTR = long long).

## Gate phase 2: 9front tree with the cross gcc

- Worktree of the 9front repo (commit `01e5c0aa5` + `plan9.ld`) built with
  `CC=gcc TOOLPREFIX=x86_64-pc-plan9- ARCH=amd64 build.exe`: **813 files expected, ok**.
  Compared against a second worktree with the Linux gcc (16.2.1) (`objcopy --strip-debug
  -R .comment -R .note.gnu.property`): **5032 of 5037 objects byte-identical**.
- The 5 differences: `9/pc64/userinit.o`, `pc64.o` embed linked binaries (initcode,
  bootfs.paq) whose `.comment` carries the compiler version (+ paq timestamp); `cmd/gs`
  (gs.r.o, gsmisc.o, gsmalloc.o): `malloc_.h` has `#ifdef linux` — **the Linux gcc defines
  `linux`, so the tree build takes Linux paths** (latent bug in the 9front build, cf.
  MEMORY.md; the plan9 target correctly does not define it).
- A tree for Linux-gcc comparisons needs `../plan9port` next to it (gen-runetype.sh).

## Native toolchain (phase 4)

- Canadian cross: build=Linux, host=target=x86_64-pc-plan9, `--prefix=/amd64/gnu`,
  `DESTDIR=out/amd64`. `ac_cv_c_bigendian=no` (AC_C_BIGENDIAN finds nothing in the Canadian build),
  `CXXFLAGS=-DCODY_NETWORKING=0` (libcody assumes sockets with `__unix__`).
- **C default gnu17 instead of gnu23** (new target macro `C_DEFAULT_GNU17`, `c-family/c-opts.cc`):
  APE declares signal handlers K&R `void (*)()` (deliberately without parameter types: handlers with
  note + Ureg); C23 reads `()` as `(void)`. An explicit `-std=` wins.
- **libcpp bug (upstream)**: `_cpp_post_stack_file` takes `sysp` as `bool` → 2 („C header,
  implicitly extern "C"") becomes 1. So `SYSTEM_IMPLICIT_EXTERN_C` never took effect (AIX is affected
  the same way). Fix: `int`. Found with gdb: directory sysp=2, line map sysp=1.
- `SYSTEM_IMPLICIT_EXTERN_C` in plan9.h: APE headers without `extern "C"` (e.g. `sys/time.h`).
- `_BSD_EXTENSION` **no longer** by default: `bsd.h` has non-POSIX prototypes (getopt,
  strcasecmp without const) → conflicts with programs. BSD-only headers (`sys/param.h`,
  `sys/select.h`, `select.h`, `sys/ioctl.h`, `sys/uio.h`, `sys/resource.h`, `sys/socket.h`,
  `netdb.h`) enable it in compat only for themselves. `LIB_SPEC "-lapecompat -lbsd -lap"`
  (gcc ignores `#pragma lib`).
- compat headers (renaming trick `#define f __ape_f` around the `#include_next`, then
  the POSIX declaration of the same libap function): `execv/execve/execvp` (`char *const[]`),
  `strndup` (const), `signal`/`SIG_*`/`struct sigaction` (only C++ and C23; layout checked via
  array-size check against APE's struct), `ptrdiff_t` (= `__PTRDIFF_TYPE__` long like
  libstdc++'s `std::ptrdiff_t`; APE: long long → „ambiguous“ in C++).
- compat headers without renaming: `stdbool.h` (replaces APE's C89 version), `strings.h`,
  `string.h` (+strings.h, stpcpy, stpncpy), `unistd.h` (getopt, mktemp, symlink, truncate),
  `sys/wait.h` (wait3/wait4 + struct rusage), `sys/stat.h` (fchmod, lstat, S_IF*),
  `stdint.h` (INTn_C, PTR/PTRDIFF/SIG_ATOMIC/WCHAR/WINT limits), `stddef.h` (offsetof as
  `__builtin_offsetof`, max_align_t), `dlfcn.h`, `stdio.h` (asprintf, vasprintf, popen).
- **libapecompat.a** (compat/lib): asprintf/vasprintf (growing buffer: APE's vsnprintf
  does not return C99's total length), stpcpy, stpncpy, strndup (APE declares, does not
  implement), truncate, dlopen & co. (always fails, dlerror says why — ld calls dlopen for
  `-plugin` even without plugin support), `readlink` as `__apecompat_readlink` (asm label):
  libbsd's readlink returns `int`, a POSIX caller reads `ssize_t` → -1 would come out as 2^32-1.
- 9front tree (real LP64 bugs, fixed): `sys/include/ape/stdint.h` `SIZE_MAX` was
  `UINT32_MAX` (under gcc size_t is 64 bits) → `__SIZE_MAX__`.
- Native binutils (as, ld, ar, nm, objcopy, objdump, ranlib, strip, readelf, size, …) built.
- `make_relative_prefix` (libiberty) searches for the program via `$PATH`; 9front's rc knows only
  `path`, and with binds `gcc` lies under `/bin`, independent of the install location. The driver
  skips relocation on a Plan 9 host (`__plan9__`) and takes the configured
  paths (`/amd64/gnu/...`). Before: `programs: =../libexec/...` → „cc1: not found“.
- **libiberty bug (upstream)**: `lrealpath` without `realpath`/`canonicalize_file_name` falls to the
  end of the function without `return` (only the Windows branch returns something) → garbage; ld reported
  „input file 'hi.o' is the same as output file“. Fix: `return strdup (filename);` (in gcc and
  binutils), plus `realpath` in libapecompat (absolute, `.`/`..` cleaned, exists; no
  symlinks on 9front).
- libgcc with `-std=gnu23` (`t-plan9`): it is written for gcc's own default (`bool`).
- `install-strip`: cc1 389 MB → 40 MB, `/amd64/gnu` in total 107 MB.
- **Gate phase 4** (`tests/native.sh`): in the guest `gcc` → APE program runs, native
  Plan 9 program (`-nostdinc … -lc -lgcc`) runs, object byte-identical with the cross gcc.

## 386: i686-pc-plan9

- Target entry `i[34567]86-*-plan9*` (gcc, libgcc, bfd `i386_elf32_vec`, ld `elf_i386`, gas already
  generic). `i386/plan9-386.h` before `plan9.h`: `PLAN9_OBJTYPE "386"`,
  `-mincoming-stack-boundary=2` by default (9front's 386 programs start with only a 4-byte
  aligned stack; the tree builds the same way, `386/cflags.json`), `--with-arch=pentium` like
  `-march=pentium` in the tree. `DEFAULT_PCC_STRUCT_RETURN` stays 1 (= Linux i386).
- libgcc: the unwinder (`__builtin_eh_return`) cannot tolerate stack realignment → for
  `unwind-dw2.c`/`unwind-sjlj.c` `-mincoming-stack-boundary=4` (`CFLAGS-<file>` in `t-plan9`).
- compat: `stdint.h`/`inttypes.h` depending on pointer size (`intptr_t` is `int` on 386),
  `PTRDIFF_*` from `__PTRDIFF_MAX__`; `ctype.h` (`isascii`/`toascii`, in APE only BSD; gmp).
- `386/plan9.ld` collects `.init_array` like amd64.
- Build structure is now `build/$objtype/…` (`objtype=386 ./build.sh …`), `out/$objtype`; amd64's
  old build in `build/` (symlinks from `build/amd64`) is replaced (see „APE extended for C++“).
- Cross, host libs, native built for 386 (`/386/gnu` 80 MB).
- Guest (9pc): `objtype=386 tests/cross.sh` (APE, native, C++) and `objtype=386 tests/native.sh`
  (APE, native, object byte-identical with the cross gcc) passed.

## gccmk: mk with gcc on 9front (`plan9/`, `build.sh plan9`)

- `gccmk` = mk in a namespace: `/$objtype/mkfile` → `/$objtype/gnu/plan9/mkfile`
  (`CC=gcc-cc LD=gcc-ld AS=gcc-as`, `O` as with kencc), `bind -b …/plan9/bin /bin` (`pcc`, `ar`),
  `bind -c /$objtype/lib.gcc /$objtype/lib`. Flags = those of the tree build (`cflags.json`, `gcc.json`,
  `ape.json`, Oflags/Libs; library list from `sys/src/cmd/cmds.json` or `ape/cmd`), stored by
  `build.sh plan9` as one line per argument (`lib.rc` reads with `ifs=$nl`).
- Linking like `build.exe` (`presolve`): `ld -r`, common symbols weak (`objcopy --weaken-symbols`),
  then all libraries in one group; 386 needs `libgcc.a` (64-bit division) from
  `cflags.json`'s `Libs`.
- `ar`: GNU ar with `r` (mksyslibs `ar vu` names no operation) and `s` (index for GNU ld);
  **members are named `x.o` like with build.exe** — otherwise `x.6` would sit next to the `x.o` of the build
  in the archive and ld might take the old one.
- `tests/mk.sh` (amd64 and 386 green): libbio (`gccmk install`, no `.$O` members), objects
  equal to the tree build, libc assembler via the GAS ports, hoc (yacc) and ape/cmd/expr (pcc)
  run. Comparison with `--strip-unneeded` without `.eh_frame`: the Linux gcc for i386 produces
  unwind tables by default, i686-pc-plan9 does not (code and relocations equal).
- Limits: kernel, libc as a whole (`9syscall` writes 6a assembler). libmp/libsec (amd64, 386)
  and `libc/386/387` now have GAS ports (9front repo, `tests/asm`); `tests/mk.sh` builds
  libmp/libsec assembler with gccmk and compares with the tree build.

## C++ native: g++ on 9front (amd64, 386)

- Native gcc now with `--enable-languages=c,c++`: `g++`, `cc1plus`, libstdc++/libsupc++ and
  the C++ headers under `/$objtype/gnu` (amd64 152 MB, 386 143 MB). libstdc++ options shared by
  cross and native (`LIBSTDCXX_CONF` in build.sh).
- **Exceptions worked nowhere** (not even with the cross g++, `tests/cxx.cc` avoided them): `throw`
  → `std::terminate` → „abort“. Cause: no crt files, so nobody registered `.eh_frame`
  with the unwinder, and without `dl_iterate_phdr` there is no other way to the FDEs. Fix as on
  other ELF systems without `PT_GNU_EH_FRAME`: libgcc builds `crtbegin.o`/`crtend.o` (`extra_parts`,
  `i386/t-crtstuff`, no PIC), `STARTFILE_SPEC`/`ENDFILE_SPEC` name them (not with
  `-nostdlib`/`-nostartfiles`, so not in the tree build). `frame_dummy` is in `.init_array`,
  `__wrap_main` calls it; `__register_frame_info` is weak there → C programs do not pull in
  the unwinder. crtstuff needs `-std=gnu23` like libgcc2 (`CRTSTUFF_T_CFLAGS`).
- 9front repo: `plan9.ld` (amd64, 386) collects `.gcc_except_table*` in one section in the
  text segment (before: orphans). C programs link byte-identically with it as before.
- **Canadian trap:** the target libraries are built by the installed cross g++; libstdc++'s configure
  thereby saw the *installed* libstdc++ headers — whose `<fenv.h>` wrapper is empty (APE has no
  fenv.h) → „fenv.h usability… yes“, `HAVE_FENV_H`, abort in `floating_to_chars.cc`. The
  cross build takes `xgcc -nostdinc++`; native now likewise: `CXX_FOR_TARGET="$TARGET-g++ -nostdinc++"`.
- build.sh: `conf` remembers the configure arguments (`$B/<dir>.args`) and reconfigures on
  change (before, an old tree was left standing); if the file is missing, the tree counts as up to date.
- Tests: `tests/eh.cc` (throw across frames, rethrow, destructors while unwinding, `vector::at`).
  `tests/cross.sh` (ape, plan9, cxx, eh) and `tests/native.sh` (+ cxx, eh with the native g++,
  `samecxx`: object of eh.cc byte-identical with the cross g++) green for amd64 and 386.

## APE extended for C++ (compat, musl)

Goal: libstdc++ with everything it finds beyond C99/POSIX. Decision: everything in the toolchain
(`compat/`, `libapecompat.a`), 9front's APE stays — except for real port bugs (malloc, see below).

- **musl 1.2.5** as source (`sources.txt` with key column; `keys/musl.pub`, fingerprint
  8364 8929 0BB6 B70F 99FF DA05 56BC DB59 3020 450F; `fetch.sh` checks `.asc`). `build.sh musl`
  builds with musl's own Makefile/headers (`$B/musl`) and puts objects into `compat-lib/musl-*.o`:
  - libm, fenv, complex (arch files replace generic ones as in musl's Makefile).
  - Wide-string functions (`wcs*`, `wcp*`, `wmem*`) and `<wctype.h>` (`isw*`, `tow*`, `wcwidth`):
    without locale, Unicode tables.
  - Objects whose strong symbols libap all has (C89 double math, j0, erf …) are dropped; in the
    rest libap names become local (`objcopy --localize-symbol`): i386 `scalbn.s` is also
    `ldexp`, `exp10.c` has a weak `pow10(double)` (APE: `pow10(int)`!).
  - **Engine** (`musl-engine.o`, `ld -r`): musl's vfprintf/vfscanf/strtod/vfwprintf/vfwscanf/
    vswprintf/vswscanf/wcsto* + `compat/musl/bridge.c` (translated in musl's environment); all symbols
    local except `__apecompat_m*` and `wcsto*`. Bridge FILE = musl's string FILE with write callback
    to APE's `fwrite` and a read callback that reads APE's `getc` byte by byte (1-byte buffer like
    `__stdio_read`: musl's `ungetwc` macro and `shgetc` reset `rpos` and expect the byte
    in the buffer). What scanf returns goes back to APE's FILE via `ungetc`. musl's errno ends up in a
    variable of its own (`___errno_location`) and is translated into APE's numbers (musl ENOMEM=12 is
    APE's EINVAL!). `__lockfile` is never called (lock -1). Wide functions (fputwc, getwc,
    ungetwc) are the engine's own: musl's query the locale via the thread pointer (`%fs`).
- **printf/scanf**: APE's reads every floating-point argument as double (`%Lf` gave garbage), does not know `%a`,
  `%F`, `hh`, `j`, `t`, `snprintf` does not return C99's length. compat `stdio.h`: APE's
  declarations are renamed, the own ones carry link names `__apecompat_*` (asm labels) — only
  where libap has the same name; otherwise normal names (configure's link tests declare themselves:
  `strtold` with a label was not found).
- **x87/SSE at start** (libgcc `__wrap_main`): 9front's kernel gives CW 0x0232 and MXCSR 0x1900 —
  division by zero, overflow, invalid trap („sys: fp: division by zero“), and the x87
  computes in 53 bits (long double = double!). Now CW 0x037f, MXCSR 0x1f80 (amd64; 386 has
  masked SSE exceptions from the kernel, a Pentium has no MXCSR), like every POSIX x86. Only for programs linked
  by the driver; `-nostdlib` (tree) keeps the kernel's.
- **`<float.h>`**: APE's (`/$objtype/include/ape/float.h`) describes kencc: `LDBL_*` = double,
  `LDBL_MAX` a double constant. compat takes gcc's values (+ C99/C11 macros), APE's rest
  (FPdbleword, FP* bits) stays.
- **`wchar_t`**: gcc's 32 bits; compat `stddef.h` defines `_WCHAR_T` before APE's header (which otherwise
  typedefs `unsigned short`). APE's `mbtowc` & co. write 16 bits → replaced under labels
  (`compat/lib/multibyte.c`, strict UTF-8: overlong forms, surrogates, > 0x10FFFF = EILSEQ).
  Wide I/O (`wstdio.c`): ungetwc returns the bytes via `ungetc` into APE's buffer, otherwise
  (unbuffered, buffer boundary) into a table, as long as fd and buffer of the FILE are the same.
- **`intmax_t`** = `__INTMAX_TYPE__` (long on amd64; APE: long long): otherwise `-Wformat` for `%jd`
  and a type different from `std::intmax_t`. `int64_t` stays APE's long long (APE's prototypes with
  `long long *`, gcc ≥ 14 aborts on incompatible pointers).
- Time: `clock_gettime` via `/dev/bintime` (4th field = ns since boot, monotonic, `tod.c`), kept
  open permanently, `pread` (new in compat: libap's `_PREAD` + `_syserrno`). `usleep`, `sched_yield`
  (sleep(0)), `sysconf` with processor count (lines of `/dev/sysstat`) and page size.
  `unistd.h` reports `_POSIX_TIMERS`, `_POSIX_MONOTONIC_CLOCK`, `_POSIX_THREADS`,
  `_POSIX_TIMEOUTS` … (libstdc++ checks steady_clock and timed_mutex for them).
- **pthreads** (`compat/lib/pthread.c`, one object: `pthread_cancel` is gthr's proxy for
  „threads active“): thread = proc, `rfork(RFPROC|RFMEM|RFNOWAIT)`. A proc has its own
  stack segment (copy); on it lie APE's errno (`_callmain`'s local variable, `_errnoloc`
  points there) and `_privates` → both per thread without further ado; `_privates[0]` = thread. Threads
  run on stacks from malloc (shared memory), `main` too (`__plan9_mainstack`, called by
  libgcc's `__wrap_main` when linked; 16 MiB like USTKSIZE) — otherwise other threads would not see
  main's locals (lambdas with `[&]`). **Trap**: after `rfork(RFMEM)` the child stands on the
  (shared) heap stack of the parent → both run on the same stack (crash with pc in the
  heap). `__apecompat_rforkthread` (asm) makes the syscall itself and in the child immediately sets SP to
  `privtop` (below `__plan9_mainstack`'s frame, free in every copy); values in registers that the
  child gets (amd64 `forkret`: only R14/R15, rest zero; 386: all except AX). There the
  thread runs via `runon` on its heap stack and returns for cleanup (whoever frees the stack
  frees one on which nobody runs). Waiting via 9front semaphores (32-bit, `s32int`):
  mutex after Drepper (3 states), condition with queue and one semaphore per thread,
  rwlock (writers first), once, keys with sequence numbers. `exit` ends the other threads
  (`/proc/n/ctl` „kill“). No `pthread_cancel`, no cross-process objects.
- gcc: `--enable-threads=posix` (gthr-posix, emutls for `thread_local`), libstdc++ without
  `--disable-wchar_t`/`--disable-libstdcxx-threads`, with `--enable-libstdcxx-time=yes`;
  driver accepts `-pthread`. libstdc++ now finds: C99 math/stdio/stdlib/wchar/complex/fenv/
  inttypes, CLOCK_MONOTONIC, nanosleep, sched_yield, rwlock, timedlock, nprocs.
- **9front tree (real port bug)**: APE's malloc returned only 8-byte-aligned blocks
  (stride `&~7`, sbrk 4-aligned), gcc/g++ assume 16 (`max_align_t`,
  `__STDCPP_DEFAULT_NEW_ALIGNMENT__`, aligned SSE accesses). Fix in
  `sys/src/ape/lib/ap/plan9/malloc.c` (16 stride, `sbrk16`).
- **amd64 build directories**: the old symlinks `build/amd64/* -> ../*` (layout before 386) made
  `build/amd64/cross` a symlink to `build/cross`; a newly configured gcc with prefix
  `build/amd64/cross` found itself in `build/cross/bin` and thought it had been moved → `incpath.cc`
  then puts everything under `lib/gcc/…` first, gcc's `stddef.h` before APE's (error
  `max_align_t`). amd64 is now rebuilt in real directories like 386.
- Tests: `tests/c99math.c` (`-frounding-math`: otherwise gcc moves `rint` across `fesetround`),
  `c99stdio.c`, `c99time.c`, `c99wchar.c`, `pthread.c`, `cxxlib.cc` (in `cross.sh`; `cxxlib` also
  native). Test traps: gcc folds `rint(-x)` to `-rint(x)`; on 386 gcc inlines `sqrtf` as
  `fsqrt` with excess precision; glibc's „C“ locale is not UTF-8 (host test: `C.UTF-8`).
- Open: APE's stdio has no locks (threads sharing a FILE must coordinate;
  `std::cout` from several threads); `_exit`/abort do not end other threads; no locales
  other than „C“; `<uchar.h>` is missing.

## riscv64: riscv64-unknown-plan9 (branch riscv, tree `../9front-riscv`)

- **gcc target split as on other systems:** `gcc/config/plan9.h` has the system
  (include paths, feature macros, libraries, crtbegin/crtend, `--wrap=main`, gnu17,
  `SYSTEM_IMPLICIT_EXTERN_C`), `i386/plan9.h` resp. `riscv/plan9.h` the machine
  (`PLAN9_OBJTYPE`, `PLAN9_CC1_SPEC`, `PLAN9_LINK_SPEC`; x86: `.note.gnu.property`).
  libgcc: `config/plan9-main.c` and `config/t-plan9` shared, `i386/t-plan9` only the
  unwinder alignment now. **No effect for x86:** amd64 and 386 rebuilt, `-dumpspecs`,
  C and C++ macros, `ape.o`/`eh.o` byte-identical with before; `tests/cross.sh` 10/10 each.
  Trap: after a changed `config.host` `build.sh` does not reconfigure libgcc (only with
  different configure arguments) → delete `build/$objtype/cross-gcc/$TARGET/libgcc`.
- **riscv64 target:** `config.gcc` `riscv*-*-plan9*` (`elfos.h glibc-stdint.h … riscv/plan9.h
  plan9.h`, `riscv/t-riscv`, unwind tables by default like riscv64-linux-gnu); configure
  `--with-arch=rv64gc --with-abi=lp64d --with-cmodel=medany` like `riscv64/cflags.json`.
  `-mno-relax`/`--no-relax` by default: nobody sets gp (`__global_pointer$`) — neither
  9front's `_main` nor the kernel, jc takes the register as SB. `char` signed
  (`DEFAULT_SIGNED_CHAR`, like jc and `-fsigned-char` in the tree). libgcc like `riscv*-*-*`
  (softfp, save-restore) with crtbegin/crtend. binutils: nothing (the generic
  `riscv64*-*-*` entries in config.bfd, ld, gas apply).
- compat: `fenv.h` with fcsr values (musl's riscv64 bits), `pthread.c` `rfork`/`runon` in
  riscv64 assembler (number and result in s0, argument at 8(sp), `ecall`; the child inherits
  all registers except s0: privtop/t in s1/s2). `string.h` declares `strtok_r` (libap has it,
  APE only under `_REENTRANT_SOURCE` — which would bring APE's non-POSIX `strerror_r`; gas'
  `tc-riscv.c` needs it). gccmk: `lib.rc` knows riscv64 (O=j); `tests/mk.sh` takes the
  `objcopy` of the cross toolchain (the host's does not know riscv64), compares memmove,
  strcmp, setjmp; libmp/libsec have no assembler for riscv64 (mpsec dropped).
- Tree (`../9front-riscv`): `riscv64/plan9.ld` collects `.init_array` and
  `.gcc_except_table` like amd64 (C programs link byte-identically).
- **`long double` is IEEE quad** (128 bits, software via libgcc). Tests had x87 values
  hard-coded (`%.18Lf` of π, LDBL_MAX string) → now depending on `LDBL_MANT_DIG`. **musl has no
  real `expl`, `logl`, `log1pl` for quad** (`TODO: broken implementation to make things
  compile`, they call the double function; `atanhl` on top of that) — as with musl on riscv64 Linux;
  `c99math` checks double precision there. Open.
- Gates: `tests/cross.sh` 10/10, `tests/native.sh` 7/7 (gcc/g++ on 9front riscv64, objects
  byte-identical with the cross compiler), `tests/mk.sh` 5/5 (riscv64 guest under TCG: `MEM=2048
  CMDWAIT=1800`). Tree built with `TOOLPREFIX=riscv64-unknown-plan9-`: 817 files ok,
  **5811 of 5830 objects byte-identical** with the Linux cross build (riscv64-linux-gnu-gcc
  16.2.1); the 19 others: cwfs `conf.o`/`.r.o` (`-DDATE=$(date +%s)`), `virt.o` (embeds
  bootfs.paq with compiler version and timestamps).
- `/riscv64/gnu` 194 MB. Open: self-build of the tree in the riscv64 guest (very long under TCG),
  no riscv64 ISO.

## arm64: aarch64-pc-plan9 (2026-10-31, for 9front-rust)

- binutils: `aarch64-*-plan9*` in config.bfd, gas/configure.tgt, ld/configure.tgt (the
  ELF entries, emulation aarch64elf).
- gcc: `config.gcc` `aarch64*-*-plan9*` (`elfos.h glibc-stdint.h aarch64-elf.h aarch64-errata.h
  aarch64/plan9.h plan9.h`, `aarch64/t-aarch64`, unwind tables by default like
  aarch64-linux-gnu). `aarch64/plan9.h`: `PLAN9_OBJTYPE "arm64"`, `char` signed
  (7c, the tree's `-fsigned-char`), `-mno-outline-atomics` by default (their helpers choose LSE
  via getauxval, which Plan 9 does not have; the tree builds this way). libgcc: aarch64-t-aarch64, softfp
  (`long double` is quad), crtfm, **`aarch64/t-crtstuff`** (new, like riscv: crt*.o without
  unwind tables, otherwise `__EH_FRAME_BEGIN__` points to crtbegin's own .eh_frame and the
  unwinder does not find the program's: C++ exceptions ended in terminate), no t-lse.
  `build.sh`/tests: `objtype=arm64`, gccmk O=7.
- Tree (gcc branch): `arm64/plan9.ld` collects `.init_array` and `.gcc_except_table` like amd64
  (without it `frame_dummy` never ran); C programs stay byte-identical. `/sys/include/ape/fenv.h` had
  the x87 values for arm64 (FE_UPWARD 0x800 instead of fpcrs 0x400000): fesetround rejected the
  rounding modes.
- `tests/cross.sh` arm64 10/10 (guest under TCG, one processor).
- **native** (`build.sh hostlibs native plan9`, `/arm64/gnu` 148 MB): gcc on an aarch64 host
  includes `host_detect_local_cpu` (`-mcpu=native`), but `driver-aarch64.o` only for known
  hosts: `gcc/config.host` adds `aarch64*-*-plan9*` (it reads `/proc/cpuinfo`; if that is missing,
  `-mcpu=native` detects nothing). Linker for rustc on 9front arm64 (9front-rust).

## Gaps from the Mesa port (branch `gpu` of the tree, „9front next generation“)

When cross-building Mesa's venus (`../9front`, branch `gpu`, `scripts/mesa`), gaps showed up
that had so far been worked around in the Mesa build. Fixed at the root; in the tree
9front code may be changed on branch `gpu`.
- **`-pthread`**: The specs swallowed it (`%{pthread:}`), but the driver did not know the option
  („unrecognized command-line option“). `config/plan9.opt` (`pthread` as
  `Driver` option, plus `plan9.opt.urls`, which GCC 16 requires), `config.gcc`
  `extra_options` for `*-*-plan9*`.
- **Link order**: `LIB_SPEC` puts `-lapecompat -lbsd -lap` into a group
  (`--start-group`): libap calls libbsd (mkstemp → mktemp) and vice versa; under 9front
  `#pragma lib` resolves this without order.
- **compat**: `readlink`/`symlink` only if APE does not declare them POSIX-conformantly
  (APE's `unistd.h` then defines `__APE_POSIX_LINKS`; libbsd's are then POSIX
  signatures). `sysconf(_SC_PHYS_PAGES)` from `/dev/swap`. pthread barriers
  (`pthread_barrier_*`), `pthread_sigmask` (APE's `sigprocmask`), `pthread_getcpuclockid`
  (ENOENT: no CPU-time clocks).
- Tree (branch `gpu`), APE: `int8_t` is `signed char` (gcc: `char*` and `signed char*`
  are different types; gcc's own `__INT8_TYPE__` is `signed char` anyway);
  `posix_memalign`, `aligned_alloc`, `memalign` in `malloc.c` (the aligned block
  lies inside one from malloc, in front of it a header with magic `ALIGNED`, which `free`/`realloc`
  follow) — before, meson reported `posix_memalign` as present (gcc knows the builtin),
  APE did not have it; `mkstemp`/`mkstemps` without libbsd's `mktemp`, declared under `_POSIX_SOURCE`;
  POSIX `lstat`, `symlink`, `readlink` (libbsd); `static_assert` (C11) in
  `assert.h`; `SIGSYS`; `setenv`/`unsetenv`; `localtime_r` & co. under `_POSIX_SOURCE`;
  `putenv` wrote an entry behind the enlarged `environ` (n+1 instead of n+2).

## compat in APE (2026-10-29)

compat (`compat/`, `libapecompat.a`, musl as source) has moved into the 9front tree
(branch `gcc`): APE's headers have the gcc parts under `#if defined(__GNUC__)`, the
functions are in `sys/src/ape/lib/ap/gnu`, musl's subset in `sys/src/ape/lib/musl`,
all in `libap.a`. The driver links `--start-group -lbsd -lap --end-group`, no
tool include directory any more; `build.sh compat` and musl in `sources.txt` are gone.
Needs a tree with this APE (`PLAN9`; `gpu` must merge `gcc`, `../9front-riscv`
likewise). `tests/cross.sh` amd64 against the gcc worktree: all 10 green.

## 386: intptr_t like size_t (2026-10-30)

i686-pc-plan9 had `size_t`/`ptrdiff_t` `(unsigned) long` (gcc's default, like Plan 9's
`usize`), but `intptr_t`/`uintptr_t` `(unsigned) int` (glibc-stdint.h for 32 bits): two
types of equal size, gcc 14 rejects a `uintptr_t *` for a `size_t *` (curl's
`rustls.c` against rustls-ffi, when building cargo for 386 in 9front-rust). `i386/plan9-386.h`:
`INTPTR_TYPE`/`UINTPTR_TYPE` `long`, like size_t and u.h's `uintptr`; no ABI change.
APE's `inttypes.h` (branch gcc) gives PRI*PTR an `l` for it (under `__plan9__`). Toolchain in
its own build directory (`BUILD=build-gcc`, against ../9front-gccwt); `tests/cross.sh` 386: 10/10.

## Threads: GTHREAD_USE_WEAK 0 (2026-10-30)

libgcc's gthr-posix checks via a weak reference to `pthread_cancel` whether a program
has threads (`__gthread_active_p`). A weak reference does not pull an object out of
`libap.a`, though: in a Rust program (own rfork procs, no APE pthread) it was 0, the
unwinder's mutexes (FDE lists, unwind-dw2-fde) did nothing. Concurrent panics in
several threads (Rust's std tests with 12 test threads) aborted phase 2 (`abort`), the
program hung. `gcc/config/plan9.h`: `GTHREAD_USE_WEAK 0` like darwin.h and cygwin.h; APE's
pthreads are a fixed part of libap, their mutexes work for every proc (`pthread_self` knows
std's procs). libstdc++ (`config/os/generic`) already had `_GLIBCXX_GTHREAD_USE_WEAK 0`.
