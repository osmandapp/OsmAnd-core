#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Fail on any error
set -e

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

# Download upstream if needed
if [ ! -f "$SRCLOC/upstream.pack" ]; then
	echo "Downloading '$NAME' upstream..."
	curl -L http://sourceforge.net/projects/ogl-math/files/glm-0.9.5.2/glm-0.9.5.2.zip/download > "$SRCLOC/upstream.pack" || { echo "Failure" 1>&2; exit; }
fi

# Extract upstream if needed
if [ ! -d "$SRCLOC/upstream.original" ]; then
	echo "Extracting '$NAME' upstream..."
	mkdir -p "$SRCLOC/upstream.tmp"
	unzip -q "$SRCLOC/upstream.pack" -d "$SRCLOC/upstream.tmp"
	mv "$SRCLOC/upstream.tmp/glm" "$SRCLOC/upstream.original"
	rm -rf "$SRCLOC/upstream.tmp"
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