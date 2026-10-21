#!/bin/bash
# Build the gcc toolchain for 9front in stages (README.md):
#
#   cross     binutils and gcc (C, C++) on Linux for the target,
#             with libgcc and libstdc++ over APE   -> build/$objtype/cross
#   compat    the toolchain's additions to APE (compat/) into build/$objtype/cross
#   hostlibs  gmp, mpfr, mpc for 9front            -> build/$objtype/hostlibs
#   native    binutils and gcc that run on 9front  -> out/$objtype
#   plan9     gccmk: mk with gcc on 9front (plan9/) -> out/$objtype
#
# objtype=amd64 (default): x86_64-pc-plan9; objtype=386: i686-pc-plan9;
# objtype=riscv64: riscv64-unknown-plan9.
# usage: [objtype=386] build.sh stage...    (PLAN9: the 9front tree, default
#        ../9front; for riscv64 ../9front-riscv, the branch with the port)
set -e
TOP=$(cd "$(dirname "$0")" && pwd)
export objtype=${objtype:-amd64}
case $objtype in
amd64)	TARGET=x86_64-pc-plan9 ARCHCONF=() ;;
# the tree's 386 code: -march=pentium (386/cflags.json)
386)	TARGET=i686-pc-plan9 ARCHCONF=(--with-arch=pentium) ;;
# the tree's riscv64 code (riscv64/cflags.json)
riscv64)	TARGET=riscv64-unknown-plan9
	ARCHCONF=(--with-arch=rv64gc --with-abi=lp64d --with-cmodel=medany)
	[ -n "$PLAN9" ] || [ ! -d "$TOP/../9front-riscv" ] || PLAN9=$TOP/../9front-riscv ;;
*)	echo "build.sh: objtype=$objtype: no target" >&2; exit 1 ;;
esac
export PLAN9=${PLAN9:-$(cd "$TOP/../9front" && pwd)}
PLAN9=$(cd "$PLAN9" && pwd)
SRC=$TOP/src
B=$TOP/build/$objtype
SYSROOT=$B/sysroot
CROSS=$B/cross
J=${J:-$(nproc)}
export PATH=$CROSS/bin:$PATH

# common to every gcc we build: what 9front has and does not have
GCC_CONF=(
	--target=$TARGET
	--with-native-system-header-dir=/sys/include/ape
	--disable-shared --enable-threads=posix --disable-tls --disable-nls
	--disable-lto --disable-plugin --disable-multilib --disable-fixincludes
	--without-isl --without-zstd --disable-libssp --disable-libquadmath
	--disable-libgomp --disable-libatomic --disable-libsanitizer
	--disable-libitm --disable-libvtv --disable-gcov --disable-libcc1
	--disable-bootstrap --enable-initfini-array
	"${ARCHCONF[@]}"
)
# libstdc++ over APE and compat (cross and native): threads are
# compat's pthreads (gcc --enable-threads=posix; thread_local by emutls,
# --disable-tls), only the "C" locale (generic); the clocks
# (steady_clock), nanosleep, sched_yield of compat checked by linking
# (=yes; auto tries only on known systems)
LIBSTDCXX_CONF=(
	--enable-clocale=generic --disable-libstdcxx-pch
	--enable-libstdcxx-time=yes
	--disable-libstdcxx-filesystem-ts
	--disable-libstdcxx-backtrace --disable-libstdcxx-verbose
)
BINUTILS_CONF=(
	--target=$TARGET
	--disable-nls --disable-werror --disable-gdb --disable-gdbserver
	--disable-sim --disable-gprofng --disable-gprof --disable-libdecnumber
	--disable-readline --disable-plugins --disable-shared --without-zstd
	--without-debuginfod
)

