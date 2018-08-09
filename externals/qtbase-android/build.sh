#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

# Cleanup environment
cleanupEnvironment

# Verify input
targetOS=$1
compiler=$2
targetArch=$3
if [[ "$targetOS" != "android" ]]; then
	echo "'android' is the only supported target, while '${targetOS}' was specified"
	exit 1
fi
if [[ "$compiler" != "gcc" ]]; then
	echo "'gcc' is the only supported compilers, while '${compiler}' was specified"
	exit 1
fi
if [[ "$targetArch" != "armeabi" ]] && [[ "$targetArch" != "armeabi-v7a" ]] && [[ "$targetArch" != "x86" ]] && [[ "$targetArch" != "mips" ]]; then
	echo "'armeabi', 'armeabi-v7a', 'x86', 'mips' are the only supported target architectures, while '${targetArch}' was specified"
	exit 1
fi
echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"

# Verify environment
if [[ -z "$ANDROID_SDK" ]]; then
	echo "ANDROID_SDK is not set"
	exit 1
fi
if [[ ! -d "$ANDROID_SDK" ]]; then
	echo "ANDROID_SDK '${ANDROID_SDK}' is set incorrectly"
	exit 1
fi
export ANDROID_SDK_ROOT=$ANDROID_SDK
echo "Using ANDROID_SDK '${ANDROID_SDK}'"

if [[ -z "$ANDROID_NDK" ]]; then
	echo "ANDROID_NDK is not set"
	exit 1
fi
if [[ ! -d "$ANDROID_NDK" ]]; then
	echo "ANDROID_NDK '${ANDROID_NDK}' is set incorrectly"
	exit 1
fi
export ANDROID_NDK_ROOT=$ANDROID_NDK
echo "Using ANDROID_NDK '${ANDROID_NDK}'"

if [[ "$(uname -a)" =~ Linux ]]; then
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
else
	echo "'$(uname -a)' host is not supported"
	exit 1
fi
if [[ -z "$ANDROID_SDK" ]]; then
	echo "ANDROID_NDK '${ANDROID_NDK}' contains no valid host prebuilt tools"
	exit 1
fi
echo "Using ANDROID_NDK_HOST '${ANDROID_NDK_HOST}'"

export ANDROID_NDK_PLATFORM=android-14
if [[ ! -d "${ANDROID_NDK}/platforms/${ANDROID_NDK_PLATFORM}" ]]; then
	echo "Platform '${ANDROID_NDK}/platforms/${ANDROID_NDK_PLATFORM}' does not exist"
	exit 1
fi
echo "Using ANDROID_NDK_PLATFORM '${ANDROID_NDK_PLATFORM}'"

export ANDROID_TARGET_ARCH=$targetArch
targetArchFamily=""
if [[ "$targetArch"=="armeabi" ]] || [[ "$targetArch"=="armeabi-v7a" ]]; then
	targetArchFamily="arm"
elif [[ "$targetArch"=="x86" ]]; then
	targetArchFamily="x86"
elif [[ "$targetArch"=="mips" ]]; then
	targetArchFamily="mips"
fi
if [[ ! -d "${ANDROID_NDK}/platforms/${ANDROID_NDK_PLATFORM}/arch-${targetArchFamily}" ]]; then
	echo "Architecture headers '${ANDROID_NDK}/platforms/${ANDROID_NDK_PLATFORM}/arch-${targetArchFamily}' does not exist"
	exit 1
fi
echo "Using ANDROID_TARGET_ARCH '${ANDROID_TARGET_ARCH}'"

if [[ "$compiler" == "gcc" ]]; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=4.9
fi
echo "Using ANDROID_NDK_TOOLCHAIN_VERSION '${ANDROID_NDK_TOOLCHAIN_VERSION}'"

TOOLCHAIN_PATH=""
if [[ "$targetArch"=="armeabi" ]] || [[ "$targetArch"=="armeabi-v7a" ]]; then
	TOOLCHAIN_PATH="${ANDROID_NDK}/toolchains/arm-linux-androideabi-${ANDROID_NDK_TOOLCHAIN_VERSION}"
elif [[ "$targetArch"=="x86" ]]; then
	TOOLCHAIN_PATH="${ANDROID_NDK}/toolchains/x86-${ANDROID_NDK_TOOLCHAIN_VERSION}"
elif [[ "$targetArch"=="mips" ]]; then
	TOOLCHAIN_PATH="${ANDROID_NDK}/toolchains/mipsel-linux-android-${ANDROID_NDK_TOOLCHAIN_VERSION}"
fi
if [[ ! -d "$TOOLCHAIN_PATH" ]]; then
	echo "Toolchain at '$TOOLCHAIN_PATH' not found"
	exit 1
fi
echo "Using toolchain '${TOOLCHAIN_PATH}'"

# Prepare configuration
QTBASE_CONFIGURATION=$(echo "
	-release -opensource -confirm-license -c++std c++11 -no-accessibility -qt-sql-sqlite
	-no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
	-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
	-no-opengl -no-evdev
	-v
" | tr '\n' ' ')
if [[ "$compiler"=="gcc" ]]; then
	QTBASE_CONFIGURATION="-xplatform android-g++ ${QTBASE_CONFIGURATION}"
fi
if [[ "$targetArch"=="mips" ]]; then
	QTBASE_CONFIGURATION="${QTBASE_CONFIGURATION} -no-use-gold-linker"
fi

# Function: makeFlavor(type)
makeFlavor()
{
	local type=$1

	local name="${compiler}-${targetArch}.${type}"
	local path="$SRCLOC/upstream.patched.${targetOS}.${name}"
	
	# Configure
	if [ ! -d "$path" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$path"
		(cd "$path" && ./configure $QTBASE_CONFIGURATION -$type -prefix $path)
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

makeFlavor "shared"
makeFlavor "static"
