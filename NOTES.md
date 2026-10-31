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
  alter Bau in `build/` (Symlinks aus `build/amd64`) ist ersetzt (s. „APE für C++ erweitert“).
- Cross, Host-Libs, nativ für 386 gebaut (`/386/gnu` 80 MB).
- Gast (9pc): `objtype=386 tests/cross.sh` (APE, nativ, C++) und `objtype=386 tests/native.sh`
  (APE, nativ, Objekt byte-gleich mit dem Cross-gcc) bestanden.

## gccmk: mk mit gcc auf 9front (`plan9/`, `build.sh plan9`)

- `gccmk` = mk in einem Namensraum: `/$objtype/mkfile` → `/$objtype/gnu/plan9/mkfile`
  (`CC=gcc-cc LD=gcc-ld AS=gcc-as`, `O` wie bei kencc), `bind -b …/plan9/bin /bin` (`pcc`, `ar`),
  `bind -c /$objtype/lib.gcc /$objtype/lib`. Flags = die des Tree-Builds (`cflags.json`, `gcc.json`,
  `ape.json`, Oflags/Libs; Bibliotheksliste aus `sys/src/cmd/cmds.json` bzw. `ape/cmd`), von
  `build.sh plan9` als eine Zeile je Argument abgelegt (`lib.rc` liest mit `ifs=$nl`).
- Linken wie `build.exe` (`presolve`): `ld -r`, Common-Symbole schwach (`objcopy --weaken-symbols`),
  dann alle Bibliotheken in einer Gruppe; 386 braucht `libgcc.a` (64-Bit-Division) aus
  `cflags.json`s `Libs`.
- `ar`: GNU ar mit `r` (mksyslibs `ar vu` nennt keine Operation) und `s` (Index für GNU ld);
  **Mitglieder heißen `x.o` wie bei build.exe** — sonst stünden `x.6` neben den `x.o` des Builds
  im Archiv und ld nähme womöglich das alte.
- `tests/mk.sh` (amd64 und 386 grün): libbio (`gccmk install`, keine `.$O`-Mitglieder), Objekte
  gleich dem Tree-Build, libc-Assembler über die GAS-Ports, hoc (yacc) und ape/cmd/expr (pcc)
  laufen. Vergleich mit `--strip-unneeded` ohne `.eh_frame`: der Linux-gcc für i386 erzeugt
  Unwind-Tabellen per Default, i686-pc-plan9 nicht (Code und Relokationen gleich).
- Grenzen: Kernel, libc als Ganzes (`9syscall` schreibt 6a-Assembler). libmp/libsec (amd64, 386)
  und `libc/386/387` haben inzwischen GAS-Ports (9front-Repo, `tests/asm`); `tests/mk.sh` baut
  libmp/libsec-Assembler mit gccmk und vergleicht mit dem Tree-Build.

## C++ nativ: g++ auf 9front (amd64, 386)

- Nativer gcc jetzt mit `--enable-languages=c,c++`: `g++`, `cc1plus`, libstdc++/libsupc++ und
  die C++-Header unter `/$objtype/gnu` (amd64 152 MB, 386 143 MB). libstdc++-Optionen für
  Cross und nativ gemeinsam (`LIBSTDCXX_CONF` in build.sh).
- **Exceptions gingen nirgends** (auch nicht mit dem Cross-g++, `tests/cxx.cc` mied sie): `throw`
  → `std::terminate` → „abort“. Ursache: keine crt-Dateien, also registrierte niemand `.eh_frame`
  beim Unwinder, und ohne `dl_iterate_phdr` gibt es keinen anderen Weg zu den FDEs. Fix wie auf
  anderen ELF-Systemen ohne `PT_GNU_EH_FRAME`: libgcc baut `crtbegin.o`/`crtend.o` (`extra_parts`,
  `i386/t-crtstuff`, kein PIC), `STARTFILE_SPEC`/`ENDFILE_SPEC` nennen sie (nicht bei
  `-nostdlib`/`-nostartfiles`, also nicht im Tree-Build). `frame_dummy` steht in `.init_array`,
  `__wrap_main` ruft es; `__register_frame_info` ist dort schwach → C-Programme ziehen den
  Unwinder nicht herein. crtstuff braucht wie libgcc2 `-std=gnu23` (`CRTSTUFF_T_CFLAGS`).
