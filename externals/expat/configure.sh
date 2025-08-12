#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromTarArchive "$SRCLOC" "http://builder.osmand.net/dependencies-mirror/expat-2.5.0.tar.gz"
#prepareUpstreamFromTarArchive "$SRCLOC" "https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.gz"
patchUpstream "$SRCLOC"
