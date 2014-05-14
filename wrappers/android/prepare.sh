#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

# Get root
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT="$SRCLOC/../../.."

# Fail on any error
set -e

# Prepare core dependencies
echo "Configuring dependencies..."
"$ROOT/core/externals/configure.sh" expat freetype gdal giflib glm glsl-optimizer jpeg libpng protobuf qtbase-android skia icu4c boost-android
echo "Building dependencies..."
"$ROOT/core/externals/build.sh"
