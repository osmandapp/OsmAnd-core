#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if ls -1 $SRCLOC/upstream.patched >/dev/null 2>&1
then
   exit
fi

# Delete old one if such exists
if ls -1 $SRCLOC/upstream.* >/dev/null 2>&1
then
	echo "Deleting old upstream..."
	rm -rf $SRCLOC/upstream.*
fi

# Download
echo "Zlib Downloading new upstream..."
curl -L http://sourceforge.net/projects/libpng/files/zlib/1.2.7/zlib-1.2.7.tar.bz2/download > $SRCLOC/upstream.tar.bz2 || { echo "Failed to download!" 1>&2; exit; }

# Extract
echo "Zlib Extracting upstream..."
mkdir -p $SRCLOC/upstream.original
tar -xjf $SRCLOC/upstream.tar.bz2 -C $SRCLOC/upstream.original --strip 1

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
