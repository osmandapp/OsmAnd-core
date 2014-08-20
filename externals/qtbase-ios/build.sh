#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

QTBASE_CONFIGURATION=$(echo "
	-release -opensource -confirm-license -c++11 -static -largefile -no-accessibility -qt-sql-sqlite
	-no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
	-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
	-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
	-v
" | tr '\n' ' ')

if [[ "$(uname -a)" =~ Darwin ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi

	# Function: makeFlavor(name, platform, configuration)
	makeFlavor()
	{
		local name=$1
		local platform=$2
		local configuration=$3
		
		local path="$SRCLOC/upstream.patched.$name"
		
		# Configure
		if [ ! -d "$path" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$path"
			(cd "$path" && ./configure -xplatform $platform $configuration)
			retcode=$?
			if [ $retcode -ne 0 ]; then
				echo "Failed to configure 'qtbase-ios' for '$name', aborting..."
				rm -rf "$path"
				exit $retcode
			fi
		fi
		
		# Build
		(cd "$path" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
		retcode=$?
		if [ $retcode -ne 0 ]; then
			echo "Failed to build 'qtbase-ios' for '$name', aborting..."
			rm -rf "$path"
			exit $retcode
		fi
	}
	
	makeFlavor "ios.simulator.clang-i386.static" "macx-ios-clang-simulator-i386" "$QTBASE_CONFIGURATION -sdk iphonesimulator"
	makeFlavor "ios.device.clang-armv7.static" "macx-ios-clang-device-armv7" "$QTBASE_CONFIGURATION -sdk iphoneos"
	makeFlavor "ios.device.clang-armv7s.static" "macx-ios-clang-device-armv7s" "$QTBASE_CONFIGURATION -sdk iphoneos"

	if [ ! -h "$SRCLOC/upstream.patched.ios.simulator.clang.static" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.simulator.clang-i386.static" "upstream.patched.ios.simulator.clang.static")
	fi

	if [ ! -h "$SRCLOC/upstream.patched.ios.device.clang.static" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.device.clang-armv7.static" "upstream.patched.ios.device.clang.static")
	fi

	if [ ! -h "$SRCLOC/upstream.patched.ios.clang-iphoneos" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.device.clang-armv7.static" "upstream.patched.ios.clang-iphoneos")
	fi

	if [ ! -h "$SRCLOC/upstream.patched.ios.clang-iphonesimulator" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.simulator.clang-i386.static" "upstream.patched.ios.clang-iphonesimulator")
	fi

	if [ ! -d "$SRCLOC/upstream.patched.ios.clang" ]; then
		# Make link to cmake stuff and include, src and bin from already built target (any is suitable)
		mkdir -p "$SRCLOC/upstream.patched.ios.clang"
		(cd "$SRCLOC/upstream.patched.ios.clang" && \
			ln -s "../upstream.patched.ios.simulator.clang-i386.static/include" "include")
		(cd "$SRCLOC/upstream.patched.ios.clang" && \
			ln -s "../upstream.patched.ios.simulator.clang-i386.static/src" "src")
		(cd "$SRCLOC/upstream.patched.ios.clang" && \
			ln -s "../upstream.patched.ios.simulator.clang-i386.static/bin" "bin")
		mkdir -p "$SRCLOC/upstream.patched.ios.clang/lib"
		(cd "$SRCLOC/upstream.patched.ios.clang/lib" && \
			ln -s "../../upstream.patched.ios.simulator.clang-i386.static/lib/cmake" "cmake")

		# Make universal libraries using lipo
		libraries=(Core Concurrent Network Sql Xml)
		for libName in "${libraries[@]}" ; do
			echo "Packing '$libName'..."
			lipo -create \
				"$SRCLOC/upstream.patched.ios.simulator.clang-i386.static/lib/libQt5${libName}.a" \
				"$SRCLOC/upstream.patched.ios.device.clang-armv7.static/lib/libQt5${libName}.a" \
				"$SRCLOC/upstream.patched.ios.device.clang-armv7s.static/lib/libQt5${libName}.a" \
				-output "$SRCLOC/upstream.patched.ios.clang/lib/libQt5${libName}.a"
			retcode=$?
			if [ $retcode -ne 0 ]; then
				echo "Failed to lipo 'libQt5${libName}.a', aborting..."
				rm -rf "$path"
				exit $retcode
			fi
		done
	fi
	if [ ! -d "$SRCLOC/upstream.patched.ios.clang-fat.static" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.clang" "upstream.patched.ios.clang-fat.static")
	fi
fi
