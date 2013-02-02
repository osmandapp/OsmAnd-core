#!/bin/bash

# 0.Variables
PROGUARD_PATH=../../../proguard4.8
OSMAND_REPO=../../
AVIAN_PATH=../../avian/core
AVIAN_BUILD=$AVIAN_PATH/build/wp8-i386-debug-bootimage

OSMAND_JAVA=$OSMAND_REPO/core/OsmAnd-java
BINARY_NAME=routing_test

# Ensure that OsmAnd and Avian is compiled
ant -f $OSMAND_JAVA/build.xml
make -C $AVIAN_PATH platform=wp8 arch=i386 mode=debug

# Prepare *.class files
rm -rf stage
mkdir -p stage
(cd stage && \
	jar -xf ../$AVIAN_BUILD/host/classpath.jar && \
	jar -xf ../$OSMAND_JAVA/OsmAnd-avian.jar && \
	jar -xf ../$OSMAND_JAVA/lib/simple-logging.jar && \
	jar -xf ../$OSMAND_JAVA/lib/kxml2-2.3.0.jar && \
	jar -xf ../$OSMAND_JAVA/lib/junidecode-0.1.jar)

# Collect resources
rm -rf resources
mkdir -p resources
(cd stage && \
	find . -type f -not -name '*.class' | xargs tar cf - | tar xf - -C ../resources)
(cd resources && \
	jar cf ../resources.jar *)

# Compile resources
$AVIAN_BUILD/host/binaryToObject/binaryToObject \
	resources.jar \
	resources.obj \
	_binary_resources_jar_start _binary_resources_jar_end windows i386 1

# AOT-compile our code
rm -f OsmAnd-core.lib
rm -f bootimage-bin.obj
rm -f codeimage-bin.obj
$AVIAN_BUILD/bootimage-generator \
    -cp stage \
    -bootimage bootimage-bin.obj \
    -codeimage codeimage-bin.obj

# Compile our code all together
ar=`cygpath -u "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\WPSDK\WP80\bin\lib.exe"`
"$ar" bootimage-bin.obj codeimage-bin.obj resources.obj -out:OsmAnd-core.lib
