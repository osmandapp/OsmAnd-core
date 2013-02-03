#!/bin/bash
# 0.Variables
PROGUARD_PATH=../../../../proguard4.9beta1

OSMAND_REPO=../../
AVIAN_PATH=../../avian/core
AND_PLATFORM=$ANDROID_NDK/platforms/android-5/arch-arm/usr/
AND_TOOLCHAIN=$ANDROID_NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86/

OSMAND_JAVA=$OSMAND_REPO/core/OsmAnd-java
BINARY_NAME=routing_test
# LIBNAME=lib$BINARY_NAME.so #uncomment line to make library instead of binary
# JAR_BOOT=1 #uncomment this line to make jar compilation

# this is used because of the bug in android toolchain we compile 
# codeimage.o, bootimage.o with linux-arm and here we specify the path
# CODEIMAGE_OBJ=codeimage-bin.o 
# BOOTIMAGE_OBJ=bootimage-bin.o

ARCH=i386
PLATFORM=linux
B_ARCH=$ARCH
B_PLATFORM=$PLATFORM
# DEBUG=-debug
BOOTIMAGE=-bootimage


AVIAN_BUILD=$AVIAN_PATH/build/$PLATFORM-$ARCH$DEBUG$BOOTIMAGE
AVIAN_BUILD_NOT_BOOT=$AVIAN_PATH/build/$PLATFORM-$ARCH$DEBUG
#CXX="g++ -DDEBUG -g"
CXX="g++ -DDEBUG -g"
CXX_FLAGS="-I$JAVA_HOME/include -I$JAVA_HOME/include/$PLATFORM -D_JNI_IMPLEMENTATION_ -fPIC"
LD_FLAGS="-lpthread -g"
STRIP="strip"
# PROGUARD=true

# CXX="$AND_TOOLCHAIN/bin/arm-linux-androideabi-g++ --sysroot=$AND_PLATFORM"
# STRIP="$AND_TOOLCHAIN/bin/arm-linux-androideabi-strip"
# CXX_FLAGS="-I$JAVA_HOME/include -I$JAVA_HOME/include/$PLATFORM -I$AND_PLATFORM/include -D_JNI_IMPLEMENTATION_"
# B_PLATFORM=linux
# LD_FLAGS="-L$ANDROID_NDK/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi-v7a/ -lgnustl_shared -llog"


if [ $JAR_BOOT ]; then
	CXX_FLAGS+=" -DBOOT_JAR"
fi
##################################
#1. (cd $OSMAND_JAVA && ant) # build jar
echo "Combining result.jar"
mkdir -p stage1 && rm -rf stage1/*
cp -r $AVIAN_BUILD/classpath/* stage1/
cp $OSMAND_JAVA/OsmAnd-avian.jar $OSMAND_JAVA/lib/simple-logging.jar $OSMAND_JAVA/lib/kxml2-2.3.0.jar $OSMAND_JAVA/lib/junidecode-0.1.jar stage1/
(cd stage1 && jar -xf OsmAnd-avian.jar  && jar -xf simple-logging.jar  && jar -xf kxml2-2.3.0.jar && jar -xf junidecode-0.1.jar)
rm stage1/*.jar
JAR_FOLDER=stage1

# 2. proguard

if [ $PROGUARD ]; then
 echo "Proguard"
 mkdir -p stage2 && rm -rf stage2/*
 java -jar $PROGUARD_PATH/lib/proguard.jar -dontusemixedcaseclassnames \
	-injars stage1 -outjars stage2 @$AVIAN_BUILD/../../vm.pro @osmand-avian.pro
 JAR_FOLDER=stage2
fi

# 3. combine all o
echo "Collecting all .o"
mkdir -p stage3 && rm -rf stage3/*
(cp $AVIAN_BUILD/libavian.a stage3/ && cd stage3 && ar x libavian.a)
if [ $JAR_BOOT ]; then
	(cd $JAR_FOLDER && jar cvf classes.jar * >> /dev/null)
	$AVIAN_BUILD/binaryToObject/binaryToObject $JAR_FOLDER/classes.jar stage3/classes-jar.o \
	 _binary_boot_jar_start _binary_boot_jar_end $B_PLATFORM $B_ARCH
else
	# Collect resources
	rm -rf resources 
	mkdir -p resources
	(cd stage1 && find . -type f -not -name '*.class' | xargs tar cf - | tar xf - -C ../resources)
	(cd resources && jar cf ../resources.jar *)

	# Compile resources
	$AVIAN_BUILD_NOT_BOOT/binaryToObject/binaryToObject \
		resources.jar stage3/resources.o _binary_resources_jar_start _binary_resources_jar_end $B_PLATFORM $B_ARCH
	if [ $CODEIMAGE_OBJ ]; then
		cp $CODEIMAGE_OBJ stage3
		cp $BOOTIMAGE_OBJ stage3
    else 
    	$AVIAN_BUILD/bootimage-generator -cp $JAR_FOLDER  -bootimage stage3/bootimage-bin.o -codeimage stage3/codeimage-bin.o
    fi
fi

# 4. compile main
if [ $LIBNAME ]; then
	echo "Compile library"
	$CXX $CXX_FLAGS -DBOOTSTRAP_SO="\"-Davian.bootstrap=$LIBNAME\"" -c $BINARY_NAME.cpp -o stage3/$BINARY_NAME.o
	$CXX -shared stage3/*.o -ldl -lz  -Wl,--no-undefined \
		$LD_FLAGS -o $LIBNAME
	$STRIP --strip-all $LIBNAME
	#
else 
	echo "Compile main"
	$CXX $CXX_FLAGS -c $BINARY_NAME.cpp -o stage3/$BINARY_NAME.o
	$CXX -rdynamic stage3/*.o -ldl -lz $LD_FLAGS -o $BINARY_NAME
	#$STRIP --strip-all $BINARY_NAME
fi


