#!/bin/bash
# The sysroot of the target x86_64-pc-plan9: a view of the gcc-built 9front
# tree ($PLAN9, default ../9front) as it is on an installed system --
# /amd64/include, /sys/include, /amd64/lib.gcc (libap.a, libc.a, ...),
# /amd64/plan9.ld. Needs the tree built (CC=gcc ARCH=amd64 build.exe).
#
# usage: sysroot.sh dir
set -e
TOP=$(cd "$(dirname "$0")" && pwd)
. "$TOP/scripts/nfront-paths.sh"	# TREE, TREE_GCC, GCC_DIR, ... (the distribution's paths.env)
PLAN9=${PLAN9:-$(cd "$TREE" && pwd)}
objtype=${objtype:-amd64}
OUT=${1:?usage: sysroot.sh dir}
for f in $objtype/include sys/include $objtype/lib.gcc/ape/libap.a $objtype/lib.gcc/libc.a $objtype/plan9.ld; do
	[ -e "$PLAN9/$f" ] || { echo "sysroot: no $PLAN9/$f (build the tree first)" >&2; exit 1; }
done
rm -rf "$OUT"
mkdir -p "$OUT/$objtype" "$OUT/sys"
ln -s "$PLAN9/$objtype/include" "$OUT/$objtype/include"
ln -s "$PLAN9/$objtype/lib.gcc" "$OUT/$objtype/lib.gcc"
ln -s "$PLAN9/$objtype/plan9.ld" "$OUT/$objtype/plan9.ld"
ln -s "$PLAN9/sys/include" "$OUT/sys/include"
