#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REAL_SRCLOC=`readlink -f $SRCLOC`

OSMAND_CORE_ROOT_RELATIVE="../.."
OSMAND_CORE_ROOT="$REAL_SRCLOC/$OSMAND_CORE_ROOT_RELATIVE"

if [[ "$(uname -a)" =~ Linux ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
elif [[ "$(uname -a)" =~ Darwin ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi
elif [[ "$(uname -a)" =~ Cygwin ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
else
	OSMAND_BUILD_CPU_CORES_NUM=
fi

export OSMAND_CORE_ROOT
export OSMAND_CORE_ROOT_RELATIVE
echo "Using OsmAnd Core root: $OSMAND_CORE_ROOT"
echo "Using OsmAnd Core root (relative): $OSMAND_CORE_ROOT_RELATIVE"

echo "Will execute from $REAL_SRCLOC"
(cd $REAL_SRCLOC && ndk-build -j$OSMAND_BUILD_CPU_CORES_NUM)
