#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OSMAND_ARCHITECTURES_SET=($*)

if [[ -z "$ANDROID_SDK" ]]; then
	echo "ANDROID_SDK is not set"
	exit 1
fi
if [ ! -d "$ANDROID_SDK" ]; then
	echo "ANDROID_SDK is set incorrectly"
	exit 1
fi
if [[ -z "$ANDROID_NDK" ]]; then
	echo "ANDROID_NDK is not set"
	exit 1
fi
if [ ! -d "$ANDROID_NDK" ]; then
	echo "ANDROID_NDK is set incorrectly"
	exit 1
fi

QTBASE_CONFIGURATION=$(echo "
	-xplatform android-g++ $ANDROID_NDK_TOOLCHAIN
	-release -opensource -confirm-license -c++11 -static -largefile -no-accessibility -qt-sql-sqlite
	-no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
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

# Function: makeFlavor(name, arch, platform, configuration)
makeFlavor()
{
	local name=$1
	local arch=$2
	local platform=$3
	local configuration=$4
	
	local path="$SRCLOC/upstream.patched.$name"
	
	# Configure
	if [ ! -d "$path" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$path"
		export ANDROID_API_VERSION=android-$platform
		(cd "$path" && ./configure -android-ndk-platform android-$platform -android-arch $arch $configuration)
		retcode=$?
		if [ $retcode -ne 0 ]; then
			echo "Failed to configure 'qtbase-android' for '$name', aborting..."
			rm -rf "$path"
			exit $retcode
		fi
	fi
	
	# Build
	(cd "$path" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	retcode=$?
	if [ $retcode -ne 0 ]; then
		echo "Failed to build 'qtbase-android' for '$name', aborting..."
		rm -rf "$path"
		exit $retcode
	fi
}

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv5 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	makeFlavor "armeabi.static" "armeabi" "8" "$QTBASE_CONFIGURATION"
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7 ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7-neon ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	makeFlavor "armeabi-v7a.static" "armeabi-v7a" "8" "$QTBASE_CONFIGURATION"
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	makeFlavor "x86.static" "x86" "9" "$QTBASE_CONFIGURATION"
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ mips ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	makeFlavor "mips.static" "mips" "9" "$QTBASE_CONFIGURATION"
fi
