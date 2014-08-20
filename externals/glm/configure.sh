#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromZipArchive "$SRCLOC" "http://sourceforge.net/projects/ogl-math/files/glm-0.9.5.3/glm-0.9.5.3.zip/download"
patchUpstream "$SRCLOC"
