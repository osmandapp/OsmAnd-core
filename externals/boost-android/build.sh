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
if ls $ANDROID_NDK/toolchains/*-4.8 &> /dev/null; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=4.8
elif ls $ANDROID_NDK/toolchains/*-4.7 &> /dev/null; then
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
if [[ "$(uname -a)" =~ Cygwin ]]; then
	echo "Building for Android under Cygwin is not supported"
	exit 1
fi

ARM_TOOLCHAIN_PREFIX="arm-linux-androideabi"
MIPS_TOOLCHAIN_PREFIX="mipsel-linux-android"
X86_TOOLCHAIN_PREFIX="x86"

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv5 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi.static"
		export ANDROID_NDK_PLATFORM=android-8
		cat "$SRCLOC/armv5.jam" > "$SRCLOC/upstream.patched.armeabi.static/tools/build/v2/user-config.jam"
		(cd "$SRCLOC/upstream.patched.armeabi.static" && \
			./bootstrap.sh)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi.static" && \
		./b2 variant=debug,release --layout=versioned toolset=gcc-$ANDROID_NDK_TOOLCHAIN_VERSION target-os=linux threading=multi link=static stage)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a.static"
		export ANDROID_NDK_PLATFORM=android-8
		cat "$SRCLOC/armv7a.jam" > "$SRCLOC/upstream.patched.armeabi-v7a.static/tools/build/v2/user-config.jam"
		(cd "$SRCLOC/upstream.patched.armeabi-v7a.static" && \
			./bootstrap.sh)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a.static" && \
		./b2 variant=debug,release --layout=versioned toolset=gcc-$ANDROID_NDK_TOOLCHAIN_VERSION target-os=linux threading=multi link=static stage)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ arm ]] || [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ armv7-neon ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.armeabi-v7a-neon.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.armeabi-v7a-neon.static"
		export ANDROID_NDK_PLATFORM=android-8
		cat "$SRCLOC/armv7a-neon.jam" > "$SRCLOC/upstream.patched.armeabi-v7a-neon.static/tools/build/v2/user-config.jam"
		(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon.static" && \
			./bootstrap.sh)
	fi
	(cd "$SRCLOC/upstream.patched.armeabi-v7a-neon.static" && \
		./b2 variant=debug,release --layout=versioned toolset=gcc-$ANDROID_NDK_TOOLCHAIN_VERSION target-os=linux threading=multi link=static stage)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.x86.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.x86.static"
		export ANDROID_NDK_PLATFORM=android-9
		cat "$SRCLOC/x86.jam" > "$SRCLOC/upstream.patched.x86.static/tools/build/v2/user-config.jam"
		(cd "$SRCLOC/upstream.patched.x86.static" && \
			./bootstrap.sh)
	fi
	(cd "$SRCLOC/upstream.patched.x86.static" && \
		./b2 variant=debug,release --layout=versioned toolset=gcc-$ANDROID_NDK_TOOLCHAIN_VERSION target-os=linux threading=multi link=static stage)
fi

if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ mips ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.mips.static" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.mips.static"
		export ANDROID_NDK_PLATFORM=android-9
		cat "$SRCLOC/mips.jam" > "$SRCLOC/upstream.patched.mips.static/tools/build/v2/user-config.jam"
		(cd "$SRCLOC/upstream.patched.mips.static" && \
			./bootstrap.sh)
	fi
	(cd "$SRCLOC/upstream.patched.mips.static" && \
		./b2 variant=debug,release --layout=versioned toolset=gcc-$ANDROID_NDK_TOOLCHAIN_VERSION target-os=linux threading=multi link=static stage)
fi
