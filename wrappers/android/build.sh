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
	echo "Specify one of build types as first argument: debug, release"
	exit 1
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
	
	local bakedDir="${PROJECTS_ROOT}/baked/${arch}-android-gcc-${BUILD_TYPE}.make"
	echo "Building SDK for '$arch':"
	if [[ ! -d "$bakedDir" ]]; then
		echo -e "\tBaking in '$bakedDir"
		"${PROJECTS_ROOT}/build/${arch}-android-gcc.sh" $BUILD_TYPE
		retcode=$?
		if [[ $retcode -ne 0 ]]; then
			echo "Failed to bake ($retcode), aborting..."
			rm -rf "$bakedDir"
			exit $retcode
		fi
	else
		echo -e "\tFound baked in '$bakedDir"
	fi
	
	(cd "$bakedDir" && make -j$OSMAND_BUILD_CPU_CORES_NUM OsmAndCoreJNI)
	if [[ $retcode -ne 0 ]]; then
		echo "Failed to build ($retcode), aborting..."
		rm -rf "$bakedDir"
		exit $retcode
	fi
}

buildArch "armeabi"
if [[ $retcode -ne 0 ]]; then
	echo "Failed!"
	exit $retcode
fi

buildArch "armeabi-v7a"
if [[ $retcode -ne 0 ]]; then
	echo "Failed!"
	exit $retcode
fi

buildArch "x86"
if [[ $retcode -ne 0 ]]; then
	echo "Failed!"
	exit $retcode
fi

buildArch "mips"
if [[ $retcode -ne 0 ]]; then
	echo "Failed!"
	exit $retcode
fi