- 9front-Repo: `plan9.ld` (amd64, 386) sammelt `.gcc_except_table*` in einer Sektion im
  Textsegment (vorher Orphans). C-Programme linken damit byte-gleich wie vorher.
- **Canadian-Falle:** die Target-Bibliotheken baut der installierte Cross-g++; libstdc++s configure
  sah damit die *installierten* libstdc++-Header — deren `<fenv.h>`-Wrapper ist leer (APE hat kein
  fenv.h) → „fenv.h usability… yes“, `HAVE_FENV_H`, Abbruch in `floating_to_chars.cc`. Der
  Cross-Build nimmt `xgcc -nostdinc++`; nativ jetzt ebenso: `CXX_FOR_TARGET="$TARGET-g++ -nostdinc++"`.
- build.sh: `conf` merkt sich die configure-Argumente (`$B/<dir>.args`) und konfiguriert bei
  Änderung neu (vorher blieb ein alter Baum stehen); fehlt die Datei, gilt der Baum als aktuell.
- Tests: `tests/eh.cc` (throw über Rahmen, rethrow, Destruktoren beim Unwinden, `vector::at`).
  `tests/cross.sh` (ape, plan9, cxx, eh) und `tests/native.sh` (+ cxx, eh mit dem nativen g++,
  `samecxx`: Objekt von eh.cc byte-gleich mit dem Cross-g++) für amd64 und 386 grün.

## APE für C++ erweitert (compat, musl)

Ziel: libstdc++ mit allem, was es über C99/POSIX findet. Entscheidung: alles in der Toolchain
(`compat/`, `libapecompat.a`), 9fronts APE bleibt — bis auf echte Port-Fehler (malloc, s. u.).

- **musl 1.2.5** als Quelle (`sources.txt` mit Schlüsselspalte; `keys/musl.pub`, Fingerprint
  8364 8929 0BB6 B70F 99FF DA05 56BC DB59 3020 450F; `fetch.sh` prüft `.asc`). `build.sh musl`
  baut mit musls eigenem Makefile/Headern (`$B/musl`) und legt Objekte nach `compat-lib/musl-*.o`:
  - libm, fenv, complex (Arch-Dateien ersetzen generische wie in musls Makefile).
  - Wide-String-Funktionen (`wcs*`, `wcp*`, `wmem*`) und `<wctype.h>` (`isw*`, `tow*`, `wcwidth`):
    ohne Locale, Unicode-Tabellen.
  - Objekte, deren starke Symbole alle libap hat (C89-double-Mathe, j0, erf …), entfallen; in den
    übrigen werden libap-Namen lokal (`objcopy --localize-symbol`): i386 `scalbn.s` ist auch
    `ldexp`, `exp10.c` hat ein schwaches `pow10(double)` (APE: `pow10(int)`!).
  - **Engine** (`musl-engine.o`, `ld -r`): musls vfprintf/vfscanf/strtod/vfwprintf/vfwscanf/
    vswprintf/vswscanf/wcsto* + `compat/musl/bridge.c` (in musls Umgebung übersetzt); alle Symbole
    lokal außer `__apecompat_m*` und `wcsto*`. Bridge-FILE = musls String-FILE mit Write-Callback
    auf APEs `fwrite` und Read-Callback, der byteweise APEs `getc` liest (1-Byte-Puffer wie
    `__stdio_read`: musls `ungetwc`-Makro und `shgetc` setzen `rpos` zurück und erwarten das Byte
    im Puffer). Was scanf zurückgibt, geht per `ungetc` an APEs FILE. musls errno landet in einer
    eigenen Variablen (`___errno_location`) und wird in APEs Nummern übersetzt (musl ENOMEM=12 ist
    APEs EINVAL!). `__lockfile` wird nie gerufen (lock -1). Wide-Funktionen (fputwc, getwc,
    ungetwc) in der Engine eigene: musls fragen die Locale über den Thread-Pointer (`%fs`) ab.
