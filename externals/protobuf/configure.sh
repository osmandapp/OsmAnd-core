#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

# prepareUpstreamFromTarArchive "$SRCLOC" "https://builder.osmand.net/dependencies-mirror/protobuf-2.4.1.tar.bz2"
prepareUpstreamFromTarArchive "$SRCLOC" "https://builder.osmand.net/dependencies-mirror/protobuf-2.5.0.tar.bz2"
patchUpstream "$SRCLOC"
