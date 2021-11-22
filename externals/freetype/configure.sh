#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromGit "$SRCLOC" "https://chromium.googlesource.com/chromium/src/third_party/freetype2" "12ef831fc314518bff45278008a568608501a8e4"
patchUpstream "$SRCLOC"
