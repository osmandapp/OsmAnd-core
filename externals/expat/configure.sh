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
curl -L http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download > $SRCLOC/upstream.tar.gz || { echo "Failed to download!" 1>&2; exit; }

# Extract
echo "Extracting upstream..."
mkdir -p $SRCLOC/upstream.original
tar -xzf $SRCLOC/upstream.tar.gz -C $SRCLOC/upstream.original --strip 1

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