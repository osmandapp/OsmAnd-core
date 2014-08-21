#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Until bug with auto-detection of *.swig changes, bump this version: 2

REAL_SRCLOC=`readlink -f $SRCLOC`

if [ -d "$SRCLOC/gen" ]; then
	rm -rf "$SRCLOC/gen"
fi
mkdir -p "$SRCLOC/gen/csharp"
mkdir -p "$SRCLOC/gen/cpp"

if [[ -z "$SWIG" ]]; then
	SWIG=`which swig`
fi

$SWIG \
	-csharp \
	-namespace net.osmand.core.dotnet \
	-outdir "$SRCLOC/gen/csharp" \
	-o "$SRCLOC/gen/cpp/swig.cpp" \
	-I"$REAL_SRCLOC/../../include" \
	-c++ \
	-v \
	"$REAL_SRCLOC/../../core.swig"
