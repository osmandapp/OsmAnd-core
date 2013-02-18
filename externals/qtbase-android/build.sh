#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $(dirname "${BASH_SOURCE[0]}") )

if [ ! -d "$ANDROID_SDK" ]; then
	echo "ANDROID_SDK is not set"
	exit
fi
if [ ! -d "$ANDROID_NDK" ]; then
	echo "ANDROID_NDK is not set"
	exit
fi
export ANDROID_SDK_ROOT=$ANDROID_SDK
export ANDROID_NDK_ROOT=$ANDROID_NDK
export ANDROID_NDK_TOOLCHAIN_VERSION=4.7
export ANDROID_NDK_PLATFORM=android-8

QTBASE_CONFIGURATION = \
	-opensource -confirm-license -xplatform android-g++ \
	-nomake examples -nomake demos -nomake tests -nomake docs \
	-qpa \
	-qt-sql-sqlite \
	-no-gui -no-widgets -no-opengl -no-accessibility -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit \
	-shared -release \
	-v
echo "nrpoc = "`nproc`
if [ ! -d "$SRCLOC/upstream.patched.armeabi" ]; then
	cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi"
	export ANDROID_TARGET_ARCH=armeabi
	(cd "$SRCLOC/upstream.patched.armeabi" && \
		./configure $QTBASE_CONFIGURATION && \
		make -j`nproc`)
fi

if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a" ]; then
	cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a"
	export ANDROID_TARGET_ARCH=armeabi-v7a
	(cd "$SRCLOC/upstream.patched.armeabi-v7a" && \
		./configure $QTBASE_CONFIGURATION && \
		make -j`nproc`)
fi

if [ ! -d "$SRCLOC/upstream.patched.x86" ]; then
	cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.x86"
	export ANDROID_TARGET_ARCH=x86
	(cd "$SRCLOC/upstream.patched.x86" && \
		./configure $QTBASE_CONFIGURATION && \
		make -j`nproc`)
fi

if [ ! -d "$SRCLOC/upstream.patched.mips" ]; then
	cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.mips"
	export ANDROID_TARGET_ARCH=mips
	(cd "$SRCLOC/upstream.patched.mips" && \
		./configure $QTBASE_CONFIGURATION && \
		make -j`nproc`)
fi
