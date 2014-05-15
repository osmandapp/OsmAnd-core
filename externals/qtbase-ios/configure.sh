#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../functions.sh"

configureExternalFromGit "$SRCLOC" "https://github.com/osmandapp/OsmAnd-external-qtbase.git" "ios"
cp -rpf "$SRCLOC/upstream.original/mkspecs/macx-ios-clang" "$SRCLOC/upstream.original/mkspecs/macx-ios-clang-device-armv7"
cp -rpf "$SRCLOC/upstream.original/mkspecs/macx-ios-clang" "$SRCLOC/upstream.original/mkspecs/macx-ios-clang-device-armv7s"
cp -rpf "$SRCLOC/upstream.original/mkspecs/macx-ios-clang" "$SRCLOC/upstream.original/mkspecs/macx-ios-clang-simulator-i386"
patchExternal "$SRCLOC" 
