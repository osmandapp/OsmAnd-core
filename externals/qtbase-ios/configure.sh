#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Check if already configured
if [ -d "$SRCLOC/upstream.patched" ]; then
	echo "Skipping external '$NAME': already configured"
	exit
fi

# Extract upstream if needed
if [ ! -d "$SRCLOC/upstream.original" ]; then
	echo "Downloading '$NAME' upstream..."
	git clone https://github.com/osmandapp/OsmAnd-external-qtbase.git "$SRCLOC/upstream.original" -b ios --depth=1
fi

# Patch
cp -rpf "$SRCLOC/upstream.original" "$SRCLOC/upstream.patched"
cp -rpf "$SRCLOC/upstream.original/mkspecs/unsupported/macx-ios-clang" "$SRCLOC/upstream.patched/mkspecs/unsupported/macx-ios-clang-device-armv7"
cp -rpf "$SRCLOC/upstream.original/mkspecs/unsupported/macx-ios-clang" "$SRCLOC/upstream.patched/mkspecs/unsupported/macx-ios-clang-device-armv7s"
cp -rpf "$SRCLOC/upstream.original/mkspecs/unsupported/macx-ios-clang" "$SRCLOC/upstream.patched/mkspecs/unsupported/macx-ios-clang-simulator-i386"
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
