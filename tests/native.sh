#!/bin/bash
# The native toolchain (out/$objtype, /$objtype/gnu) in the 9front guest
# (../9front's scripts/guest/boot-cli.sh): compile and run tests/ape.c
# (APE, the driver's defaults) and tests/plan9.c (native Plan 9 C), and
# compare the object gcc makes on 9front with the cross compiler's (no
# debug info, no .comment: the same code). The guest's root has no
# headers and archives, they come along with the toolchain.
# Prints "name ok" or "name FAIL ..." per test.
set -e
TOP=$(cd "$(dirname "$0")/.." && pwd)
export PLAN9=${PLAN9:-$(cd "$TOP/../9front" && pwd)}
export objtype=${objtype:-amd64}
case $objtype in amd64) TARGET=x86_64-pc-plan9 ;; 386) TARGET=i686-pc-plan9 ;; esac
X=$TOP/build/$objtype/cross/bin/$TARGET
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
E=$T/extra
mkdir -p "$E/$objtype" "$E/sys" "$E/tests"
cp -r "$TOP/out/$objtype/$objtype/gnu" "$E/$objtype/"
cp -r "$PLAN9/$objtype/include" "$PLAN9/$objtype/lib.gcc" "$PLAN9/$objtype/plan9.ld" "$E/$objtype/"
cp -r "$PLAN9/sys/include" "$E/sys/"
cp "$TOP"/tests/*.c "$E/tests/"
# the cross compiler's object, as the native one is made
P9FLAGS="-O2 -nostdinc -I/$objtype/include -I/sys/include -fplan9-extensions -fno-builtin"
$X-gcc $P9FLAGS --sysroot="$TOP/build/$objtype/sysroot" -c -o "$E/tests/plan9.cross.o" "$TOP/tests/plan9.c" 2>/dev/null ||
	$X-gcc $(echo $P9FLAGS | sed "s|-I/|-I$PLAN9/|g") -c -o "$E/tests/plan9.cross.o" "$TOP/tests/plan9.c"
$X-objcopy --strip-debug -R .comment "$E/tests/plan9.cross.o"

cmds=(
	"bind -b /$objtype/gnu/bin /bin"
	'cd /tmp'
	"gcc -O2 -o ape /tests/ape.c && ./ape; if(~ \$status '') echo ape ok; if not echo ape FAIL \$status"
	"gcc $P9FLAGS -c /tests/plan9.c && gcc -nostdlib -o plan9 plan9.o -L/$objtype/lib.gcc -lc -lgcc && ./plan9; if(~ \$status '') echo plan9 ok; if not echo plan9 FAIL \$status"
	"objcopy --strip-debug -R .comment plan9.o plan9.s.o && cmp plan9.s.o /tests/plan9.cross.o; if(~ \$status '') echo samecode ok; if not echo samecode FAIL \$status"
)
cd "$PLAN9"
MEM=${MEM:-1024} CMDWAIT=${CMDWAIT:-900} UFS_EXTRA=$E bash scripts/guest/boot-cli.sh "${cmds[@]}" > "$T/log" 2>&1 || true
tr -d '\r' < "$T/log" > "$TOP/build/$objtype/native-test.log"
grep -E '^(ape|plan9|samecode) (ok|FAIL)' "$TOP/build/$objtype/native-test.log" || true
[ "$(grep -cE '^(ape|plan9|samecode) ok' "$TOP/build/$objtype/native-test.log")" = 3 ] ||
	{ echo "native: not all ok (build/$objtype/native-test.log)" >&2; exit 1; }
