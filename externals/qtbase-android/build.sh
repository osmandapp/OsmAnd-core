#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $(dirname "${BASH_SOURCE[0]}") )

if [ ! -d "$ANDROID_SDK_ROOT" ]; then
	echo "ANDROID_SDK_ROOT is not set"
	exit
fi
if [ ! -d "$ANDROID_NDK_ROOT" ]; then
	echo "ANDROID_NDK_ROOT is not set"
	exit
fi
export ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT
export ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT
export ANDROID_NDK_TOOLCHAIN_VERSION=4.7
export ANDROID_NDK_PLATFORM=android-8
(cd "$SRCLOC/upstream.patched" && \
./configure -opensource -confirm-license -xplatform android-g++ \
	-nomake examples -nomake demos -nomake tests -nomake docs \
	-qpa \
	-qt-sql-sqlite \
	-no-gui -no-widgets -no-opengl -no-accessibility -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit \
	-shared -release \
	-v && \
make -j10 \
)