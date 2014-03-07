#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	exec bash "$0" "$@"
	exit $?
fi

# Until bug with auto-detection of *.swig changes, bump this version: 1

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REAL_SRCLOC=`readlink -f $SRCLOC`

if [ -d "$SRCLOC/gen" ]; then
	rm -rf "$SRCLOC/gen"
fi
mkdir -p "$SRCLOC/gen/java/net/osmand/core/jni"
mkdir -p "$SRCLOC/gen/cpp"

if [[ -z "$SWIG" ]]; then
	SWIG=`which swig`
fi

$SWIG -java -package net.osmand.core.jni -outdir "$SRCLOC/gen/java/net/osmand/core/jni" -o "$SRCLOC/gen/cpp/swig.cpp" -I"$REAL_SRCLOC/../../include" -c++ -v "$REAL_SRCLOC/../../core.swig"
