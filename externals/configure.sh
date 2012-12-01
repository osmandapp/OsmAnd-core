#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for external in $SRCLOC/* ; do
	if [ -d "$external" ]; then
		if [ -e "$external/configure.sh" ]; then
			$external/configure.sh
		fi
	fi
done