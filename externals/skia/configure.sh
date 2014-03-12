#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Check if needs reconfiguring
if [ -f "$SRCLOC/stamp" ]; then
	last_stamp=""
	if [ -f "$SRCLOC/.stamp" ]; then
		last_stamp=`cat "$SRCLOC/.stamp"`
	fi
	current_stamp=`cat "$SRCLOC/stamp"`
	echo "Last stamp:    "$last_stamp
	echo "Current stamp: "$current_stamp
	if [ "$last_stamp" != "$current_stamp" ]; then
		echo "Stamps differ, will clean external '$NAME'..."
		"$SRCLOC/../clean.sh" $NAME
		cp "$SRCLOC/stamp" "$SRCLOC/.stamp"
	fi
fi

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
		git fetch --depth=1 && \
		git checkout $VERSION)
	rm -rf "$SRCLOC/upstream.original/.git"
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
