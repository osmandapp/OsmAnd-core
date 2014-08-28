#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

# Cleanup environment
cleanupEnvironment

# Verify input
targetOS=$1
compiler=$2
targetArch=$3
if [[ "$targetOS" == "android" ]]; then
	source "$SRCLOC/build-android.sh"
	exit $?
elif [[ "$targetOS" == "windows" ]] || [[ "$targetOS" == "linux" ]] || [[ "$targetOS" == "macosx" ]] || [[ "$targetOS" == "ios" ]]; then
	echo "Building Boost for '${targetOS}' is not needed"
	exit 0
else
	echo "'windows', 'linux', 'macosx', 'ios', 'android' are the only supported targets, while '${targetOS}' was specified"
	exit 1
fi
