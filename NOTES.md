# 9front-gcc — Log

Ziel: GCC 16.2.0 + Binutils 2.47 laufen nativ auf 9front amd64, liegen auf der ISO,
und das System baut sich im Gast selbst (Plan: Phasen 1–6, README.md).

## Target x86_64-pc-plan9 (Phase 1)

- `config.sub` kennt `plan9*` schon (gcc und binutils).
- binutils: `bfd/config.bfd` (x86_64_elf64_vec), `ld/configure.tgt` (elf_x86_64),
  `gas/configure.tgt` (`i386-*-plan9*` → fmt=elf). Sonst nichts: gcc übergibt `-T plan9.ld`.
- gcc: `config.gcc` (`*-*-plan9*`: gas, gnu_ld, native_system_header_dir=/sys/include/ape;
  `x86_64-*-plan9*`: tm_file wie x86_64-elf, aber `glibc-stdint.h` wie Linux, + `i386/plan9.h`).
- `i386/plan9.h`:
  - Codegen = x86_64-linux-gnu bei gleichen Optionen (kein default PIE/SSP — der Tree
    übergibt `-fno-pie -fno-stack-protector` ohnehin). Builtins `__plan9__`, `__unix__`/`unix`
    (der Linux-gcc definiert `__unix__` auch; `__linux__` prüft im Tree nur PPC-/Fremdcode).
  - `INCLUDE_DEFAULTS`: C++-Verzeichnisse, `/amd64/include/ape`, `/sys/include/ape`, dann gccs
    eigenes. APE-Verzeichnisse sind nicht `cxx_aware` → cpp wickelt sie in `extern "C"` (APE-Header
    haben keine `__cplusplus`-Guards; kein Wrapper-Verzeichnis nötig). Sie stehen nach den
    C++-Verzeichnissen, weil `<cstdlib>` & Co. `#include_next <stdlib.h>` machen.
  - Bibliotheken über `STANDARD_STARTFILE_PREFIX_1/2` (`/amd64/lib.gcc/ape/`, `/amd64/lib.gcc/`,
    sysroot-relativ, nach den `-L` des Nutzers); `LIB_SPEC -lap`; keine crt-Dateien
    (`_main` steckt in libap/libc).
  - `LINK_SPEC`: `-static -z noseparate-code -z max-page-size=0x1000`, `-T %R/amd64/plan9.ld`
    (wenn kein `-T`), `--wrap=main`.
- Konstruktoren: 9fronts `_main` kennt kein `.init_array`. `amd64/plan9.ld` (9front-Repo) sammelt
  es jetzt mit `__init_array_start/end`; libgcc bekommt `config/i386/plan9-main.c`
  (`__wrap_main` ruft die Konstruktoren, dann `__real_main`). Symbole weak.
  `DEFAULT_USE_CXA_ATEXIT 0` (APE hat atexit, kein `__cxa_atexit`).
- fixincludes aus (`--disable-fixincludes`): die APE-Header werden nicht kopiert/verändert.

## Arbeitsweise

- `src/gcc`, `src/binutils` sind git-Repos mit Tag `pristine` (= Tarball); Änderungen dort
  machen, `./patches.sh` schreibt `patches/*.patch`; `fetch.sh` spielt sie auf frische Quellen.
- Signaturen aller Tarballs gegen `gnu-keyring.gpg` geprüft (gpgv), sha256 in `sha256.txt`.

## Cross-Toolchain (Phase 2)

- `build.sh cross`: binutils + gcc (c, c++) + libgcc + libstdc++ nach `build/cross`, Sysroot =
  Symlinks in den gebauten 9front-Baum (`sysroot.sh`).
- APE-Feature-Makros: `unistd.h` bricht ohne `_POSIX_SOURCE` ab. Der Treiber setzt
  `-D_POSIX_SOURCE= -D_BSD_EXTENSION= -D_LIMITS_EXTENSION=` (leer wie `#define _POSIX_SOURCE`
  in Programmen → keine Redefinitions-Warnung), aber nicht bei `-nostdinc` (Tree-Builds).
- `_Noreturn` (C11, in APE-`stdlib.h`) gibt es in C++ nicht: im C++-Modus vordefiniert als
  `__attribute__((__noreturn__))` (wie glibc `sys/cdefs.h`).
