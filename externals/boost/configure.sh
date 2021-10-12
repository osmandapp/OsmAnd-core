#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromTarArchive "$SRCLOC" "https://boostorg.jfrog.io/artifactory/main/release/1.69.0/source/boost_1_69_0.tar.bz2"
#prepareUpstreamFromTarArchive "$SRCLOC" "https://boostorg.jfrog.io/artifactory/main/release/1.68.0/source/boost_1_68_0.tar.bz2"
#prepareUpstreamFromTarArchive "$SRCLOC" "https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.bz2"

#prepareUpstreamFromTarArchive "$SRCLOC" "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2"
#"https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2"
#"http://builder.osmand.net/dependencies-mirror/boost_1_56_0.tar.bz2"
patchUpstream "$SRCLOC"
