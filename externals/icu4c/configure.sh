#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Fail on any error
set -e

# Check if already configured
if [ -d "$SRCLOC/upstream.patched" ] && [ -d "$SRCLOC/upstream.data" ]; then
	echo "Skipping external '$NAME': already configured"
	exit
fi

# Download upstream if needed
if [ ! -f "$SRCLOC/upstream.pack" ]; then
	echo "Downloading '$NAME' upstream..."
	curl -L http://download.icu-project.org/files/icu4c/52.1/icu4c-52_1-src.tgz > "$SRCLOC/upstream.pack" || { echo "Failure" 1>&2; exit; }
fi

# Extract upstream if needed
if [ ! -d "$SRCLOC/upstream.original" ]; then
	echo "Extracting '$NAME' upstream..."
	mkdir -p "$SRCLOC/upstream.original"
	tar -xf "$SRCLOC/upstream.pack" -C "$SRCLOC/upstream.original" --strip 1	
fi

# Patch
if [ ! -d "$SRCLOC/upstream.patched" ]; then
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
fi

# Download upstream data if needed
if [ ! -f "$SRCLOC/upstream.data-pack" ]; then
	echo "Downloading '$NAME' upstream data..."
	curl -L http://download.osmand.net/prebuilt/icudt52l.zip > "$SRCLOC/upstream.data-pack" || { echo "Failure" 1>&2; exit; }
fi

# Extract upstream data if needed
if [ ! -d "$SRCLOC/upstream.data" ]; then
	echo "Extracting '$NAME' upstream data..."
	mkdir -p "$SRCLOC/upstream.data"
	unzip -d "$SRCLOC/upstream.data" "$SRCLOC/upstream.data-pack"
fi