- `.note.GNU-stack`/`.note.gnu.property` wie Linux (`file_end_indicate_exec_stack_and_gnu_property`,
  `gnu-property.o`); `-z noexecstack` im LINK_SPEC, weil APEs asm-Stubs keine Notiz haben.
- `MATH_LIBRARY ""`: Mathe steckt in libap, g++ soll kein `-lm` anhängen.
- libstdc++: `crossconfig.m4` bekam `*-plan9*` (Link-Tests gegen den Sysroot); configure mit
  autoconf **2.69** neu erzeugt (Host hat 2.73; `build/tools`, Quelle in sources.txt).
  `system_error.cc`: `EPROTOTYPE == EPROTONOSUPPORT` (beide 37) in APE → `duplicate case`;
  Guard im Stil der vorhandenen `EWOULDBLOCK != EAGAIN`-Abfrage.
- **compat/include** (→ `$prefix/x86_64-pc-plan9/include`, `TOOL_INCLUDE_DIR`, in
  `INCLUDE_DEFAULTS` vor den APE-Verzeichnissen): was C99/C11/POSIX.1-2008 APE fehlt, per
  `#include_next` auf APE: `errno.h` (EILSEQ, EALREADY, ECONNRESET, ELOOP, ENOMSG, ENOTSUP,
  EOVERFLOW; Werte ab 1100, libap setzt sie nie), `math.h` (fpclassify/FP_*, isfinite,
  isnormal, signbit über Builtins), `stddef.h` (max_align_t). Bewusst nicht in 9fronts APE:
  der Tree und seine Builds bleiben unverändert.
- Das C++23-Modul `std` kompiliert nicht (APE hat kein C99-libm: `sinf`, `strtold`, …);
  libstdc++ fängt das selbst ab (leeres Modul, „Cannot compile std module“). Kein Fehler.
- `gcc_cv_initfini_array=no` im Cross-Build (configure kann es nicht prüfen) → statische
  Konstruktoren wurden nirgends eingetragen. `--enable-initfini-array` erzwingt `.init_array`.
- Dezimal-Gleitkomma: der Linux-gcc hat `__DEC*`-Makros, unser Target nicht
  („decimal float is not supported for this target“). Der Tree nutzt `_Decimal*` nicht.
- Im Gast (boot-cli.sh, `UFS_EXTRA`): APE-Hello-World und natives Plan-9-Programm
  (`-nostdinc -I…`, `-nostdlib … -lc -lgcc`) vom Cross-gcc laufen.

## Host-Bibliotheken (Phase 3)

- gmp 6.3.0: configure-Tests mit K&R-Deklarationen scheitern unter gcc 16 (C23-Default) →
  `CFLAGS=-std=gnu17` für die Host-Bibliotheken. Doppelte `-D_POSIX_SOURCE` (Treiber + CFLAGS)
  gaben Redefinitions-Warnungen, die gmps Compiler-Test als Fehler wertet → nur der Treiber.
