#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)
OSMAND_ARCHITECTURES_SET=($*)

if [[ -z "$ANDROID_SDK" ]]; then
	echo "ANDROID_SDK is not set"
	exit
fi
if [ ! -d "$ANDROID_SDK" ]; then
	echo "ANDROID_SDK is set incorrectly"
	exit
fi
if [[ -z "$ANDROID_NDK" ]]; then
	echo "ANDROID_NDK is not set"
	exit
fi
if [ ! -d "$ANDROID_NDK" ]; then
	echo "ANDROID_NDK is set incorrectly"
	exit
fi

QTBASE_CONFIGURATION=$(echo "
	-xplatform android-g++ $ANDROID_NDK_TOOLCHAIN
	-release -opensource -confirm-license -c++11 -static -largefile -no-accessibility -qt-sql-sqlite
	-no-javascript-jit -no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
	-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
	-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
	-v
" | tr '\n' ' ')

export ANDROID_SDK_ROOT=$ANDROID_SDK
export ANDROID_NDK_ROOT=$ANDROID_NDK
pushd $ANDROID_NDK
if ls toolchains/*-4.8 &> /dev/null; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=4.8
elif ls toolchains/*-4.7 &> /dev/null; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=4.7
fi
popd
if [ -n "$ANDROID_NDK_TOOLCHAIN_VERSION" ]; then
	echo "Using $ANDROID_NDK_TOOLCHAIN_VERSION toolchain version"
	ANDROID_NDK_TOOLCHAIN="-android-toolchain-version $ANDROID_NDK_TOOLCHAIN_VERSION"
else
	echo "Using auto-detected toolchain version"
	ANDROID_NDK_TOOLCHAIN=""
fi

if [[ "$(uname -a)" =~ Linux ]]; then
	MAKE=make
	if [[ "$(uname -m)" == x86_64 ]] && [ -d "$ANDROID_NDK/prebuilt/linux-x86_64" ]; then
		export ANDROID_NDK_HOST=linux-x86_64
	elif [ -d "$ANDROID_NDK/prebuilt/linux-x86" ]; then
		export ANDROID_NDK_HOST=linux-x86
	else
		export ANDROID_NDK_HOST=linux
	fi

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
elif [[ "$(uname -a)" =~ Darwin ]]; then
	MAKE=make
	if [[ "$(uname -m)" == x86_64 ]] && [ -d "$ANDROID_NDK/prebuilt/darwin-x86_64" ]; then
		export ANDROID_NDK_HOST=darwin-x86_64
	elif [ -d "$ANDROID_NDK/prebuilt/darwin-x86" ]; then
		export ANDROID_NDK_HOST=darwin-x86
	else
		export ANDROID_NDK_HOST=darwin
	fi

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi
elif [[ "$(uname -a)" =~ Cygwin ]]; then
	echo "Building for Android under Cygwin is not supported, run build.sh from MinGW"
	exit 1
elif [[ "$(uname -a)" =~ MINGW ]]; then
	MAKE=mingw32-make
	QTBASE_CONFIGURATION="-platform win32-g++ $QTBASE_CONFIGURATION"
	if [ -d "$ANDROID_NDK/prebuilt/windows-x86_64" ]; then
		export ANDROID_NDK_HOST=windows-x86_64
	elif [ -d "$ANDROID_NDK/prebuilt/windows-x86" ]; then
		export ANDROID_NDK_HOST=windows-x86
	else
		export ANDROID_NDK_HOST=windows
	fi

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=$NUMBER_OF_PROCESSORS
	fi
else
	echo "'$(uname -a)' is not recognized"
	exit 1
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv5 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi.static"
		export ANDROID_TARGET_ARCH=armeabi
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi.static" && \
			./configure $QTBASE_CONFIGURATION \
			-no-neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi.static" && $MAKE -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a.static"
		export ANDROID_TARGET_ARCH=armeabi-v7a
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi-v7a.static" && \
			./configure $QTBASE_CONFIGURATION \
			-no-neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a.static" && $MAKE -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7-neon ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a-neon.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a-neon.static"
		export ANDROID_TARGET_ARCH=armeabi-v7a
		export ANDROID_NDK_PLATFORM=android-8
		(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon.static" && \
			./configure $QTBASE_CONFIGURATION \
			-qtlibinfix _neon)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon.static" && $MAKE -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.x86.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.x86.static"
		export ANDROID_TARGET_ARCH=x86
		export ANDROID_NDK_PLATFORM=android-9
		(cd "$SRCLOC/upstream.patched.x86.static" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.x86.static" && $MAKE -j$OSMAND_BUILD_CPU_CORES_NUM)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ mips ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.mips.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.mips.static"
		export ANDROID_TARGET_ARCH=mips
		export ANDROID_NDK_PLATFORM=android-9
		(cd "$SRCLOC/upstream.patched.mips.static" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.mips.static" && $MAKE -j$OSMAND_BUILD_CPU_CORES_NUM)
fi
