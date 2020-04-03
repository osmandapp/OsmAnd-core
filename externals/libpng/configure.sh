#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

# prepareUpstreamFromTarArchive "$SRCLOC" "https://builder.osmand.net/dependencies-mirror/libpng-1.6.33.tar.xz"
prepareUpstreamFromTarArchive "$SRCLOC" "https://sourceforge.net/projects/libpng/files/libpng16/older-releases/1.6.33/libpng-1.6.33.tar.xz/download"
patchUpstream "$SRCLOC"
