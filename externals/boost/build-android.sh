#!/bin/bash

if [[ "$compiler" != "gcc" ]]; then
	echo "'gcc' is the only supported compilers, while '${compiler}' was specified"
	exit 1
fi
if [[ "$targetArch" != "armeabi" ]] && [[ "$targetArch" != "armeabi-v7a" ]] && [[ "$targetArch" != "x86" ]] && [[ "$targetArch" != "mips" ]]; then
	echo "'armeabi', 'armeabi-v7a', 'x86', 'mips' are the only supported target architectures, while '${targetArch}' was specified"
	exit 1
fi
echo "Going to build Boost for ${targetOS}/${compiler}/${targetArch}"

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

# Configuration
BOOST_CONFIGURATION=$(echo "
	--layout=versioned
	--with-thread
	toolset=gcc-android
	target-os=linux
	threading=multi
	link=static
	runtime-link=shared
	variant=release
	threadapi=pthread
	stage
" | tr '\n' ' ')

# Configure & Build static
STATIC_BUILD_PATH="$SRCLOC/upstream.patched.${targetOS}.${compiler}-${targetArch}.static"
if [ ! -d "$STATIC_BUILD_PATH" ]; then
	cp -rpf "$SRCLOC/upstream.patched" "$STATIC_BUILD_PATH"

	(cd "$STATIC_BUILD_PATH" && \
		./bootstrap.sh)
	retcode=$?
	if [ $retcode -ne 0 ]; then
		echo "Failed to configure 'Boost' for '${targetOS}.${compiler}-${targetArch}', aborting..."
		rm -rf "$path"
		exit $retcode
	fi
	
	echo "Using '${targetOS}.${compiler}-${targetArch}.jam'"
	cat "$SRCLOC/targets/${targetOS}.${compiler}-${targetArch}.jam" > "$STATIC_BUILD_PATH/project-config.jam"
fi
(cd "$STATIC_BUILD_PATH" && \
	./b2 $BOOST_CONFIGURATION -j $OSMAND_BUILD_CPU_CORES_NUM)
retcode=$?
if [ $retcode -ne 0 ]; then
	echo "Failed to build 'Boost' for '${targetOS}.${compiler}-${targetArch}', aborting..."
	rm -rf "$path"
	exit $retcode
fi

exit 0
