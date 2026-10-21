#!/bin/bash
# Build tests/*.c, *.cc with the cross toolchain (build/cross) and run
# them in the 9front guest (../9front's scripts/guest/boot-cli.sh, the
# programs in /tests): APE and native Plan 9 C, C++ (exceptions), the
# toolchain's additions to APE (compat/: C99 math, stdio, time, wide
# characters, pthreads) and libstdc++ over them. Prints one line per
# program: "name ok" or "name FAIL ...". Needs the 9front tree built.
set -e
TOP=$(cd "$(dirname "$0")/.." && pwd)
export objtype=${objtype:-amd64}
# riscv64: the branch with the port, beside the tree
[ -n "$PLAN9" ] || [ $objtype != riscv64 ] || [ ! -d "$TOP/../9front-riscv" ] || PLAN9=$TOP/../9front-riscv
export PLAN9=$(cd "${PLAN9:-$TOP/../9front}" && pwd)
case $objtype in amd64) TARGET=x86_64-pc-plan9 ;; 386) TARGET=i686-pc-plan9 ;; riscv64) TARGET=riscv64-unknown-plan9 ;; esac
X=$TOP/build/$objtype/cross/bin/$TARGET
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
mkdir -p "$T/extra/tests"
O=$T/extra/tests
$X-gcc -O2 -o "$O/ape" "$TOP/tests/ape.c"
$X-gcc -O2 -nostdinc -I"$PLAN9/$objtype/include" -I"$PLAN9/sys/include" -fplan9-extensions \
	-fno-builtin -c -o "$T/plan9.o" "$TOP/tests/plan9.c"
$X-gcc -nostdlib -o "$O/plan9" "$T/plan9.o" -L"$PLAN9/$objtype/lib.gcc" -lc -lgcc
$X-g++ -O2 -o "$O/cxx" "$TOP/tests/cxx.cc"
$X-g++ -O2 -o "$O/eh" "$TOP/tests/eh.cc"
# c99math: fenv needs -frounding-math (gcc's FENV_ACCESS)
$X-gcc -O2 -frounding-math -o "$O/c99math" "$TOP/tests/c99math.c"
for p in c99stdio c99time c99wchar pthread; do
	$X-gcc -O2 -o "$O/$p" "$TOP/tests/$p.c"
done
$X-g++ -O2 -o "$O/cxxlib" "$TOP/tests/cxxlib.cc"
tests="ape plan9 cxx eh c99math c99stdio c99time c99wchar pthread cxxlib"
cmds=()
for p in $tests; do
	cmds+=("/tests/$p; if(~ \$status '') echo $p ok; if not echo $p FAIL \$status")
done
re="^(${tests// /|}) "
cd "$PLAN9"
UFS_EXTRA=$T/extra bash "$PLAN9/scripts/guest/boot-cli.sh" "${cmds[@]}" > "$T/log" 2>&1 || true
tr -d '\r' < "$T/log" > "$TOP/build/$objtype/cross-test.log"
# the failures the programs print, then the results
grep -E "^(${tests// /|}): FAIL" "$TOP/build/$objtype/cross-test.log" || true
grep -E "$re(ok|FAIL)" "$TOP/build/$objtype/cross-test.log" || { tail -20 "$TOP/build/$objtype/cross-test.log"; exit 1; }
[ "$(grep -cE "${re}ok" "$TOP/build/$objtype/cross-test.log")" = "$(echo $tests | wc -w)" ]
