#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

if [ ! -d "$ANDROID_SDK" ]; then
	echo "ANDROID_SDK is not set"
	exit
fi
if [ ! -d "$ANDROID_NDK" ]; then
	echo "ANDROID_NDK is not set"
	exit
fi
export ANDROID_SDK_ROOT=`echo $ANDROID_SDK | sed 's/\\\\/\//g'`
export ANDROID_NDK_ROOT=`echo $ANDROID_NDK | sed 's/\\\\/\//g'`
export ANDROID_NDK_TOOLCHAIN_VERSION=4.7
if [[ "$(uname -a)" == *Cygwin* ]]; then
	if [[ "$(uname -a)" == *WOW64* ]] && [ -d $ANDROID_NDK/prebuilt/windows-x86_64 ]; then
		export ANDROID_NDK_HOST=windows-x86_64
	else
		export ANDROID_NDK_HOST=windows
	fi
fi
if [[ "$(uname -a)" == *Linux* ]]; then
	if [[ "$(uname -m)" == x86_64 ]] && [ -d $ANDROID_NDK/prebuilt/linux-x86_64 ]; then
		export ANDROID_NDK_HOST=linux-x86_64;
	elif [ -d $ANDROID_NDK/prebuilt/linux-x86 ]; then
		export ANDROID_NDK_HOST=linux-x86
	else
		export ANDROID_NDK_HOST=linux
	fi
fi
if [[ "$(uname -a)" == *Darwin* ]]; then
	if [[ "$(uname -m)" == x86_64 ]] && [ -d $ANDROID_NDK/prebuilt/darwin-x86_64 ]; then
		export ANDROID_NDK_HOST=darwin-x86_64;
	elif [ -d $ANDROID_NDK/prebuilt/darwin-x86 ]; then
		export ANDROID_NDK_HOST=darwin-x86
	else
		export ANDROID_NDK_HOST=darwin
	fi
fi

QTBASE_CONFIGURATION=\
"-opensource -confirm-license -xplatform android-g++ "\
"-nomake examples -nomake tools "\
"-qt-sql-sqlite "\
"-no-accessibility -no-gui -no-widgets -no-nis -no-opengl -no-kms -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit "\
"-c++11 -shared -release "\
"-v"

if [[ "$(uname -a)" == *Cygwin* ]]; then
	$QTBASE_CONFIGURATION="$QTBASE_CONFIGURATION -no-c++11"
fi

if [ ! -d "$SRCLOC/upstream.patched.armeabi" ]; then
	cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi"
	export ANDROID_TARGET_ARCH=armeabi
	export ANDROID_NDK_PLATFORM=android-8
	(cd "$SRCLOC/upstream.patched.armeabi" && \
		./configure $QTBASE_CONFIGURATION \
		-no-neon)
fi
(cd "$SRCLOC/upstream.patched.armeabi" && make -j`nproc`)

if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a" ]; then
	cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a"
	export ANDROID_TARGET_ARCH=armeabi-v7a
	export ANDROID_NDK_PLATFORM=android-8
	(cd "$SRCLOC/upstream.patched.armeabi-v7a" && \
		./configure $QTBASE_CONFIGURATION \
		-no-neon)
fi
(cd "$SRCLOC/upstream.patched.armeabi-v7a" && make -j`nproc`)

if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a-neon" ]; then
	cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a-neon"
	export ANDROID_TARGET_ARCH=armeabi-v7a
	export ANDROID_NDK_PLATFORM=android-8
	(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon" && \
		./configure $QTBASE_CONFIGURATION \
		-qtlibinfix _neon)
fi
(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon" && make -j`nproc`)

if [ ! -d "$SRCLOC/upstream.patched.x86" ]; then
	cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.x86"
	export ANDROID_TARGET_ARCH=x86
	export ANDROID_NDK_PLATFORM=android-9
	(cd "$SRCLOC/upstream.patched.x86" && \
		./configure $QTBASE_CONFIGURATION)
fi
(cd "$SRCLOC/upstream.patched.x86" && make -j`nproc`)

if [ ! -d "$SRCLOC/upstream.patched.mips" ]; then
	cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.mips"
	export ANDROID_TARGET_ARCH=mips
	export ANDROID_NDK_PLATFORM=android-9
	(cd "$SRCLOC/upstream.patched.mips" && \
		./configure $QTBASE_CONFIGURATION)
fi
(cd "$SRCLOC/upstream.patched.mips" && make -j`nproc`)
