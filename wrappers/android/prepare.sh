#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

ROOT="$SRCLOC/../../.."

# Prepare core dependencies
echo "Configuring dependencies..."
"$ROOT/core/externals/configure.sh" expat freetype gdal giflib glm jpeg libpng protobuf qtbase-android skia icu4c libarchive boost-android
retcode=$?
if [ $retcode -ne 0 ]; then
	echo "Failed to configure dependencies, aborting..."
	exit $retcode
fi
echo "Building dependencies..."
"$ROOT/core/externals/build.sh" expat freetype gdal giflib glm jpeg libpng protobuf qtbase-android skia icu4c libarchive boost-android
retcode=$?
if [ $retcode -ne 0 ]; then
	echo "Failed to build dependencies, aborting..."
	exit $retcode
fi
