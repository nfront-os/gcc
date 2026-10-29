#!/bin/bash
# Build the gcc toolchain for 9front in stages (README.md):
#
#   cross     binutils and gcc (C, C++) on Linux for the target,
#             with libgcc and libstdc++ over APE   -> build/$objtype/cross
#   hostlibs  gmp, mpfr, mpc for 9front            -> build/$objtype/hostlibs
#   native    binutils and gcc that run on 9front  -> out/$objtype
#   plan9     gccmk: mk with gcc on 9front (plan9/) -> out/$objtype
#
# objtype=amd64 (default): x86_64-pc-plan9; objtype=386: i686-pc-plan9;
# objtype=riscv64: riscv64-unknown-plan9.
# usage: [objtype=386] build.sh stage...    (BUILD: build directory, default
#        build; OUTDIR: where native and plan9 go, default out; PLAN9: the 9front tree, default
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
B=${BUILD:-$TOP/build}/$objtype
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
# libstdc++ over APE (cross and native): threads are APE's pthreads
# (gcc --enable-threads=posix; thread_local by emutls,
# --disable-tls), only the "C" locale (generic); the clocks
# (steady_clock), nanosleep, sched_yield of APE checked by linking
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
OUT=${OUTDIR:-$TOP/out}/$objtype
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
	hostlibs) hostlibs ;;
	native) native ;;
	plan9) plan9 ;;
	*) echo "build.sh: unknown stage $s" >&2; exit 1 ;;
	esac
done
