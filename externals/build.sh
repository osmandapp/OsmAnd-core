#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OSMAND_EXTERNALS_SET=($*)
if [ -z "$OSMAND_EXTERNALS_SET" ]; then
	OSMAND_EXTERNALS_SET=*
fi

if [ -z "$OSMAND_ARCHITECTURES_SET" ]; then
	OSMAND_ARCHITECTURES_SET=(x64 x86 mips arm armv5 armv7 armv7-neon)
fi

for external in ${OSMAND_EXTERNALS_SET[@]/#/$SRCLOC/} ; do
	if [ -f "$external/build.sh" ] && [ -d "$external/upstream.patched" ]; then
		echo "Building '"$(basename "$external")"'..."
		"$external/build.sh" ${OSMAND_ARCHITECTURES_SET[*]}
	fi
done