log(){ echo "== $*"; }
# compat dir: the toolchain's additions to APE -- headers (compat/include)
# and libapecompat.a (compat/lib, built with the cross compiler) -- into
# the target directory $dir ($prefix/x86_64-pc-plan9)
compat(){
	local d=$1 o=$B/compat-lib f
	mkdir -p "$d/include" "$d/lib" "$o"
	cp -r "$TOP"/compat/include/. "$d/include/"
	rm -f "$o"/*.o
	for f in "$TOP"/compat/lib/*.c; do
		$TARGET-gcc -O2 -g -std=gnu17 -c -o "$o/$(basename "${f%.c}").o" "$f"
	done
	musl "$o"
	rm -f "$d/lib/libapecompat.a"
	$TARGET-ar rcs "$d/lib/libapecompat.a" "$o"/*.o
}
# musl o: musl's libm, fenv and complex (C99's float, long double and
# new functions, <fenv.h>, <complex.h>) as objects musl-*.o into o, built by musl's own
# makefile in its own headers (they need nothing outside themselves).
# Leaves out the objects whose functions libap has (C89's double ones,
# j0, erf, ...): APE programs keep APE's, and there are no duplicates.
musl(){
	local o=$1 m=$B/musl a f b dir
	case $objtype in amd64) a=x86_64 ;; 386) a=i386 ;; riscv64) a=riscv64 ;; esac
	if [ ! -f "$m/config.mak" ]; then
		mkdir -p "$m"
		(cd "$m" && "$SRC/musl/configure" --target=$a CC=$TARGET-gcc --disable-shared) \
			> "$m.configure.log" 2>&1 || { tail -20 "$m.configure.log"; exit 1; }
	fi
	# musl's makefile: a file in src/<dir>/<arch>/ replaces the generic one
	local objs=()
	for dir in src/math src/fenv src/complex; do
		for f in "$SRC/musl/$dir"/*.c; do
			b=$(basename "${f%.c}")
			if ls "$SRC/musl/$dir/$a/$b".[csS] >/dev/null 2>&1; then
				objs+=("obj/$dir/$a/$b.o")
			else
				objs+=("obj/$dir/$b.o")
			fi
		done
		for f in "$SRC/musl/$dir/$a"/*.[csS]; do
			[ -e "$f" ] || continue	# no $a directory (complex)
			b=$(basename "${f%.*}")
			[ -e "$SRC/musl/$dir/$b.c" ] || objs+=("obj/$dir/$a/$b.o")
		done
	done
	# the wide string functions and <wctype.h> (musl's Unicode tables,
	# no locale), not the rest of their directories (libap has it)
	for f in "$SRC"/musl/src/string/{wcs,wcp,wmem}*.c "$SRC"/musl/src/ctype/{isw,tow,wc}*.c; do
		b=${f#$SRC/musl/}
		objs+=("obj/${b%.c}.o")
	done
	(cd "$m" && make -j"$J" "${objs[@]}") > "$m.make.log" 2>&1 ||
		{ tail -20 "$m.make.log"; echo "musl failed: $m.make.log" >&2; exit 1; }
	# libap's functions; an object goes if all its strong symbols are
	# there (or it has none: musl's empty "see floorl.s" files); in the
	# others libap's names become local (i386 scalbn.s is ldexp too,
	# exp10.c has a weak pow10(double), APE's is pow10(int))
	$TARGET-nm "$PLAN9/$objtype/lib.gcc/ape/libap.a" 2>/dev/null |
		awk '$2 ~ /^[TDRBC]$/ {print $3}' | sort -u > "$m/libap.syms"
	rm -f "$o"/musl-*.o
	for f in "${objs[@]}"; do
		$TARGET-nm "$m/$f" 2>/dev/null | awk '$2 ~ /^[TDRB]$/ {print $3}' > "$m/obj.syms"
		[ -z "$(grep -vxF -f "$m/libap.syms" "$m/obj.syms")" ] && continue
		b=${f#obj/src/}
		$TARGET-objcopy $($TARGET-nm "$m/$f" | awk '$2 ~ /^[TDRBVW]$/ {print $3}' |
			grep -xF -f "$m/libap.syms" | sed 's/^/--localize-symbol=/') \
			"$m/$f" "$o/musl-${b//\//-}"
	done
	musl_engine "$o"
}
# musl_engine o: musl's printf, scanf and strto[fdl], their wide
# versions and wcsto*, with the bridge to APE's FILE (compat/musl/
# bridge.c) as one object, musl-engine.o: linked with ld -r, its entry
# points renamed __apecompat_m* (wcsto* keep their names), all else
# local. What it needs from outside: libap (memcpy, malloc, ...), musl's
# libm and wide functions above, compat's multibyte functions (mbtowc,
# wctomb under their link names) and APE's FILE (compat/lib/stdio.c,
# wstdio.c).
musl_engine(){
	local o=$1 m=$B/musl f
	local objs=(obj/src/stdio/vfprintf.o obj/src/stdio/vsnprintf.o
		obj/src/stdio/vfscanf.o obj/src/stdio/vsscanf.o
		obj/src/stdio/fprintf.o obj/src/stdio/fscanf.o
		obj/src/stdio/snprintf.o
		obj/src/stdio/vfwprintf.o obj/src/stdio/vswprintf.o
		obj/src/stdio/vfwscanf.o obj/src/stdio/vswscanf.o
		obj/src/stdio/__towrite.o obj/src/stdio/__toread.o
		obj/src/stdio/__uflow.o obj/src/stdio/fwrite.o
		obj/src/internal/floatscan.o obj/src/internal/intscan.o
		obj/src/internal/shgetc.o obj/src/stdlib/strtod.o
		obj/src/stdlib/wcstod.o obj/src/stdlib/wcstol.o)
	(cd "$m" && make -j"$J" "${objs[@]}") >> "$m.make.log" 2>&1 ||
		{ tail -20 "$m.make.log"; echo "musl failed: $m.make.log" >&2; exit 1; }
	# musl's own flags and headers (CFLAGS_ALL of its makefile; relative
	# to its build directory)
	f=$(cd "$m" && printf 'print-cflags:\n\t@echo $(CFLAGS_ALL)\n' |
		make -s -f Makefile -f - print-cflags)
	(cd "$m" && $TARGET-gcc $f -c -o bridge.o "$TOP/compat/musl/bridge.c")
	$TARGET-ld -r -o "$m/engine.o" "${objs[@]/#/$m/}" "$m/bridge.o"
	$TARGET-objcopy \
		--redefine-sym vfprintf=__apecompat_mvfprintf_ \
		--redefine-sym vsnprintf=__apecompat_mvsnprintf \
		--redefine-sym vsscanf=__apecompat_mvsscanf \
		--redefine-sym strtof=__apecompat_mstrtof \
		--redefine-sym strtold=__apecompat_mstrtold \
		--redefine-sym vswprintf=__apecompat_mvswprintf \
		--redefine-sym vswscanf=__apecompat_mvswscanf \
		--redefine-sym wctomb=__apecompat_wctomb \
		--redefine-sym mbtowc=__apecompat_mbtowc \
		"$m/engine.o" "$m/engine.r.o"
	$TARGET-objcopy \
		--keep-global-symbol=__apecompat_merrno \
		--keep-global-symbol=__apecompat_mvfprintf \
		--keep-global-symbol=__apecompat_mvfscanf \
		--keep-global-symbol=__apecompat_mvsnprintf \
		--keep-global-symbol=__apecompat_mvsscanf \
		--keep-global-symbol=__apecompat_mstrtof \
		--keep-global-symbol=__apecompat_mstrtold \
		--keep-global-symbol=__apecompat_mvfwprintf \
		--keep-global-symbol=__apecompat_mvfwscanf \
		--keep-global-symbol=__apecompat_mvswprintf \
		--keep-global-symbol=__apecompat_mvswscanf \
		$(printf -- '--keep-global-symbol=%s ' wcstof wcstod wcstold \
			wcstol wcstoul wcstoll wcstoull wcstoimax wcstoumax) \
		"$m/engine.r.o" "$o/musl-engine.o"
}
# conf dir srcdir args...: configure in $B/dir, again (from scratch)
# when the arguments differ from the last time's ($d.args; a tree
# configured before there were such files is taken as it is)
conf(){
	local d=$B/$1 s=$2; shift 2
	[ -f "$d/Makefile" ] && [ ! -f "$d.args" ] && printf '%s\n' "$@" > "$d.args"
	if [ -f "$d/Makefile" ] && [ "$(printf '%s\n' "$@")" != "$(cat "$d.args")" ]; then
		log "${d##*/}: configure arguments changed, reconfiguring"
		rm -rf "$d"
	fi
	mkdir -p "$d"
	if [ ! -f "$d/Makefile" ]; then
		printf '%s\n' "$@" > "$d.args"
		(cd "$d" && "$s/configure" "$@") > "$d.configure.log" 2>&1 ||
			{ tail -30 "$d.configure.log"; echo "configure $d failed: $d.configure.log" >&2; exit 1; }
	fi
}
# mk dir log targets...
mk(){
	local d=$B/$1 l=$B/$1.$2.log; shift 2
	(cd "$d" && make -j"$J" "$@") > "$l" 2>&1 ||
		{ tail -40 "$l"; echo "make $* in $d failed: $l" >&2; exit 1; }
}

