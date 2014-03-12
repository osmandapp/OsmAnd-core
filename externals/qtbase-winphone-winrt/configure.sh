#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Check if tools are present
if [[ "$(uname -a)" == *Cygwin* ]]; then
	if [ ! -f "$SRCLOC/tools.windows.pack" ]; then
		echo "Downloading '$NAME' tools for Windows..."
		curl -L https://qt.gitorious.org/qt/qt5/archive-tarball/dev > "$SRCLOC/tools.windows.pack" || { echo "Failure" 1>&2; exit; }
	fi
	if [ ! -d "$SRCLOC/tools.windows" ]; then
		echo "Extracting '$NAME' tools for Windows..."
		mkdir -p "$SRCLOC/tools.windows"
		tar -xf "$SRCLOC/tools.windows.pack" -C "$SRCLOC/tools.windows" "qt-qt5/gnuwin32" --strip=2
		chmod +x "$SRCLOC/tools.windows/bin"/*
	fi
fi

# Check if already configured
if [ -d "$SRCLOC/upstream.patched" ]; then
	echo "Skipping external '$NAME': already configured"
	exit
fi

# Extract upstream if needed
if [ ! -d "$SRCLOC/upstream.original" ]; then
	echo "Downloading '$NAME' upstream..."
	git clone https://github.com/osmandapp/OsmAnd-external-qtbase.git "$SRCLOC/upstream.original" -b winrt-winphone --depth=1
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
