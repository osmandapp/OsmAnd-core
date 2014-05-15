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

# Check if everything needs reconfiguring
if [ -f "$SRCLOC/stamp" ]; then
	lastStamp=""
	if [ -f "$SRCLOC/.stamp" ]; then
		lastStamp=$(cat "$SRCLOC/.stamp")
	fi
	currentStamp=$(cat "$SRCLOC/stamp")
	echo "Last global stamp:    "$lastStamp
	echo "Current global stamp: "$currentStamp
	if [ "$lastStamp" != "$currentStamp" ]; then
		echo "Stamps differ, will clean external '$externalName'..."
		"$SRCLOC/clean.sh"
		cp "$SRCLOC/stamp" "$SRCLOC/.stamp"
	fi
fi

for external in ${OSMAND_EXTERNALS_SET[@]/#/$SRCLOC/} ; do
	echo "Looking for external in '$external'..."
	if [ -d "$external" ] && [ -e "$external/configure.sh" ]; then
		"$external/configure.sh"
		retcode=$?
		if [ $retcode -ne 0 ]; then
			echo "Failed to configure external in '$external', aborting..."
			exit $retcode
		fi
	fi
done