- **printf/scanf**: APEs liest jedes Gleitkomma-Argument als double (`%Lf` gab Müll), kennt `%a`,
  `%F`, `hh`, `j`, `t` nicht, `snprintf` liefert nicht C99s Länge. compat `stdio.h`: APEs
  Deklarationen werden umbenannt, die eigenen tragen Link-Namen `__apecompat_*` (Asm-Labels) — nur
  wo libap denselben Namen hat; sonst normale Namen (configures Link-Tests deklarieren selbst:
  `strtold` mit Label wurde nicht gefunden).
- **x87/SSE beim Start** (libgcc `__wrap_main`): 9fronts Kernel gibt CW 0x0232 und MXCSR 0x1900 —
  Division durch null, Überlauf, ungültig trappen („sys: fp: division by zero“), und der x87
  rechnet auf 53 Bit (long double = double!). Jetzt CW 0x037f, MXCSR 0x1f80 (amd64; 386 hat
  maskierte SSE-Ausnahmen vom Kernel, ein Pentium kein MXCSR), wie jedes POSIX-x86. Nur für vom
  Treiber gelinkte Programme; `-nostdlib` (Tree) behält die des Kernels.
- **`<float.h>`**: APEs (`/$objtype/include/ape/float.h`) beschreibt kencc: `LDBL_*` = double,
  `LDBL_MAX` eine double-Konstante. compat übernimmt gccs Werte (+ C99/C11-Makros), APEs Rest
  (FPdbleword, FP*-Bits) bleibt.
- **`wchar_t`**: gccs 32 Bit; compat `stddef.h` definiert `_WCHAR_T` vor APEs Header (der sonst
  `unsigned short` typedeft). APEs `mbtowc` & Co. schreiben 16 Bit → unter Labels ersetzt
  (`compat/lib/multibyte.c`, striktes UTF-8: Überlangformen, Surrogate, > 0x10FFFF = EILSEQ).
  Wide-I/O (`wstdio.c`): ungetwc gibt die Bytes per `ungetc` in APEs Puffer zurück, sonst
  (ungepuffert, Puffergrenze) in eine Tabelle, solange fd und Puffer des FILE gleich sind.
- **`intmax_t`** = `__INTMAX_TYPE__` (long auf amd64; APE: long long): sonst `-Wformat` bei `%jd`
  und ein anderer Typ als `std::intmax_t`. `int64_t` bleibt APEs long long (APEs Prototypen mit
  `long long *`, gcc ≥ 14 bricht bei inkompatiblen Zeigern ab).
- Zeit: `clock_gettime` über `/dev/bintime` (4. Feld = ns seit Boot, monoton, `tod.c`), dauerhaft
  offen, `pread` (neu in compat: libaps `_PREAD` + `_syserrno`). `usleep`, `sched_yield`
  (sleep(0)), `sysconf` mit Prozessorzahl (Zeilen von `/dev/sysstat`) und Seitengröße.
  `unistd.h` meldet `_POSIX_TIMERS`, `_POSIX_MONOTONIC_CLOCK`, `_POSIX_THREADS`,
  `_POSIX_TIMEOUTS` … (libstdc++ prüft danach steady_clock und timed_mutex).
