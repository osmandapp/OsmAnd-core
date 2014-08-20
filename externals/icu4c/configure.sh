#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromTarArchive "$SRCLOC" "http://download.icu-project.org/files/icu4c/52.1/icu4c-52_1-src.tgz"
patchUpstream "$SRCLOC"

# Download upstream data if needed
if [ ! -f "$SRCLOC/upstream.data-pack" ]; then
	echo "Downloading '$NAME' upstream data..."
	curl -L http://download.osmand.net/prebuilt/icudt52l.zip > "$SRCLOC/upstream.data-pack"
	retcode=$?
	if [ $retcode -ne 0 ]; then
		echo "Failed to download data for '$externalName', aborting..."
		rm -rf "$externalPath/upstream.data-pack"
		exit $retcode
	fi
fi

# Extract upstream data if needed
if [ ! -d "$SRCLOC/upstream.data" ]; then
	echo "Extracting '$NAME' upstream data..."
	mkdir -p "$SRCLOC/upstream.data"
	unzip "$SRCLOC/upstream.data-pack" -d "$SRCLOC/upstream.data"
	retcode=$?
	if [ $retcode -ne 0 ]; then
		echo "Failed to download data for '$externalName', aborting..."
		rm -rf "$externalPath/upstream.data-pack" "$externalPath/upstream.data"
		exit $retcode
	fi
fi
