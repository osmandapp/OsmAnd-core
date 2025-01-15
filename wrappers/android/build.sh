#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

PROJECTS_ROOT="$SRCLOC/../../.."
BUILD_TYPE=$1
if [[ "$BUILD_TYPE" != "debug" ]] && [[ "$BUILD_TYPE" != "release" ]]; then
	echo "None of build types (debug, release) were specified as first argument, going to build all"

	"${BASH_SOURCE[0]}" debug && "${BASH_SOURCE[0]}" release
	exit $?
fi
echo "Build type: $BUILD_TYPE"

if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
	if [[ "$(uname -a)" =~ Linux ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	elif [[ "$(uname -a)" =~ Darwin ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi
fi
echo "$OSMAND_BUILD_CPU_CORES_NUM build threads"

buildArch()
{
	local arch=$1
	
	local bakedDir="${PROJECTS_ROOT}/baked/${arch}-android-clang-${BUILD_TYPE}.make"
	echo "Building SDK for '$arch':"
	if [[ ! -d "$bakedDir" ]]; then
		echo -e "\tBaking in '$bakedDir"
		"${PROJECTS_ROOT}/build/${arch}-android-clang.sh" $BUILD_TYPE
		retcode=$?
		if [[ $retcode -ne 0 ]]; then
			echo "Failed to bake ($retcode), aborting..."
			rm -rf "$bakedDir"
			exit $retcode
		fi
	else
		echo -e "\tFound baked in '$bakedDir"
	fi
	
	(cd "$bakedDir" && make -j$OSMAND_BUILD_CPU_CORES_NUM OsmAndCoreWithJNI)
	retcode=$?
	if [[ $retcode -ne 0 ]]; then
		echo "Failed to build ($retcode), aborting..."
		rm -rf "$bakedDir"
		exit $retcode
	fi
}

buildArch "arm64-v8a"
retcode=$?
if [[ $retcode -ne 0 ]]; then
	echo "buildArch(arm64-v8a) failed with $retcode, exiting..."
	exit $retcode
fi

#exit 0; # for testing purposes

buildArch "armeabi-v7a"
retcode=$?
if [[ $retcode -ne 0 ]]; then
        echo "buildArch(armeabi-v7a) failed with $retcode, exiting..."
        exit $retcode
fi

buildArch "x86"
retcode=$?
if [[ $retcode -ne 0 ]]; then
	echo "buildArch(x86) failed with $retcode, exiting..."
	exit $retcode
fi

buildArch "x86_64"
retcode=$?
if [[ $retcode -ne 0 ]]; then
        echo "buildArch(x86_64) failed with $retcode, exiting..."
        exit $retcode
fi
