#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -z "$DO_NOT_REGENERATE_SWIG" ]; then
	echo "Don't regenerate swig (unset env var DO_NOT_REGENERATE_SWIG to regenerate)"
	exit 0;
fi

# Prepare output folders
OUTPUTDIR=$1
if [ -z "$OUTPUTDIR" ]; then
	OUTPUTDIR="$(pwd)"
fi
echo "Output path:$OUTPUTDIR"
if [ -d "$OUTPUTDIR/gen" ]; then
	rm -rf "$OUTPUTDIR/gen"
fi
mkdir -p "$OUTPUTDIR/gen/java/net/osmand/core/jni"
mkdir -p "$OUTPUTDIR/gen/cpp"

if [[ -z "$SWIG" ]]; then
	SWIG=`which swig`
else
	if [[ "$(uname -a)" =~ Cygwin ]]; then
		SWIG=$(cygpath -u "$SWIG")
	fi
fi
if [[ -z "$SWIG" ]]; then
	echo "SWIG not found in path and was not specified by environment variable SWIG"
	exit 1
fi
echo "Using '$SWIG' as swig"

# Actually perform generation
$SWIG \
	-java \
	-package net.osmand.core.jni \
	-outdir "$OUTPUTDIR/gen/java/net/osmand/core/jni" \
	-o "$OUTPUTDIR/gen/cpp/swig.cpp" \
	-DSWIG_JAVA \
	-I"$SRCLOC/../../include" \
	-c++ \
	-v \
	"$SRCLOC/../../core.swig"

# Collect output files
FIND_CMD="$(which find)"
PRINT_CMD="-print"
if [[ "$(uname -a)" =~ Cygwin ]]; then
	PRINT_CMD="-print0 | xargs -0 cygpath -w"
fi
(cd "${OUTPUTDIR}/gen/cpp" && eval "$FIND_CMD . -type f $PRINT_CMD" > "$OUTPUTDIR/gen/cpp.list")
(cd "${OUTPUTDIR}/gen/java" && eval "$FIND_CMD . -type f $PRINT_CMD" > "$OUTPUTDIR/gen/java.list")
