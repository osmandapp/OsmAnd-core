#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

# Check if already configured
if [ -d "$SRCLOC/upstream.patched" ]; then
	echo "Skipping external '$NAME': already configured"
	exit
fi

# Extract upstream if needed
if [ ! -d "$SRCLOC/upstream.original" ]; then
	echo "Downloading '$NAME' upstream..."
	git clone https://github.com/osmandapp/OsmAnd-external-harfbuzz-legacy.git "$SRCLOC/upstream.original" --depth=1
fi

# Patch
cp -rpf "$SRCLOC/upstream.original" "$SRCLOC/upstream.patched"
if [ -d "$SRCLOC/patches" ]; then
	echo "Patching '$NAME'..."
	PATCHES=`ls -1 $SRCLOC/patches/*.patch | sort`
	for PATCH in $PATCHES
	do
		read  -rd '' PATCH <<< "$PATCH"
		echo "Applying "`basename $PATCH`
		patch --strip=1 --directory="$SRCLOC/upstream.patched/" --input="$PATCH"
	done
fi

if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/mirroring-parse.py" ]; then
	curl -L http://skia.googlecode.com/svn/trunk/third_party/harfbuzz/contrib/tables/mirroring-parse.py \
		> "$SRCLOC/upstream.patched/contrib/tables/mirroring-parse.py" || { echo "Failed to download mirroring-parse.py" 1>&2; exit; }
fi

if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/DerivedGeneralCategory.txt" ]; then
	curl -L http://www.unicode.org/Public/5.1.0/ucd/extracted/DerivedGeneralCategory.txt \
		> "$SRCLOC/upstream.patched/contrib/tables/DerivedGeneralCategory.txt" || { echo "Failed to download DerivedGeneralCategory.txt!" 1>&2; exit; }
fi
if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/category-properties.h" ]; then
	python "$SRCLOC/upstream.patched/contrib/tables/category-parse.py" \
		"$SRCLOC/upstream.patched/contrib/tables/DerivedGeneralCategory.txt" "$SRCLOC/upstream.patched/contrib/tables/category-properties.h"
fi

if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/DerivedCombiningClass.txt" ]; then
	curl -L http://www.unicode.org/Public/5.1.0/ucd/extracted/DerivedCombiningClass.txt \
		> "$SRCLOC/upstream.patched/contrib/tables/DerivedCombiningClass.txt" || { echo "Failed to download DerivedCombiningClass.txt!" 1>&2; exit; }
fi
if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/combining-properties.h" ]; then
	python "$SRCLOC/upstream.patched/contrib/tables/combining-class-parse.py" \
		"$SRCLOC/upstream.patched/contrib/tables/DerivedCombiningClass.txt" "$SRCLOC/upstream.patched/contrib/tables/combining-properties.h"
fi

if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/GraphemeBreakProperty.txt" ]; then
	curl -L http://www.unicode.org/Public/5.1.0/ucd/auxiliary/GraphemeBreakProperty.txt \
		> "$SRCLOC/upstream.patched/contrib/tables/GraphemeBreakProperty.txt" || { echo "Failed to download GraphemeBreakProperty.txt!" 1>&2; exit; }
fi
if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/grapheme-break-properties.h" ]; then
	python "$SRCLOC/upstream.patched/contrib/tables/grapheme-break-parse.py" \
		"$SRCLOC/upstream.patched/contrib/tables/GraphemeBreakProperty.txt" "$SRCLOC/upstream.patched/contrib/tables/grapheme-break-properties.h"
fi

if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/Scripts.txt" ]; then
	curl -L http://www.unicode.org/Public/5.1.0/ucd/Scripts.txt \
		> "$SRCLOC/upstream.patched/contrib/tables/Scripts.txt" || { echo "Failed to download Scripts.txt!" 1>&2; exit; }
fi
if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/script-properties.h" ]; then
	python "$SRCLOC/upstream.patched/contrib/tables/scripts-parse.py" \
		"$SRCLOC/upstream.patched/contrib/tables/Scripts.txt" "$SRCLOC/upstream.patched/contrib/tables/script-properties.h"
fi

if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/BidiMirroring.txt" ]; then
	curl -L http://www.unicode.org/Public/5.1.0/ucd/BidiMirroring.txt \
		> "$SRCLOC/upstream.patched/contrib/tables/BidiMirroring.txt" || { echo "Failed to download BidiMirroring.txt!" 1>&2; exit; }
fi
if [ ! -f "$SRCLOC/upstream.patched/contrib/tables/mirroring-properties.h" ]; then
	python "$SRCLOC/upstream.patched/contrib/tables/mirroring-parse.py" \
		"$SRCLOC/upstream.patched/contrib/tables/BidiMirroring.txt" "$SRCLOC/upstream.patched/contrib/tables/mirroring-properties.h"
fi
