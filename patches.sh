#!/bin/bash
# Write patches/<name>.patch from the working trees of src/gcc and
# src/binutils (diff against their "pristine" commit, new files included).
set -e
cd "$(dirname "$0")"
mkdir -p patches
for n in gcc binutils; do
	(cd "src/$n" && git add -A && git diff --cached pristine) > "patches/$n.patch"
	echo "patches/$n.patch: $(grep -c '^diff --git' "patches/$n.patch") files"
done