- **pthreads** (`compat/lib/pthread.c`, ein Objekt: `pthread_cancel` ist gthrs Proxy für
  „Threads aktiv“): Thread = Proc, `rfork(RFPROC|RFMEM|RFNOWAIT)`. Ein Proc hat sein eigenes
  Stacksegment (Kopie); darauf liegen APEs errno (`_callmain`s lokale Variable, `_errnoloc`
  zeigt hin) und `_privates` → beide pro Thread ohne weiteres; `_privates[0]` = Thread. Threads
  laufen auf Stacks aus malloc (geteilter Speicher), `main` auch (`__plan9_mainstack`, von
  libgccs `__wrap_main` gerufen, wenn gelinkt; 16 MiB wie USTKSIZE) — sonst sähen andere Threads
  mains Locals nicht (Lambdas mit `[&]`). **Falle**: nach `rfork(RFMEM)` steht das Kind auf dem
  (geteilten) Heap-Stack des Elternteils → beide laufen auf demselben Stack (Absturz mit pc im
  Heap). `__apecompat_rforkthread` (Asm) macht den Syscall selbst und setzt im Kind sofort SP auf
  `privtop` (unter `__plan9_mainstack`s Rahmen, in jeder Kopie frei); Werte in Registern, die das
  Kind bekommt (amd64 `forkret`: nur R14/R15, Rest null; 386: alle außer AX). Dort läuft der
  Thread per `runon` auf seinem Heap-Stack und kehrt zum Aufräumen zurück (wer den Stack freigibt,
  gibt einen frei, auf dem niemand läuft). Warten über 9front-Semaphoren (32-Bit, `s32int`):
  Mutex nach Drepper (3 Zustände), Condition mit Warteschlange und einer Semaphore je Thread,
  rwlock (Schreiber zuerst), once, Keys mit Sequenznummern. `exit` beendet die anderen Threads
  (`/proc/n/ctl` „kill“). Kein `pthread_cancel`, keine prozessübergreifenden Objekte.
- gcc: `--enable-threads=posix` (gthr-posix, emutls für `thread_local`), libstdc++ ohne
  `--disable-wchar_t`/`--disable-libstdcxx-threads`, mit `--enable-libstdcxx-time=yes`;
  Treiber nimmt `-pthread` an. libstdc++ findet jetzt: C99 math/stdio/stdlib/wchar/complex/fenv/
  inttypes, CLOCK_MONOTONIC, nanosleep, sched_yield, rwlock, timedlock, nprocs.
- **9front-Baum (echter Port-Fehler)**: APEs malloc lieferte nur 8-Byte-ausgerichtete Blöcke
  (Schrittweite `&~7`, sbrk 4-ausgerichtet), gcc/g++ nehmen 16 an (`max_align_t`,
  `__STDCPP_DEFAULT_NEW_ALIGNMENT__`, ausgerichtete SSE-Zugriffe). Fix in
  `sys/src/ape/lib/ap/plan9/malloc.c` (16er-Schritt, `sbrk16`).
- **amd64-Bauverzeichnisse**: die alten Symlinks `build/amd64/* -> ../*` (Layout vor 386) machten
  `build/amd64/cross` zum Symlink auf `build/cross`; ein neu konfigurierter gcc mit Präfix
  `build/amd64/cross` fand sich in `build/cross/bin` und hielt sich für verschoben → `incpath.cc`
  stellt dann alles unter `lib/gcc/…` an den Anfang, gccs `stddef.h` vor APEs (Fehler
  `max_align_t`). amd64 jetzt in echten Verzeichnissen neu gebaut wie 386.
- Tests: `tests/c99math.c` (`-frounding-math`: sonst schiebt gcc `rint` über `fesetround`),
  `c99stdio.c`, `c99time.c`, `c99wchar.c`, `pthread.c`, `cxxlib.cc` (in `cross.sh`; `cxxlib` auch
  nativ). Testfallen: gcc faltet `rint(-x)` zu `-rint(x)`; auf 386 inlinet gcc `sqrtf` als
  `fsqrt` mit Überpräzision; glibcs „C“-Locale ist kein UTF-8 (Hosttest: `C.UTF-8`).
- Offen: APEs stdio hat keine Locks (Threads, die ein FILE teilen, müssen sich abstimmen;
  `std::cout` aus mehreren Threads); `_exit`/abort beenden andere Threads nicht; keine Locales
  außer „C“; `<uchar.h>` fehlt.

## riscv64: riscv64-unknown-plan9 (Branch riscv, Tree `../9front-riscv`)

