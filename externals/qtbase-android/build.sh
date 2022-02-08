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
if [[ "$compiler" != "clang" ]]; then
	echo "'clang' is the only supported compilers, while '${compiler}' was specified"
	exit 1
fi
if [[ "$targetArch" != "arm64-v8a" ]] && [[ "$targetArch" != "armeabi-v7a" ]] && [[ "$targetArch" != "x86" ]] && [[ "$targetArch" != "x86_64" ]]; then
	echo "'arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64' are the only supported target architectures, while '${targetArch}' was specified"
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
	if [[ "$(uname -m)" == x86_64 ]] && [ -d "$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64" ]; then
		export ANDROID_NDK_HOST=linux-x86_64
	elif [ -d "$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86" ]; then
		export ANDROID_NDK_HOST=linux-x86
	else
		export ANDROID_NDK_HOST=linux
	fi

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
elif [[ "$(uname -a)" =~ Darwin ]]; then
	if [[ "$(uname -m)" == x86_64 || "$(uname -m)" == arm64 ]] && [ -d "$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86_64" ]; then
		export ANDROID_NDK_HOST=darwin-x86_64
	elif [ -d "$ANDROID_NDK/toolchains/llvm/prebuilt/darwin-x86" ]; then
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
if [[ ! -d "$ANDROID_NDK/toolchains/llvm/prebuilt/$ANDROID_NDK_HOST" ]]; then
	echo "ANDROID_NDK_HOST '${ANDROID_NDK_HOST}' has been detected incorrectly or is unsupported"
	exit 1
fi
echo "Using ANDROID_NDK_HOST '${ANDROID_NDK_HOST}'"

export ANDROID_NDK_PLATFORM=android-21

# Prepare configuration
QTBASE_CONFIGURATION=$(echo "
	-release -opensource -confirm-license -c++std c++11 -no-accessibility -no-sql-sqlite
	-qt-zlib -no-zstd -no-gif -no-libpng -no-libjpeg -no-openssl -no-feature-gssapi -no-feature-sspi -qt-pcre
	-nomake tests -nomake examples -nomake tools -no-gui -no-widgets -no-cups -no-iconv -no-icu -no-dbus
	-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
	-no-use-gold-linker
	-v
" | tr '\n' ' ')

# Function: makeFlavor(type)
makeFlavor()
{
	local type=$1

	local name="${compiler}-${targetArch}.${type}"
	local path="$SRCLOC/upstream.patched.${targetOS}.${name}"
	
	# Configure
	if [ ! -d "$path" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$path"
		(cd "$path" && ./configure -xplatform android-clang -android-arch ${targetArch} $QTBASE_CONFIGURATION -$type -prefix $path)
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
