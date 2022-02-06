#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi

cp -rpf "mkspecs/macx-clang" "mkspecs/macx-clang-libc++-i386"
cp -rpf "mkspecs/macx-clang" "mkspecs/macx-clang-libc++-x86_64"
cp -rpf "mkspecs/macx-clang" "mkspecs/macx-clang-libc++-arm64"
cp -rpf "mkspecs/linux-clang" "mkspecs/linux-clang-i686"
cp -rpf "mkspecs/linux-clang" "mkspecs/linux-clang-amd64"
cp -rpf "mkspecs/win32-g++" "mkspecs/win32-g++-i686"
cp -rpf "mkspecs/win32-g++" "mkspecs/win32-g++-64"