- **gcc-Target aufgeteilt wie bei anderen Systemen:** `gcc/config/plan9.h` hat das System
  (Include-Pfade, Feature-Makros, Bibliotheken, crtbegin/crtend, `--wrap=main`, gnu17,
  `SYSTEM_IMPLICIT_EXTERN_C`), `i386/plan9.h` bzw. `riscv/plan9.h` die Maschine
  (`PLAN9_OBJTYPE`, `PLAN9_CC1_SPEC`, `PLAN9_LINK_SPEC`; x86: `.note.gnu.property`).
  libgcc: `config/plan9-main.c` und `config/t-plan9` gemeinsam, `i386/t-plan9` nur noch die
  Unwinder-Ausrichtung. **Für x86 ohne Wirkung:** amd64 und 386 neu gebaut, `-dumpspecs`,
  C- und C++-Makros, `ape.o`/`eh.o` byte-gleich mit vorher; `tests/cross.sh` 10/10 je.
  Falle: nach geänderter `config.host` konfiguriert `build.sh` libgcc nicht neu (nur bei
  anderen configure-Argumenten) → `build/$objtype/cross-gcc/$TARGET/libgcc` löschen.
- **riscv64-Target:** `config.gcc` `riscv*-*-plan9*` (`elfos.h glibc-stdint.h … riscv/plan9.h
  plan9.h`, `riscv/t-riscv`, Unwind-Tabellen per Default wie riscv64-linux-gnu); configure
  `--with-arch=rv64gc --with-abi=lp64d --with-cmodel=medany` wie `riscv64/cflags.json`.
  `-mno-relax`/`--no-relax` per Default: niemand setzt gp (`__global_pointer$`) — weder
  9fronts `_main` noch der Kernel, jc nimmt das Register als SB. `char` vorzeichenbehaftet
  (`DEFAULT_SIGNED_CHAR`, wie jc und `-fsigned-char` im Tree). libgcc wie `riscv*-*-*`
  (softfp, save-restore) mit crtbegin/crtend. binutils: nichts (die generischen
  `riscv64*-*-*`-Einträge in config.bfd, ld, gas greifen).
- compat: `fenv.h` mit fcsr-Werten (musls riscv64 bits), `pthread.c` `rfork`/`runon` in
  riscv64-Assembler (Nummer und Ergebnis in s0, Argument bei 8(sp), `ecall`; das Kind erbt
  alle Register außer s0: privtop/t in s1/s2). `string.h` deklariert `strtok_r` (libap hat es,
  APE nur unter `_REENTRANT_SOURCE` — das brächte APEs Nicht-POSIX-`strerror_r`; gas'
  `tc-riscv.c` braucht es). gccmk: `lib.rc` kennt riscv64 (O=j); `tests/mk.sh` nimmt das
  `objcopy` der Cross-Toolchain (das des Hosts kennt riscv64 nicht), vergleicht memmove,
  strcmp, setjmp; libmp/libsec haben für riscv64 keinen Assembler (mpsec entfällt).
- Tree (`../9front-riscv`): `riscv64/plan9.ld` sammelt `.init_array` und
  `.gcc_except_table` wie amd64 (C-Programme linken byte-gleich).
- **`long double` ist IEEE quad** (128 Bit, Software über libgcc). Tests hatten x87-Werte
  fest (`%.18Lf` von π, LDBL_MAX-Zeichenkette) → je nach `LDBL_MANT_DIG`. **musl hat für quad
  kein echtes `expl`, `logl`, `log1pl`** (`TODO: broken implementation to make things
  compile`, sie rufen die double-Funktion; `atanhl` darüber) — wie bei musl auf riscv64-Linux;
  `c99math` prüft dort double-Genauigkeit. Offen.
- Gates: `tests/cross.sh` 10/10, `tests/native.sh` 7/7 (gcc/g++ auf 9front riscv64, Objekte
  byte-gleich mit dem Cross-Compiler), `tests/mk.sh` 5/5 (riscv64-Gast unter TCG: `MEM=2048
  CMDWAIT=1800`). Tree mit `TOOLPREFIX=riscv64-unknown-plan9-` gebaut: 817 files ok,
  **5811 von 5830 Objekten byte-gleich** mit dem Linux-Cross-Build (riscv64-linux-gnu-gcc
  16.2.1); die 19 anderen: cwfs `conf.o`/`.r.o` (`-DDATE=$(date +%s)`), `virt.o` (bettet
  bootfs.paq mit Compiler-Version und Zeitstempeln ein).
