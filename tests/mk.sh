#!/bin/bash
# gccmk (plan9/, build.sh plan9) in the 9front guest (../9front's
# scripts/guest/boot-cli.sh): 9front's own mkfiles with the native gcc --
# a library (libbio: mksyslib, ar; its members replace the build's x.o),
# a program with yacc (hoc: mkone) and an APE program (ape/cmd/expr:
# pcc), each run, and the assembly of libc, libmp and libsec (gcc-as: the
# GAS ports). libbio's and the assembly's objects must be the build
# tool's (the tree's build on Linux; the code, see STRIP). Prints "name ok" or "name FAIL ..."
# per test.
set -e
TOP=$(cd "$(dirname "$0")/.." && pwd)
export PLAN9=${PLAN9:-$(cd "$TOP/../9front" && pwd)}
export objtype=${objtype:-amd64}
case $objtype in amd64) O=6 ;; 386) O=8 ;; esac
[ -r "$TOP/out/$objtype/$objtype/gnu/plan9/mkfile" ] || { echo "mk: run build.sh plan9 first" >&2; exit 1; }
[ -s "$PLAN9/$objtype/lib.gcc/libbio.a" ] || { echo "mk: no $PLAN9/$objtype/lib.gcc/libbio.a (build the tree)" >&2; exit 1; }
# the code: no debug info, version strings, notes or unwind tables (the
# Linux gcc for i386 makes them, i686-pc-plan9 not)
STRIP="--strip-unneeded -R .comment -R .note.gnu.property -R .eh_frame -R .rel.eh_frame -R .rela.eh_frame"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
E=$T/extra
# /$objtype/lib: where gccmk binds lib.gcc (9front has it, the guest not)
mkdir -p "$E/$objtype/lib" "$E/sys/src/cmd" "$E/sys/src/ape/cmd" "$E/tests/ref"
cp -r "$TOP/out/$objtype/$objtype/gnu" "$E/$objtype/"
cp -r "$PLAN9/$objtype/include" "$PLAN9/$objtype/lib.gcc" "$PLAN9/$objtype/plan9.ld" "$PLAN9/$objtype/mkfile" "$E/$objtype/"
cp -r "$PLAN9/sys/include" "$E/sys/"
cp "$PLAN9/sys/src/mkfile.proto" "$E/sys/src/"
cp "$PLAN9"/sys/src/cmd/mk{one,many,syslib} "$E/sys/src/cmd/"
cp "$PLAN9/sys/src/ape/config" "$E/sys/src/ape/"
# the sources only (git's files), not the Linux build's objects
for d in sys/src/libbio sys/src/cmd/hoc sys/src/ape/cmd/expr sys/src/libc/$objtype \
	sys/src/libmp sys/src/libsec; do
	mkdir -p "$E/$d"
	git -C "$PLAN9" ls-files -z "$d" | (cd "$PLAN9" && tar -cf - --null -T -) | tar -xf - -C "$E"
done
(cd "$E/tests/ref" && ar x "$PLAN9/$objtype/lib.gcc/libbio.a" &&
	ar x "$PLAN9/$objtype/lib.gcc/libc.a" memmove.o strlen.o setjmp.o &&
	ar x "$PLAN9/$objtype/lib.gcc/libmp.a" mpvecadd.o mpdigdiv.o &&
	ar x "$PLAN9/$objtype/lib.gcc/libsec.a" md5block.o sha1block.o)
for o in "$E"/tests/ref/*.o; do
	objcopy $STRIP "$o"
done

cmds=(
	"bind -b /$objtype/gnu/bin /bin"
	"cd /sys/src/libbio && gccmk install >/tmp/libbio.log >[2=1] && ! ar t /$objtype/lib.gcc/libbio.a | grep -s '\.'$O'$'; if(~ \$status '') echo libbio ok; if not { tail -5 /tmp/libbio.log; echo libbio FAIL }"
	"bad=0; for(o in *.$O){ objcopy $STRIP \$o /tmp/x.o; cmp -s /tmp/x.o /tests/ref/\`{basename \$o .$O}^.o || { echo differs \$o; bad=1 } }; if(~ \$bad 0) echo samecode ok; if not echo samecode FAIL"
	"cd /sys/src/libc/$objtype && gccmk memmove.$O strlen.$O setjmp.$O >/tmp/asm.log >[2=1] && bad=0 && for(o in memmove strlen setjmp){ objcopy $STRIP \$o.$O /tmp/x.o; cmp -s /tmp/x.o /tests/ref/\$o.o || bad=1 } && ~ \$bad 0; if(~ \$status '') echo asm ok; if not { tail -5 /tmp/asm.log; echo asm FAIL }"
	"bad=0; for(d in libmp:mpvecadd libmp:mpdigdiv libsec:md5block libsec:sha1block){ l=\`{echo \$d | sed 's/:.*//'}; o=\`{echo \$d | sed 's/.*://'}; cd /sys/src/\$l/$objtype && gccmk \$o.$O >>/tmp/mpsec.log >[2=1] && objcopy $STRIP \$o.$O /tmp/x.o && cmp -s /tmp/x.o /tests/ref/\$o.o || bad=1 }; if(~ \$bad 0) echo mpsec ok; if not { tail -5 /tmp/mpsec.log; echo mpsec FAIL }"
	"cd /sys/src/cmd/hoc && gccmk >/tmp/hoc.log >[2=1] && ~ \`{echo '6*7' | ./$O.out} 42; if(~ \$status '') echo hoc ok; if not { tail -5 /tmp/hoc.log; echo hoc FAIL }"
	"cd /sys/src/ape/cmd/expr && gccmk >/tmp/expr.log >[2=1] && ~ \`{./$O.out 6 '*' 7} 42; if(~ \$status '') echo expr ok; if not { tail -5 /tmp/expr.log; echo expr FAIL }"
)
cd "$PLAN9"
MEM=${MEM:-1024} CMDWAIT=${CMDWAIT:-600} UFS_EXTRA=$E bash scripts/guest/boot-cli.sh "${cmds[@]}" > "$T/log" 2>&1 || true
tr -d '\r' < "$T/log" > "$TOP/build/$objtype/mk-test.log"
grep -E '^(libbio|samecode|asm|mpsec|hoc|expr) (ok|FAIL)|^differs' "$TOP/build/$objtype/mk-test.log" || true
[ "$(grep -cE '^(libbio|samecode|asm|mpsec|hoc|expr) ok' "$TOP/build/$objtype/mk-test.log")" = 6 ] ||
	{ echo "mk: not all ok (build/$objtype/mk-test.log)" >&2; exit 1; }
