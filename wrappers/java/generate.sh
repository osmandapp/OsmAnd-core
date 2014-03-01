#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -d "$DIRECTORY" ]; then
	rm -rf "$SRCLOC/gen"
fi
mkdir -p "$SRCLOC/gen/java/net/osmand/core/jni"
mkdir -p "$SRCLOC/gen/cpp"

if [[ -z "$SWIG" ]]; then
	SWIG=`which swig`
fi

$SWIG -java -package net.osmand.core.jni -outdir "$SRCLOC/gen/java/net/osmand/core/jni" -o "$SRCLOC/gen/cpp/swig.cpp" -I"$SRCLOC/../../include" -c++ -v "$SRCLOC/../../core.swig"
