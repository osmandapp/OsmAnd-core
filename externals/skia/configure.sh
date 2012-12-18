#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Delete old one if such exists
if ls -1 $SRCLOC/upstream.* >/dev/null 2>&1
then
	echo "Deleting old upstream..."
	rm -rf $SRCLOC/upstream.*
fi

# Download
echo "Downloading new upstream..."
git clone http://git.chromium.org/external/skia.git $SRCLOC/upstream.original --depth=10
git --work-tree="$SRCLOC/upstream.original" --git-dir="$SRCLOC/upstream.original/.git" checkout d7204d413233fac7fc15c0efecb0622205165987
git --work-tree="$SRCLOC/upstream.original" --git-dir="$SRCLOC/upstream.original/.git" reset --hard

# Patch
cp -rf $SRCLOC/upstream.original $SRCLOC/upstream.patched
if [ -d $SRCLOC/patches ]; then
	echo "Patching..."
	PATCHES=`ls -1 $SRCLOC/patches/*.patch | sort`
	for PATCH in $PATCHES
	do
		read  -rd '' PATCH <<< "$PATCH"
		echo "Applying "`basename $PATCH`
		patch --strip=1 --directory=$SRCLOC/upstream.patched/ --input=$PATCH
	done
fi
