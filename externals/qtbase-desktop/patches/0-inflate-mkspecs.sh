#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi

cp -rpf "mkspecs/macx-clang" "mkspecs/macx-clang-libc++-32"
cp -rpf "mkspecs/macx-clang" "mkspecs/macx-clang-libc++-64"
cp -rpf "mkspecs/linux-clang" "mkspecs/linux-clang-32"
cp -rpf "mkspecs/linux-clang" "mkspecs/linux-clang-64"
cp -rpf "mkspecs/win32-g++" "mkspecs/win32-g++-32"
cp -rpf "mkspecs/win32-g++" "mkspecs/win32-g++-64"