- mpfr: `static_assert(MPFR_EXP_MAX > MPFR_PREC_MAX)` scheiterte: **APEs `limits.h` hatte
  `LONG_MAX`/`ULONG_MAX` fest auf 32 Bit**, unter gcc ist long 64 Bit. Echter Port-Fehler im
  9front-Baum (libap `strtol`/`strtoul` begrenzten auf 2^31-1) → in 9front gefixt:
  `__LONG_MAX__` (nur gcc definiert es; 6c/8c behalten front's Werte).
- mpfr braucht `wint_t` (`<wchar.h>`, fehlt in APE) → `compat/include/wchar.h` nur mit Typen
  und Grenzen (`wint_t`, `mbstate_t`, `WINT_MIN/MAX`, `WEOF`), keine Funktionen.
  `wchar_t` bleibt 32 Bit (int) wie beim Linux-gcc: 9fronts `Rune` ist `uint`, `L"…"` im Tree.
  (APEs `stddef.h` sagt `unsigned short wchar_t` — Altlast, nicht angefasst.)
- mpc 1.4 braucht `<complex.h>` → `compat/include/complex.h` (Typ-Makros, creal/cimag/conj über
  Builtins, CMPLX). mpc `radius.c` nutzt `PRIi64` → `compat/include/inttypes.h` ergänzt alle
  C99-PRI/SCN-Makros, die APE fehlen (64 Bit/MAX/PTR = long long).

## Gate Phase 2: 9front-Baum mit dem Cross-gcc

- Worktree des 9front-Repos (Commit `01e5c0aa5` + `plan9.ld`) mit
  `CC=gcc TOOLPREFIX=x86_64-pc-plan9- ARCH=amd64 build.exe` gebaut: **813 files expected, ok**.
  Gegen eine zweite Worktree mit dem Linux-gcc (16.2.1) verglichen (`objcopy --strip-debug
  -R .comment -R .note.gnu.property`): **5032 von 5037 Objekten byte-identisch**.
- Die 5 Abweichungen: `9/pc64/userinit.o`, `pc64.o` binden gelinkte Binaries ein (initcode,
  bootfs.paq), deren `.comment` die Compiler-Version trägt (+ paq-Zeitstempel); `cmd/gs`
  (gs.r.o, gsmisc.o, gsmalloc.o): `malloc_.h` hat `#ifdef linux` — **der Linux-gcc definiert
  `linux`, der Tree-Build nimmt dadurch Linux-Pfade** (latenter Fehler im 9front-Build, vgl.
  MEMORY.md; das plan9-Target definiert es korrekt nicht).
- Ein Baum für Linux-gcc-Vergleiche braucht `../plan9port` neben sich (gen-runetype.sh).

## Native Toolchain (Phase 4)

- Canadian Cross: build=Linux, host=target=x86_64-pc-plan9, `--prefix=/amd64/gnu`,
  `DESTDIR=out/amd64`. `ac_cv_c_bigendian=no` (AC_C_BIGENDIAN findet im Canadian-Build nichts),
  `CXXFLAGS=-DCODY_NETWORKING=0` (libcody nimmt bei `__unix__` Sockets an).
- **C-Default gnu17 statt gnu23** (neues Target-Makro `C_DEFAULT_GNU17`, `c-family/c-opts.cc`):
  APE deklariert Signal-Handler K&R `void (*)()` (bewusst ohne Parametertypen: Handler mit
  Note + Ureg); C23 liest `()` als `(void)`. Explizites `-std=` gewinnt.
- **libcpp-Bug (upstream)**: `_cpp_post_stack_file` nimmt `sysp` als `bool` → aus 2 („C-Header,
  implizit extern "C"") wird 1. Damit wirkte `SYSTEM_IMPLICIT_EXTERN_C` nie (AIX genauso
  betroffen). Fix: `int`. Gefunden mit gdb: Verzeichnis sysp=2, Line-Map sysp=1.
- `SYSTEM_IMPLICIT_EXTERN_C` im plan9.h: APE-Header ohne `extern "C"` (z. B. `sys/time.h`).
- `_BSD_EXTENSION` **nicht** mehr per Default: `bsd.h` hat Nicht-POSIX-Prototypen (getopt,
  strcasecmp ohne const) → Konflikte mit Programmen. BSD-only-Header (`sys/param.h`,
  `sys/select.h`, `select.h`, `sys/ioctl.h`, `sys/uio.h`, `sys/resource.h`, `sys/socket.h`,
  `netdb.h`) schalten es in compat nur für sich ein. `LIB_SPEC "-lapecompat -lbsd -lap"`
  (gcc ignoriert `#pragma lib`).
- compat-Header (Umbenennungs-Trick `#define f __ape_f` um das `#include_next`, dann
  POSIX-Deklaration derselben libap-Funktion): `execv/execve/execvp` (`char *const[]`),
  `strndup` (const), `signal`/`SIG_*`/`struct sigaction` (nur C++ und C23; Layout per
  Array-Größen-Check gegen APEs struct), `ptrdiff_t` (= `__PTRDIFF_TYPE__` long wie
  libstdc++s `std::ptrdiff_t`; APE: long long → „ambiguous“ in C++).
- compat-Header ohne Umbenennung: `stdbool.h` (ersetzt APEs C89-Version), `strings.h`,
  `string.h` (+strings.h, stpcpy, stpncpy), `unistd.h` (getopt, mktemp, symlink, truncate),
  `sys/wait.h` (wait3/wait4 + struct rusage), `sys/stat.h` (fchmod, lstat, S_IF*),
  `stdint.h` (INTn_C, PTR/PTRDIFF/SIG_ATOMIC/WCHAR/WINT-Grenzen), `stddef.h` (offsetof als
  `__builtin_offsetof`, max_align_t), `dlfcn.h`, `stdio.h` (asprintf, vasprintf, popen).
- **libapecompat.a** (compat/lib): asprintf/vasprintf (wachsender Puffer: APEs vsnprintf
  liefert nicht C99s Gesamtlänge), stpcpy, stpncpy, strndup (APE deklariert, implementiert
  nicht), truncate, dlopen & Co. (schlägt immer fehl, dlerror sagt warum — ld ruft dlopen für
  `-plugin` auch ohne Plugin-Support), `readlink` als `__apecompat_readlink` (asm-Label):
  libbsds readlink gibt `int` zurück, ein POSIX-Aufrufer liest `ssize_t` → -1 käme als 2^32-1.
- 9front-Baum (echte LP64-Fehler, gefixt): `sys/include/ape/stdint.h` `SIZE_MAX` war
  `UINT32_MAX` (unter gcc ist size_t 64 Bit) → `__SIZE_MAX__`.
- Native binutils (as, ld, ar, nm, objcopy, objdump, ranlib, strip, readelf, size, …) gebaut.
- `make_relative_prefix` (libiberty) sucht das Programm über `$PATH`; 9fronts rc kennt nur
  `path`, und mit binds liegt `gcc` unter `/bin`, unabhängig vom Installationsort. Der Treiber
  verzichtet auf einem Plan-9-Host (`__plan9__`) auf die Relokation und nimmt die konfigurierten
  Pfade (`/amd64/gnu/...`). Vorher: `programs: =../libexec/...` → „cc1: not found“.
- **libiberty-Bug (upstream)**: `lrealpath` fällt ohne `realpath`/`canonicalize_file_name` ans
  Funktionsende ohne `return` (nur der Windows-Zweig gibt etwas zurück) → Müll; ld meldete
  „input file 'hi.o' is the same as output file“. Fix: `return strdup (filename);` (in gcc und
  binutils), dazu `realpath` in libapecompat (absolut, `.`/`..` bereinigt, existiert; keine
  Symlinks auf 9front).
- libgcc mit `-std=gnu23` (`t-plan9`): sie ist für gccs eigenen Default geschrieben (`bool`).
- `install-strip`: cc1 389 MB → 40 MB, `/amd64/gnu` insgesamt 107 MB.
- **Gate Phase 4** (`tests/native.sh`): im Gast `gcc` → APE-Programm läuft, natives
  Plan-9-Programm (`-nostdinc … -lc -lgcc`) läuft, Objekt byte-gleich mit dem Cross-gcc.

## 386: i686-pc-plan9

- Target-Eintrag `i[34567]86-*-plan9*` (gcc, libgcc, bfd `i386_elf32_vec`, ld `elf_i386`, gas schon
  generisch). `i386/plan9-386.h` vor `plan9.h`: `PLAN9_OBJTYPE "386"`,
  `-mincoming-stack-boundary=2` per Default (9fronts 386-Programme starten mit nur 4-Byte-
  ausgerichtetem Stack; der Tree baut ebenso, `386/cflags.json`), `--with-arch=pentium` wie
  `-march=pentium` im Tree. `DEFAULT_PCC_STRUCT_RETURN` bleibt 1 (= Linux i386).
- libgcc: der Unwinder (`__builtin_eh_return`) verträgt keine Stack-Neuausrichtung → für
  `unwind-dw2.c`/`unwind-sjlj.c` `-mincoming-stack-boundary=4` (`CFLAGS-<datei>` in `t-plan9`).
- compat: `stdint.h`/`inttypes.h` je nach Zeigergröße (`intptr_t` ist auf 386 `int`),
  `PTRDIFF_*` aus `__PTRDIFF_MAX__`; `ctype.h` (`isascii`/`toascii`, in APE nur BSD; gmp).
- `386/plan9.ld` sammelt `.init_array` wie amd64.
- Baustruktur jetzt `build/$objtype/…` (`objtype=386 ./build.sh …`), `out/$objtype`; amd64s
  alter Bau liegt noch in `build/` (Symlinks in `build/amd64`, absolute Pfade in den
  configure-Bäumen).
- Cross, Host-Libs, nativ für 386 gebaut (`/386/gnu` 80 MB).
- Gast (9pc): `objtype=386 tests/cross.sh` (APE, nativ, C++) und `objtype=386 tests/native.sh`
  (APE, nativ, Objekt byte-gleich mit dem Cross-gcc) bestanden.
