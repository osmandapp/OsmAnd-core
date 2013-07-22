#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

QTBASE_CONFIGURATION=$(echo "
	-release -opensource -confirm-license -c++11 -static -largefile -no-accessibility -qt-sql-sqlite
	-no-javascript-jit -no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
	-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
	-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
	-v
" | tr '\n' ' ')

if [[ "$(uname -a)" =~ Darwin ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi

	if [ ! -d "$SRCLOC/upstream.patched.ios.simulator.i386" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.simulator.i386"
		(cd "$SRCLOC/upstream.patched.ios.simulator.i386" && \
			./configure -xplatform unsupported/macx-ios-clang-simulator-i386 $QTBASE_CONFIGURATION -sdk iphonesimulator)
	fi
	(cd "$SRCLOC/upstream.patched.ios.simulator.i386" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

	if [ ! -d "$SRCLOC/upstream.patched.ios.device.armv7" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.device.armv7"
		(cd "$SRCLOC/upstream.patched.ios.device.armv7" && \
			./configure -xplatform unsupported/macx-ios-clang-device-armv7 $QTBASE_CONFIGURATION -sdk iphoneos)
	fi
	(cd "$SRCLOC/upstream.patched.ios.device.armv7" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	
	if [ ! -d "$SRCLOC/upstream.patched.ios.device.armv7s" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.device.armv7s"
		(cd "$SRCLOC/upstream.patched.ios.device.armv7s" && \
			./configure -xplatform unsupported/macx-ios-clang-device-armv7s $QTBASE_CONFIGURATION -sdk iphoneos)
	fi
	(cd "$SRCLOC/upstream.patched.ios.device.armv7s" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

	if [ ! -d "$SRCLOC/upstream.patched.ios" ]; then
		# Copy cmake-related stuff from already built target (any is suitable)
		mkdir -p "$SRCLOC/upstream.patched.ios/lib"
		cp -rpf "$SRCLOC/upstream.patched.ios.simulator.i386/lib/cmake" "$SRCLOC/upstream.patched.ios/lib/cmake"

		# Make universal libraries using lipo
		for sourcePath in "$SRCLOC/upstream.patched.ios.simulator.i386/lib"/lib*.a ; do
			libName=$(basename "$sourcePath")
			if [[ "$libName" == *Bootstrap* ]]; then
				continue
			fi

			echo "Packing '$libName'..."
			lipo -create \
				"$SRCLOC/upstream.patched.ios.simulator.i386/lib/$libName" \
				"$SRCLOC/upstream.patched.ios.device.armv7/lib/$libName" \
				"$SRCLOC/upstream.patched.ios.device.armv7s/lib/$libName" \
				-output "$SRCLOC/upstream.patched.ios/lib/$libName"
		done
	fi
fi
