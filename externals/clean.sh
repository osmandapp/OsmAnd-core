#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for external in $SRCLOC/* ; do
	if ls -1 $external/upstream.* >/dev/null 2>&1
	then
		echo "Deleting old upstream $external ..."
		rm -rf $external/upstream.*
	fi
done