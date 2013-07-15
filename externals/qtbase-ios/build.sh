#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

QTBASE_CONFIGURATION=\
"-opensource -confirm-license "\
"-nomake examples -nomake tools "\
"-qt-sql-sqlite "\
"-no-accessibility -no-gui -no-widgets -no-nis -no-opengl -no-kms -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit "\
"-c++11 -static -debug-and-release "\
"-v"

if [[ "$(uname -a)" == *Darwin* ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi

	if [ ! -d "$SRCLOC/upstream.patched.ios.simulator" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.simulator"
		(cd "$SRCLOC/upstream.patched.ios.simulator" && \
			./configure -xplatform unsupported/macx-ios-clang $QTBASE_CONFIGURATION -sdk iphonesimulator)
	fi
	(cd "$SRCLOC/upstream.patched.ios.simulator" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

	if [ ! -d "$SRCLOC/upstream.patched.ios.device" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.device"
		(cd "$SRCLOC/upstream.patched.ios.device" && \
			./configure -xplatform unsupported/macx-ios-clang $QTBASE_CONFIGURATION -sdk iphoneos)
	fi
	(cd "$SRCLOC/upstream.patched.ios.device" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
fi
