#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/functions.sh"

OSMAND_EXTERNALS_SET=($*)
if [ -z "$OSMAND_EXTERNALS_SET" ]; then
	OSMAND_EXTERNALS_SET=*
fi

if [ -z "$OSMAND_ARCHITECTURES_SET" ]; then
	OSMAND_ARCHITECTURES_SET=(x64 x86 mips arm armv5 armv7 armv7-neon)
fi

for external in ${OSMAND_EXTERNALS_SET[@]/#/$SRCLOC/} ; do
	if [ -f "$external/build.sh" ] && [ -d "$external/upstream.patched" ]; then
		"$external/build.sh" ${OSMAND_ARCHITECTURES_SET[*]}
		retcode=$?
		if [ $retcode -ne 0 ]; then
			echo "Failed to build external in '$external', aborting..."
			exit $retcode
		fi
	fi
done