- `/riscv64/gnu` 194 MB. Offen: Selbstbau des Trees im riscv64-Gast (unter TCG sehr lang),
  keine riscv64-ISO.

## arm64: aarch64-pc-plan9 (2026-10-31, für 9front-rust)

- binutils: `aarch64-*-plan9*` in config.bfd, gas/configure.tgt, ld/configure.tgt (die
  ELF-Einträge, Emulation aarch64elf).
- gcc: `config.gcc` `aarch64*-*-plan9*` (`elfos.h glibc-stdint.h aarch64-elf.h aarch64-errata.h
  aarch64/plan9.h plan9.h`, `aarch64/t-aarch64`, Unwind-Tabellen per Default wie
  aarch64-linux-gnu). `aarch64/plan9.h`: `PLAN9_OBJTYPE "arm64"`, `char` vorzeichenbehaftet
  (7c, `-fsigned-char` des Trees), `-mno-outline-atomics` per Default (deren Helfer wählen LSE
  über getauxval, das Plan 9 nicht hat; der Tree baut so). libgcc: aarch64-t-aarch64, softfp
  (`long double` ist quad), crtfm, **`aarch64/t-crtstuff`** (neu, wie riscv: crt*.o ohne
  Unwind-Tabellen, sonst zeigt `__EH_FRAME_BEGIN__` auf crtbegins eigene .eh_frame und der
  Unwinder findet die des Programms nicht: C++-Exceptions endeten in terminate), kein t-lse.
  `build.sh`/Tests: `objtype=arm64`, gccmk O=7.
- Tree (gcc-Branch): `arm64/plan9.ld` sammelt `.init_array` und `.gcc_except_table` wie amd64
  (ohne lief `frame_dummy` nie); C-Programme bleiben byte-gleich. `/sys/include/ape/fenv.h` hatte
  für arm64 die x87-Werte (FE_UPWARD 0x800 statt fpcrs 0x400000): fesetround wies die
  Rundungsmodi ab.
- `tests/cross.sh` arm64 10/10 (Gast unter TCG, ein Prozessor).
- **nativ** (`build.sh hostlibs native plan9`, `/arm64/gnu` 148 MB): gcc auf einem aarch64-Host
  bindet `host_detect_local_cpu` ein (`-mcpu=native`), `driver-aarch64.o` aber nur für bekannte
  Hosts: `gcc/config.host` nimmt `aarch64*-*-plan9*` dazu (es liest `/proc/cpuinfo`; fehlt die,
  erkennt `-mcpu=native` nichts). Linker für rustc auf 9front arm64 (9front-rust).

## Lücken aus dem Mesa-Port (Branch `gpu` des Trees, „9front next generation“)

Beim Cross-Build von Mesas venus (`../9front`, Branch `gpu`, `scripts/mesa`) fielen
Lücken auf, die bisher im Mesa-Build umgangen wurden. Behoben an der Wurzel; im Tree
darf auf dem Branch `gpu` 9front-Code geändert werden.
- **`-pthread`**: Die Specs schluckten es (`%{pthread:}`), der Treiber kannte die Option
  aber nicht („unrecognized command-line option“). `config/plan9.opt` (`pthread` als
  `Driver`-Option, dazu `plan9.opt.urls`, die GCC 16 verlangt), `config.gcc`
  `extra_options` für `*-*-plan9*`.
- **Link-Reihenfolge**: `LIB_SPEC` legt `-lapecompat -lbsd -lap` in eine Gruppe
  (`--start-group`): libap ruft libbsd (mkstemp → mktemp) und umgekehrt; unter 9front
  löst `#pragma lib` das ohne Reihenfolge.
- **compat**: `readlink`/`symlink` nur noch, wenn APE sie nicht POSIX-gerecht deklariert
  (APEs `unistd.h` definiert dann `__APE_POSIX_LINKS`; libbsds sind dann POSIX'
  Signaturen). `sysconf(_SC_PHYS_PAGES)` aus `/dev/swap`. pthread-Barrieren
  (`pthread_barrier_*`), `pthread_sigmask` (APEs `sigprocmask`), `pthread_getcpuclockid`
  (ENOENT: keine CPU-Zeit-Uhren).
