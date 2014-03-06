#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REAL_SRCLOC=`readlink -f $SRCLOC`

OSMAND_CORE_ROOT_RELATIVE="../.."
OSMAND_CORE_ROOT="$REAL_SRCLOC/$OSMAND_CORE_ROOT_RELATIVE"

HOST=`uname -a`
if [[ "$HOST" =~ "Linux" ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
elif [[ "$HOST" =~ "Darwin" ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi
elif [[ "$HOST" =~ "Cygwin" ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
else
	echo "no"
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=""
	fi
fi

export OSMAND_CORE_ROOT
export OSMAND_CORE_ROOT_RELATIVE
echo "Using OsmAnd Core root: $OSMAND_CORE_ROOT"
echo "Using OsmAnd Core root (relative): $OSMAND_CORE_ROOT_RELATIVE"

echo "Will execute from $REAL_SRCLOC"
echo "Using $OSMAND_BUILD_CPU_CORES_NUM core(s)"
(cd $REAL_SRCLOC && ndk-build -j$OSMAND_BUILD_CPU_CORES_NUM)
