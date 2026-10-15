#!/bin/bash
# Build tests/*.c, *.cc with the cross toolchain (build/cross) and run
# them in the 9front guest (../9front's scripts/guest/boot-cli.sh, the
# programs in /tests). Prints one line per program: "name ok" or
# "name FAIL ...". Needs the 9front tree built.
set -e
TOP=$(cd "$(dirname "$0")/.." && pwd)
export PLAN9=${PLAN9:-$(cd "$TOP/../9front" && pwd)}
export objtype=${objtype:-amd64}
case $objtype in amd64) TARGET=x86_64-pc-plan9 ;; 386) TARGET=i686-pc-plan9 ;; esac
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
cmds=()
for p in ape plan9 cxx; do
	cmds+=("/tests/$p; if(~ \$status '') echo $p ok; if not echo $p FAIL \$status")
done
cd "$PLAN9"
UFS_EXTRA=$T/extra bash "$PLAN9/scripts/guest/boot-cli.sh" "${cmds[@]}" > "$T/log" 2>&1 || true
tr -d '\r' < "$T/log" | grep -E '^(ape|plan9|cxx) (ok|FAIL)' || { tail -20 "$T/log"; exit 1; }
! tr -d '\r' < "$T/log" | grep -qE '^(ape|plan9|cxx) FAIL'
