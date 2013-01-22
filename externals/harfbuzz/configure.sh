#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if ls -1 $SRCLOC/upstream.patched >/dev/null 2>&1
then
   exit
fi

# Delete old one if such exists
if ls -1 $SRCLOC/upstream.* >/dev/null 2>&1
then
	echo "Deleting old upstream..."
	rm -rf $SRCLOC/upstream.*
fi

# Download
echo "Downloading new upstream..."
git clone https://github.com/osmandapp/OsmAnd-external-harfbuzz-legacy.git $SRCLOC/upstream.original --depth=1

# Patch
cp -rf $SRCLOC/upstream.original $SRCLOC/upstream.patched
if [ -d $SRCLOC/patches ]; then
	echo "Patching..."
	PATCHES=`ls -1 $SRCLOC/patches/*.patch | sort`
	for PATCH in $PATCHES
	do
		read  -rd '' PATCH <<< "$PATCH"
		echo "Applying "`basename $PATCH`
		patch --strip=1 --directory=$SRCLOC/upstream.patched/ --input=$PATCH
	done
fi

# Get missing parts
curl -L http://skia.googlecode.com/svn/trunk/third_party/harfbuzz/contrib/tables/mirroring-parse.py > $SRCLOC/upstream.patched/contrib/tables/mirroring-parse.py || { echo "Failed to download mirroring-parse.py" 1>&2; exit; }

# Execute specific harfbuzz actions
curl -L http://www.unicode.org/Public/5.1.0/ucd/extracted/DerivedGeneralCategory.txt > $SRCLOC/upstream.patched/contrib/tables/DerivedGeneralCategory.txt || { echo "Failed to download DerivedGeneralCategory.txt!" 1>&2; exit; }
curl -L http://www.unicode.org/Public/5.1.0/ucd/extracted/DerivedCombiningClass.txt > $SRCLOC/upstream.patched/contrib/tables/DerivedCombiningClass.txt || { echo "Failed to download DerivedCombiningClass.txt!" 1>&2; exit; }
curl -L http://www.unicode.org/Public/5.1.0/ucd/auxiliary/GraphemeBreakProperty.txt > $SRCLOC/upstream.patched/contrib/tables/GraphemeBreakProperty.txt || { echo "Failed to download GraphemeBreakProperty.txt!" 1>&2; exit; }
curl -L http://www.unicode.org/Public/5.1.0/ucd/Scripts.txt > $SRCLOC/upstream.patched/contrib/tables/Scripts.txt || { echo "Failed to download Scripts.txt!" 1>&2; exit; }
curl -L http://www.unicode.org/Public/5.1.0/ucd/BidiMirroring.txt > $SRCLOC/upstream.patched/contrib/tables/BidiMirroring.txt || { echo "Failed to download BidiMirroring.txt!" 1>&2; exit; }
python $SRCLOC/upstream.patched/contrib/tables/category-parse.py $SRCLOC/upstream.patched/contrib/tables/DerivedGeneralCategory.txt $SRCLOC/upstream.patched/contrib/tables/category-properties.h
python $SRCLOC/upstream.patched/contrib/tables/combining-class-parse.py $SRCLOC/upstream.patched/contrib/tables/DerivedCombiningClass.txt $SRCLOC/upstream.patched/contrib/tables/combining-properties.h
python $SRCLOC/upstream.patched/contrib/tables/grapheme-break-parse.py $SRCLOC/upstream.patched/contrib/tables/GraphemeBreakProperty.txt $SRCLOC/upstream.patched/contrib/tables/grapheme-break-properties.h
python $SRCLOC/upstream.patched/contrib/tables/scripts-parse.py $SRCLOC/upstream.patched/contrib/tables/Scripts.txt $SRCLOC/upstream.patched/contrib/tables/script-properties.h
python $SRCLOC/upstream.patched/contrib/tables/mirroring-parse.py $SRCLOC/upstream.patched/contrib/tables/BidiMirroring.txt $SRCLOC/upstream.patched/contrib/tables/mirroring-properties.h
