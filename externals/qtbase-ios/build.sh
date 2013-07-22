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

	if [ ! -d "$SRCLOC/upstream.patched.ios.simulator" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.simulator"
		(cd "$SRCLOC/upstream.patched.ios.simulator" && \
			./configure -xplatform unsupported/macx-ios-clang-simulator $QTBASE_CONFIGURATION -sdk iphonesimulator)
	fi
	(cd "$SRCLOC/upstream.patched.ios.simulator" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

	if [ ! -d "$SRCLOC/upstream.patched.ios.device" ]; then
		cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.ios.device"
		(cd "$SRCLOC/upstream.patched.ios.device" && \
			./configure -xplatform unsupported/macx-ios-clang-device $QTBASE_CONFIGURATION -sdk iphoneos)
	fi
	(cd "$SRCLOC/upstream.patched.ios.device" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

	if [ ! -d "$SRCLOC/upstream.patched.ios.universal" ]; then
		# Copy cmake-related stuff from already built target (any is suitable)
		mkdir -p "$SRCLOC/upstream.patched.ios.universal/lib"
		cp -rpf "$SRCLOC/upstream.patched.ios.simulator/lib/cmake" "$SRCLOC/upstream.patched.ios.universal/lib/cmake"

		# Make universal libraries using lipo
		for sourcePath in "$SRCLOC/upstream.patched.ios.simulator/lib"/lib*.a ; do
			libName=$(basename "$sourcePath")
			if [[ "$libName" == *Bootstrap* ]]; then
				continue
			fi

			echo "Packing '$libName'..."
			lipo -create "$SRCLOC/upstream.patched.ios.simulator/lib/$libName" "$SRCLOC/upstream.patched.ios.device/lib/$libName" -output "$SRCLOC/upstream.patched.ios.universal/lib/$libName"
		done
	fi
fi
