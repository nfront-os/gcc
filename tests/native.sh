#!/bin/bash
# The native toolchain (out/$objtype, /$objtype/gnu) in the 9front guest
# (../9front's scripts/guest/boot-cli.sh): compile and run tests/ape.c
# (APE, the driver's defaults), tests/plan9.c (native Plan 9 C), and with
# g++ tests/cxx.cc, tests/eh.cc and tests/cxxlib.cc (libstdc++,
# exceptions, threads over APE's pthreads), and compare the
# objects gcc and g++ make on 9front with the cross compiler's (no
# debug info, no .comment: the same code). The guest's root has no
# headers and archives, they come along with the toolchain.
# Prints "name ok" or "name FAIL ..." per test.
set -e
TOP=$(cd "$(dirname "$0")/.." && pwd)
export objtype=${objtype:-amd64}
# riscv64: the branch with the port, beside the tree
[ -n "$PLAN9" ] || [ $objtype != riscv64 ] || [ ! -d "$TOP/../9front-riscv" ] || PLAN9=$TOP/../9front-riscv
export PLAN9=$(cd "${PLAN9:-$TOP/../9front}" && pwd)
case $objtype in amd64) TARGET=x86_64-pc-plan9 ;; 386) TARGET=i686-pc-plan9 ;; riscv64) TARGET=riscv64-unknown-plan9 ;; esac
X=${BUILD:-$TOP/build}/$objtype/cross/bin/$TARGET
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
E=$T/extra
mkdir -p "$E/$objtype" "$E/sys" "$E/tests"
cp -r "${OUTDIR:-$TOP/out}/$objtype/$objtype/gnu" "$E/$objtype/"
cp -r "$PLAN9/$objtype/include" "$PLAN9/$objtype/lib.gcc" "$PLAN9/$objtype/plan9.ld" "$E/$objtype/"
cp -r "$PLAN9/sys/include" "$E/sys/"
cp "$TOP"/tests/*.c "$TOP"/tests/*.cc "$E/tests/"
# the cross compiler's object, as the native one is made
P9FLAGS="-O2 -nostdinc -I/$objtype/include -I/sys/include -fplan9-extensions -fno-builtin"
$X-gcc $P9FLAGS --sysroot="${BUILD:-$TOP/build}/$objtype/sysroot" -c -o "$E/tests/plan9.cross.o" "$TOP/tests/plan9.c" 2>/dev/null ||
	$X-gcc $(echo $P9FLAGS | sed "s|-I/|-I$PLAN9/|g") -c -o "$E/tests/plan9.cross.o" "$TOP/tests/plan9.c"
$X-objcopy --strip-debug -R .comment "$E/tests/plan9.cross.o"
$X-g++ -O2 -c -o "$E/tests/eh.cross.o" "$TOP/tests/eh.cc"
$X-objcopy --strip-debug -R .comment "$E/tests/eh.cross.o"

cmds=(
	"bind -b /$objtype/gnu/bin /bin"
	'cd /tmp'
	"gcc -O2 -o ape /tests/ape.c && ./ape; if(~ \$status '') echo ape ok; if not echo ape FAIL \$status"
	"gcc $P9FLAGS -c /tests/plan9.c && gcc -nostdlib -o plan9 plan9.o -L/$objtype/lib.gcc -lc -lgcc && ./plan9; if(~ \$status '') echo plan9 ok; if not echo plan9 FAIL \$status"
	"objcopy --strip-debug -R .comment plan9.o plan9.s.o && cmp plan9.s.o /tests/plan9.cross.o; if(~ \$status '') echo samecode ok; if not echo samecode FAIL \$status"
	"g++ -O2 -o cxx /tests/cxx.cc && ./cxx; if(~ \$status '') echo cxx ok; if not echo cxx FAIL \$status"
	"g++ -O2 -c /tests/eh.cc && g++ -o eh eh.o && ./eh; if(~ \$status '') echo eh ok; if not echo eh FAIL \$status"
	"objcopy --strip-debug -R .comment eh.o eh.s.o && cmp eh.s.o /tests/eh.cross.o; if(~ \$status '') echo samecxx ok; if not echo samecxx FAIL \$status"
	"g++ -O2 -o cxxlib /tests/cxxlib.cc && ./cxxlib; if(~ \$status '') echo cxxlib ok; if not echo cxxlib FAIL \$status"
)
cd "$PLAN9"
MEM=${MEM:-1024} CMDWAIT=${CMDWAIT:-900} UFS_EXTRA=$E bash scripts/guest/boot-cli.sh "${cmds[@]}" > "$T/log" 2>&1 || true
tr -d '\r' < "$T/log" > "${BUILD:-$TOP/build}/$objtype/native-test.log"
grep -E '^(ape|plan9|samecode|cxx|eh|samecxx|cxxlib) (ok|FAIL)|^cxxlib: FAIL' "${BUILD:-$TOP/build}/$objtype/native-test.log" || true
[ "$(grep -cE '^(ape|plan9|samecode|cxx|eh|samecxx|cxxlib) ok' "${BUILD:-$TOP/build}/$objtype/native-test.log")" = 7 ] ||
	{ echo "native: not all ok (build/$objtype/native-test.log)" >&2; exit 1; }
