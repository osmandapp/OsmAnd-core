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

for external in ${OSMAND_EXTERNALS_SET[@]/#/$SRCLOC/} ; do
	if [ -d "$external" ]; then
		cleanExternal "$external"
	fi
done
