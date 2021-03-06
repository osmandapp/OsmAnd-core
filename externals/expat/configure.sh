#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

# TODO upgrade to 2.4.1 !!! 
# prepareUpstreamFromTarArchive "$SRCLOC" "http://builder.osmand.net/dependencies-mirror/expat-2.4.1.tar.gz"
prepareUpstreamFromTarArchive "$SRCLOC" "http://builder.osmand.net/dependencies-mirror/bak/expat-2.1.0.tar.gz"
patchUpstream "$SRCLOC"
