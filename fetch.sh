#!/bin/bash
# Download the sources listed in sources.txt to dl/, check them against
# their signatures (GNU: gnu-keyring.gpg; others: their key in keys/) and
# sha256.txt, unpack them to
# src/<name> and apply patches/<name>.patch. src/gcc and src/binutils are
# git repositories ("pristine" commit = the tarball), so patches.sh can
# write the patches back from their working trees.
# -u records the sha256 sums of new downloads.
set -e
cd "$(dirname "$0")"
mkdir -p dl/sig src
[ -s dl/sig/gnu-keyring.gpg ] || curl -fL -o dl/sig/gnu-keyring.gpg https://ftp.gnu.org/gnu/gnu-keyring.gpg
while read -r name url key; do
	case $name in ''|\#*) continue ;; esac
	f=dl/$(basename "$url")
	[ -s "$f" ] || curl -fL -o "$f" "$url"
	if [ -n "$key" ]; then
		# a project's own key, checked in (keys/, sources.txt)
		sig=dl/sig/${f#dl/}.asc
		[ -s "$sig" ] || curl -fL -o "$sig" "$url.asc"
		gpg --dearmor < "keys/$key.pub" > "dl/sig/$key.gpg"
		ring=$PWD/dl/sig/$key.gpg
	else
		sig=dl/sig/${f#dl/}.sig
		[ -s "$sig" ] || curl -fL -o "$sig" "$url.sig"
		ring=$PWD/dl/sig/gnu-keyring.gpg
	fi
	gpgv --keyring "$ring" "$sig" "$f" 2>/dev/null ||
		{ echo "fetch: bad signature on $f" >&2; exit 1; }
	if grep -q " $f\$" sha256.txt 2>/dev/null; then
		grep " $f\$" sha256.txt | sha256sum -c --quiet
	elif [ "$1" = -u ]; then
		sha256sum "$f" >> sha256.txt
	else
		echo "fetch: no sha256 for $f (run fetch.sh -u)" >&2; exit 1
	fi
	[ -d "src/$name" ] && continue
	mkdir "src/$name.tmp"
	tar -xf "$f" -C "src/$name.tmp" --strip-components=1
	mv "src/$name.tmp" "src/$name"
	if [ -f "patches/$name.patch" ]; then
		(cd "src/$name" && git init -q && git add -A &&
		 git -c user.name=pristine -c user.email=pristine@localhost commit -qm pristine && git tag pristine &&
		 git apply --index "../../patches/$name.patch")
	fi
done < sources.txt
