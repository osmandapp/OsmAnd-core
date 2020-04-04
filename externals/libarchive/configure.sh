#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromTarArchive "$SRCLOC" "https://builder.osmand.net/dependencies-mirror/libarchive-3.1.2.tar.gz"
patchUpstream "$SRCLOC"