cross(){
	bash "$TOP/sysroot.sh" "$SYSROOT"
	log "cross binutils"
	conf cross-binutils "$SRC/binutils" --prefix="$CROSS" --with-sysroot="$SYSROOT" "${BINUTILS_CONF[@]}"
	mk cross-binutils all all
	mk cross-binutils install install
	log "cross gcc"
	conf cross-gcc "$SRC/gcc" --prefix="$CROSS" --with-sysroot="$SYSROOT" \
		--enable-languages=c,c++ "${GCC_CONF[@]}" "${LIBSTDCXX_CONF[@]}"
	mk cross-gcc all-gcc all-gcc
	mk cross-gcc install-gcc install-gcc
	compat "$CROSS/$TARGET"
	log "cross libgcc"
	mk cross-gcc all-target-libgcc all-target-libgcc
	mk cross-gcc install-target-libgcc install-target-libgcc
	log "cross libstdc++"
	mk cross-gcc all-target-libstdc++-v3 all-target-libstdc++-v3
	mk cross-gcc install-target-libstdc++-v3 install-target-libstdc++-v3
}

HOSTLIBS=$B/hostlibs

hostlibs(){
	local common=(--host=$TARGET --prefix="$HOSTLIBS" --disable-shared --enable-static)
	# gnu17: gmp 6.3's configure tests use K&R declarations, an error in
	# gcc 16's default C23; APE's feature macros come from the driver
	export CC=$TARGET-gcc CFLAGS="-O2 -g -std=gnu17"
	log "hostlibs gmp"
	conf hostlibs-gmp "$SRC/gmp" "${common[@]}" --disable-assembly --disable-cxx
	mk hostlibs-gmp all all
	mk hostlibs-gmp install install
	log "hostlibs mpfr"
	conf hostlibs-mpfr "$SRC/mpfr" "${common[@]}" --with-gmp="$HOSTLIBS" --disable-thread-safe
	mk hostlibs-mpfr all all
	mk hostlibs-mpfr install install
	log "hostlibs mpc"
	conf hostlibs-mpc "$SRC/mpc" "${common[@]}" --with-gmp="$HOSTLIBS" --with-mpfr="$HOSTLIBS"
	mk hostlibs-mpc all all
	mk hostlibs-mpc install install
	unset CC CFLAGS
}

