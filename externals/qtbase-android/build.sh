#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)
OSMAND_ARCHITECTURES_SET=($*)

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
if [ -d "$ANDROID_NDK/toolchains/arm-linux-androideabi-4.8" ]; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=4.8
fi
if [ -d "$ANDROID_NDK/toolchains/arm-linux-androideabi-4.7" ]; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=4.7
fi
if [[ "$(uname -a)" =~ Linux ]]; then
	if [[ "$(uname -m)" == x86_64 ]] && [ -d "$ANDROID_NDK/prebuilt/linux-x86_64" ]; then
		export ANDROID_NDK_HOST=linux-x86_64;
	elif [ -d "$ANDROID_NDK/prebuilt/linux-x86" ]; then
		export ANDROID_NDK_HOST=linux-x86
	else
		export ANDROID_NDK_HOST=linux
	fi

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
fi
if [[ "$(uname -a)" =~ Darwin ]]; then
	if [[ "$(uname -m)" == x86_64 ]] && [ -d "$ANDROID_NDK/prebuilt/darwin-x86_64" ]; then
		export ANDROID_NDK_HOST=darwin-x86_64;
	elif [ -d "$ANDROID_NDK/prebuilt/darwin-x86" ]; then
		export ANDROID_NDK_HOST=darwin-x86
	else
		export ANDROID_NDK_HOST=darwin
	fi

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi
fi

QTBASE_CONFIGURATION=$(echo "
	-xplatform android-g++
	-release -opensource -confirm-license -c++11 -static -largefile -no-accessibility -qt-sql-sqlite
	-no-javascript-jit -no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
	-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
	-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
	-v
" | tr '\n' ' ')

if [[ "$(uname -a)" =~ Cygwin ]]; then
	echo "Building for Android under Cygwin is not supported"
	exit 1
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv5 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi"
		export ANDROID_TARGET_ARCH=armeabi
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi" && \
			./configure $QTBASE_CONFIGURATION \
			-no-neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a"
		export ANDROID_TARGET_ARCH=armeabi-v7a
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi-v7a" && \
			./configure $QTBASE_CONFIGURATION \
			-no-neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7-neon ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a-neon" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a-neon"
		export ANDROID_TARGET_ARCH=armeabi-v7a
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon" && \
			./configure $QTBASE_CONFIGURATION \
			-qtlibinfix _neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.x86" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.x86"
		export ANDROID_TARGET_ARCH=x86
		export ANDROID_NDK_PLATFORM=android-9
		(cd "$SRCLOC/upstream.patched.x86" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.x86" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ mips ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.mips" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.mips"
		export ANDROID_TARGET_ARCH=mips
		export ANDROID_NDK_PLATFORM=android-9
		(cd "$SRCLOC/upstream.patched.mips" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.mips" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
fi
