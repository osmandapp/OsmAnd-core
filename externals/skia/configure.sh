#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Check if already configured
if [ -d "$SRCLOC/upstream.patched" ]; then
	echo "Skipping external '$NAME': already configured"
	exit
fi

# Extract upstream if needed
VERSION="chromium-31.0.1626.2"
if [ ! -d "$SRCLOC/upstream.original" ]; then
	echo "Downloading '$NAME' upstream..."
	mkdir -p "$SRCLOC/upstream.original"
	(cd "$SRCLOC/upstream.original" && \
		git init && \
		git remote add origin -t $VERSION https://github.com/osmandapp/OsmAnd-external-skia.git && \
		git fetch --depth=1
		git checkout $VERSION
	)
fi

# Patch
cp -rpf "$SRCLOC/upstream.original" "$SRCLOC/upstream.patched"
if [ -d "$SRCLOC/patches" ]; then
	echo "Patching '$NAME'..."
	PATCHES=`ls -1 $SRCLOC/patches/*.patch | sort`
	for PATCH in $PATCHES
	do
		read  -rd '' PATCH <<< "$PATCH"
		echo "Applying "`basename $PATCH`
		patch --strip=1 --directory="$SRCLOC/upstream.patched/" --input="$PATCH"
	done
fi
