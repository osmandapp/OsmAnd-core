#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../../../build/utils/functions.sh"

prepareUpstreamFrom7zArchive "$SRCLOC" "https://jogamp.org/deployment/v2.2.0/archive/jogamp-all-platforms.7z"
patchUpstream "$SRCLOC"
