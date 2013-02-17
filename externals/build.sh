#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for external in $SRCLOC/* ; do
	if [ -f "$external/build.sh" ];	then
		echo "Building '"$(basename "$external")"'..."
		"$external/build.sh"
	fi
done