- Tree (Branch `gpu`), APE: `int8_t` ist `signed char` (gcc: `char*` und `signed char*`
  sind verschiedene Typen; gccs eigenes `__INT8_TYPE__` ist ohnehin `signed char`);
  `posix_memalign`, `aligned_alloc`, `memalign` in `malloc.c` (der ausgerichtete Block
  liegt in einem von malloc, davor ein Kopf mit Magic `ALIGNED`, dem `free`/`realloc`
  folgen) — vorher meldete meson `posix_memalign` als vorhanden (gcc kennt das Builtin),
  APE hatte es nicht; `mkstemp`/`mkstemps` ohne libbsds `mktemp`, unter `_POSIX_SOURCE`
  deklariert; POSIX-`lstat`, `symlink`, `readlink` (libbsd); `static_assert` (C11) in
  `assert.h`; `SIGSYS`; `setenv`/`unsetenv`; `localtime_r` & Co. unter `_POSIX_SOURCE`;
  `putenv` schrieb einen Eintrag hinter das vergrößerte `environ` (n+1 statt n+2).

## compat in APE (2026-10-29)

compat (`compat/`, `libapecompat.a`, musl als Quelle) ist in den 9front-Baum gewandert
(Branch `gcc`): APEs Header haben die gcc-Teile unter `#if defined(__GNUC__)`, die
Funktionen liegen in `sys/src/ape/lib/ap/gnu`, musls Teilmenge in `sys/src/ape/lib/musl`,
alles in `libap.a`. Der Treiber linkt `--start-group -lbsd -lap --end-group`, kein
Tool-Include-Verzeichnis mehr; `build.sh compat` und musl in `sources.txt` sind weg.
Braucht einen Baum mit diesem APE (`PLAN9`; `gpu` muss `gcc` mergen, `../9front-riscv`
ebenso). `tests/cross.sh` amd64 gegen den gcc-Worktree: alle 10 grün.

## 386: intptr_t wie size_t (2026-10-30)

i686-pc-plan9 hatte `size_t`/`ptrdiff_t` `(unsigned) long` (gccs Standard, wie Plan 9s
`usize`), aber `intptr_t`/`uintptr_t` `(unsigned) int` (glibc-stdint.h für 32 Bit): zwei
Typen gleicher Größe, gcc 14 lehnt einen `uintptr_t *` für einen `size_t *` ab (curls
`rustls.c` gegen rustls-ffi, beim Bau von cargo für 386 in 9front-rust). `i386/plan9-386.h`:
`INTPTR_TYPE`/`UINTPTR_TYPE` `long`, wie size_t und u.h's `uintptr`; keine ABI-Änderung.
APEs `inttypes.h` (Branch gcc) gibt PRI*PTR dafür `l` (unter `__plan9__`). Toolchain im
eigenen Bauverzeichnis (`BUILD=build-gcc`, gegen ../9front-gccwt); `tests/cross.sh` 386: 10/10.

## Threads: GTHREAD_USE_WEAK 0 (2026-10-30)

libgccs gthr-posix prüft über einen schwachen Verweis auf `pthread_cancel`, ob ein Programm
Threads hat (`__gthread_active_p`). Ein schwacher Verweis zieht aber kein Objekt aus
`libap.a`: in einem Rust-Programm (eigene rfork-Procs, kein APE-pthread) war er 0, die
Mutexe des Unwinders (FDE-Listen, unwind-dw2-fde) taten nichts. Gleichzeitige Panics in
mehreren Threads (Rusts std-Tests mit 12 Test-Threads) brachen Phase 2 ab (`abort`), das
Programm hing. `gcc/config/plan9.h`: `GTHREAD_USE_WEAK 0` wie darwin.h und cygwin.h; APEs
pthreads sind fester Teil von libap, ihre Mutexe gehen für jeden Proc (`pthread_self` kennt
std's Procs). libstdc++ (`config/os/generic`) hatte `_GLIBCXX_GTHREAD_USE_WEAK 0` schon.
