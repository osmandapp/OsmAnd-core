#!/bin/bash

SCRIPT_LOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $(dirname "${BASH_SOURCE[0]}") )

"$SCRIPT_LOC/externals/configure.sh"
SRCLOC="$SCRIPT_LOC/externals/qtbase-android"
QTBASE_CONFIGURATION=\
"-opensource -confirm-license -xplatform android-g++ "\
"-nomake examples -nomake demos -nomake tests -nomake docs "\
"-qt-sql-sqlite "\
"-no-gui -no-widgets -no-opengl -no-accessibility -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit "\
"-c++11 -shared -release "\
"-v"

if [[ "$(uname -a)" == *Linux* ]]; then
	if [[ "$(uname -m)" == x86_64 ]] && [ -d $ANDROID_NDK/prebuilt/linux-x86_64 ]; then
		export ANDROID_NDK_HOST=linux-x86_64;
	fi
fi

if [ -n "$BUILD_ALL" ] || [ -n "$OSMAND_ARM_ONLY" ] || [ -n "$OSMAND_ARMv5_ONLY" ]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi"
		export ANDROID_TARGET_ARCH=armeabi
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi" && \
			./configure $QTBASE_CONFIGURATION \
			-no-neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi" && make -j`nproc`)
fi

if [ -n "$BUILD_ALL" ] || [ -n "$OSMAND_ARM_ONLY" ] || [ -n "$OSMAND_ARMv7a_ONLY" ]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a"
		export ANDROID_TARGET_ARCH=armeabi-v7a
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi-v7a" && \
			./configure $QTBASE_CONFIGURATION \
			-no-neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a" && make -j`nproc`)

	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a-neon" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a-neon"
		export ANDROID_TARGET_ARCH=armeabi-v7a
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon" && \
			./configure $QTBASE_CONFIGURATION \
			-qtlibinfix _neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon" && make -j`nproc`)
fi

if [ -n "$BUILD_ALL" ] || [ -n "$OSMAND_X86_ONLY" ]; then
	if [ ! -d "$SRCLOC/upstream.patched.x86" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.x86"
		export ANDROID_TARGET_ARCH=x86
		export ANDROID_NDK_PLATFORM=android-9
		(cd "$SRCLOC/upstream.patched.x86" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.x86" && make -j`nproc`)
fi

if [ -n "$BUILD_ALL" ] || [ -n "$OSMAND_MIPS_ONLY" ]; then
	if [ ! -d "$SRCLOC/upstream.patched.mips" ]; then
	cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.mips"
		export ANDROID_TARGET_ARCH=mips
		export ANDROID_NDK_PLATFORM=android-9
		(cd "$SRCLOC/upstream.patched.mips" && \
		./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.mips" && make -j`nproc`)
fi
