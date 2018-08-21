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
echo "JPEG Downloading new upstream..."
curl -L http://www.ijg.org/files/jpegsrc.v9c.tar.gz > $SRCLOC/upstream.tar.gz || { echo "Failed to download!" 1>&2; exit; }
#curl -L http://builder.osmand.net/legacy-dependencies-mirror/jpegsrc.v8d.tar.gz > $SRCLOC/upstream.tar.gz || { echo "Failed to download!" 1>&2; exit; }

# Extract
echo "JPEG Extracting upstream..."
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
