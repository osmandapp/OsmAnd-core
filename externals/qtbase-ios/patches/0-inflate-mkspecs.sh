#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi

cp -rpf "mkspecs/macx-ios-clang" "mkspecs/macx-ios-clang-device"
cp -rpf "mkspecs/macx-ios-clang" "mkspecs/macx-ios-clang-simulator"