# binutils and gcc that run on 9front, installed as on the system
# (/$objtype/gnu) under out/$objtype; the cross toolchain compiles them,
# its libgcc build uses the sysroot's headers (--with-build-sysroot)
OUT=$TOP/out/$objtype
native(){
	local host=(--build=$(cc -dumpmachine) --host=$TARGET --prefix=/$objtype/gnu)
	log "native binutils"
	conf native-binutils "$SRC/binutils" "${host[@]}" "${BINUTILS_CONF[@]}"
	mk native-binutils all all
	mk native-binutils install install-strip DESTDIR="$OUT"
	log "native gcc"
	# AC_C_BIGENDIAN finds no answer in a Canadian build (libdecnumber,
	# libcpp): every target is little-endian
	export ac_cv_c_bigendian=no
	# libcody assumes sockets on every __unix__; APE's are not POSIX's
	# (opt-in with _BSD_EXTENSION, int lengths and results)
	export CXXFLAGS="-g -O2 -DCODY_NETWORKING=0"
	# the target libraries are built with the cross compiler; without
	# -nostdinc++ libstdc++'s configure sees the cross libstdc++'s
	# installed headers (an empty <fenv.h> wrapper: "fenv.h usability...
	# yes"), as the cross build's own xgcc -nostdinc++ does not
	conf native-gcc "$SRC/gcc" "${host[@]}" --with-build-sysroot="$SYSROOT" \
		CXX_FOR_TARGET="$TARGET-g++ -nostdinc++" \
		--with-gmp="$HOSTLIBS" --with-mpfr="$HOSTLIBS" --with-mpc="$HOSTLIBS" \
		--enable-languages=c,c++ "${GCC_CONF[@]}" "${LIBSTDCXX_CONF[@]}"
	mk native-gcc all all
	mk native-gcc install install-strip DESTDIR="$OUT"
	compat "$OUT/$objtype/gnu/$TARGET"
}

