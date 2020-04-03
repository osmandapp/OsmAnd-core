#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromTarArchive "$SRCLOC" "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2"
#"https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2"
#"https://builder.osmand.net/dependencies-mirror/boost_1_56_0.tar.bz2"
patchUpstream "$SRCLOC"
