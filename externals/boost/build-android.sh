#!/bin/bash

if [[ "$compiler" != "clang" ]]; then
	echo "'clang' is the only supported compilers, while '${compiler}' was specified"
	exit 1
fi
if [[ "$targetArch" != "arm64-v8a" ]] && [[ "$targetArch" != "armeabi-v7a" ]] && [[ "$targetArch" != "x86" ]] && [[ "$targetArch" != "x86_64" ]]; then
	echo "'arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64' are the only supported target architectures, while '${targetArch}' was specified"
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

if [[ "$compiler" == "clang" ]]; then
	export ANDROID_NDK_TOOLCHAIN_VERSION=clang
fi
echo "Using ANDROID_NDK_TOOLCHAIN_VERSION '${ANDROID_NDK_TOOLCHAIN_VERSION}'"


export ANDROID_NDK_PLATFORM=21


# Configuration
BOOST_CONFIGURATION=$(echo "
	--layout=system
	--with-thread
	--with-atomic
	--with-chrono
	toolset=clang
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
		./bootstrap.sh --with-toolset=clang)
	retcode=$?
	if [ $retcode -ne 0 ]; then
		echo "Failed to configure 'Boost' for '${targetOS}.${compiler}-${targetArch}', aborting..."
		rm -rf "$path"
		exit $retcode
	fi
fi	

echo "Using '${targetOS}.${compiler}-${targetArch}.jam'"
cat "$SRCLOC/targets/${targetOS}.${compiler}-${targetArch}.jam" > "$STATIC_BUILD_PATH/project-config.jam"
(cd "$STATIC_BUILD_PATH" && \
	./b2 $BOOST_CONFIGURATION -j $OSMAND_BUILD_CPU_CORES_NUM)
retcode=$?
if [ $retcode -ne 0 ]; then
	echo "Failed to build 'Boost' for '${targetOS}.${compiler}-${targetArch}', aborting..."
	rm -rf "$path"
	exit $retcode
fi

exit 0