# gccmk and its wrappers (plan9/) into /$objtype/gnu: mk on 9front with
# the native gcc, with the flags of the tree's build ($PLAN9/$objtype/
# cflags.json, gcc.json, ape.json; the libraries a program manifest
# links), one argument per line for the wrappers' lib.rc
plan9(){
	local d=$OUT/$objtype/gnu/plan9 O
	case $objtype in amd64) O=6 ;; 386) O=8 ;; riscv64) O=j ;; esac
	log "gccmk"
	rm -rf "$d"
	mkdir -p "$d/bin" "$OUT/$objtype/gnu/bin"
	cp "$TOP"/plan9/lib.rc "$d/"
	cp "$TOP"/plan9/bin/* "$d/bin/"
	cp "$TOP"/plan9/gccmk "$OUT/$objtype/gnu/bin/"
	printf '</sys/src/mkfile.proto\n\nCC=gcc-cc\nLD=gcc-ld\nO=%s\nAS=gcc-as\n' $O > "$d/mkfile"
	python3 - "$PLAN9" "$objtype" "$d" <<'PY'
import json, os, sys
root, objtype, d = sys.argv[1:]
def flags(name, key):
	out = []
	for e in json.load(open(os.path.join(root, objtype, name))):
		out += e.get(key, [])
	return [f.replace('/$ARCH', '/' + objtype) for f in out]
def libs(manifest):
	e = json.load(open(os.path.join(root, manifest)))[0]
	return [f.replace('/$ARCH', '/' + objtype) for f in e['Libs'] if not f.startswith('--')]
cflags = flags('cflags.json', 'Cflags') + flags('gcc.json', 'Cflags')
# the build tool's assembler flags: the C flags for the machine
asflags = [f for f in cflags if f.startswith(('-m', '-fzero', '-fcommon')) or f == '-fno-pie']
ldflags = ['--entry=_main'] + flags('cflags.json', 'Oflags')
lists = {
	'cflags': cflags,
	'apeflags': flags('ape.json', 'Cflags'),
	'asflags': asflags,
	'ldflags': ldflags,
	# and the machine's (386: libgcc.a, 64-bit division)
	'libs': libs('sys/src/cmd/cmds.json') + flags('cflags.json', 'Libs'),
	'apelibs': libs('sys/src/ape/cmd/cmds.json') + flags('cflags.json', 'Libs'),
}
for name, words in lists.items():
	with open(os.path.join(d, name), 'w') as f:
		f.write(''.join(w + '\n' for w in words))
PY
}

[ $# -gt 0 ] || { sed -n '2,11s/^# \{0,1\}//p' "$0"; exit 1; }
for s; do
	case $s in
	cross) cross ;;
	compat) compat "$CROSS/$TARGET" ;;
	hostlibs) hostlibs ;;
	native) native ;;
	plan9) plan9 ;;
	*) echo "build.sh: unknown stage $s" >&2; exit 1 ;;
	esac
done
