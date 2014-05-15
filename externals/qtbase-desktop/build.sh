#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

OSMAND_ARCHITECTURES_SET=($*)

if [[ "$(uname -a)" =~ Cygwin ]]; then
	echo "Building under Cygwin is not supported, use build.bat"
	exit 1
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
		if [ $? -ne 0 ]; then
			echo "Failed to configure 'qtbase-desktop' for '$name', aborting..."
			rm -rf "$path"
			exit $?
		fi
	fi
	
	# Build
	(cd "$path" && $MAKE -j$OSMAND_BUILD_CPU_CORES_NUM)
	if [ $? -ne 0 ]; then
		echo "Failed to build 'qtbase-desktop' for '$name', aborting..."
		rm -rf "$path"
		exit $?
	fi
}

# Function: makeStaticAndSharedFlavor(name, platform, configuration)
makeStaticAndSharedFlavor()
{
	local name=$1
	local platform=$2
	local configuration=$3

	makeFlavor "$name.static" $platform "-static $configuration"
	makeFlavor "$name.shared" $platform "-shared $configuration"
}

if [[ "$(uname -a)" =~ Linux ]]; then
	QTBASE_CONFIGURATION=$(echo "
		-release -opensource -confirm-license -c++11 -largefile -no-accessibility -qt-sql-sqlite
		-no-javascript-jit -no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
		-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
		-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
		-v
	" | tr '\n' ' ')

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
	
	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		makeStaticAndSharedFlavor "linux.i686" "linux-g++-32" "$QTBASE_CONFIGURATION"
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x64 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		makeStaticAndSharedFlavor "linux.amd64" "linux-g++-64" "$QTBASE_CONFIGURATION"
	fi
fi

if [[ "$(uname -a)" =~ Darwin ]]; then
	QTBASE_CONFIGURATION=$(echo "
		-debug-and-release -opensource -confirm-license -c++11 -largefile -no-accessibility -qt-sql-sqlite
		-no-javascript-jit -no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
		-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
		-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib -no-framework
		-v
	" | tr '\n' ' ')

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		makeStaticAndSharedFlavor "darwin.i686" "macx-clang-libc++-32" "$QTBASE_CONFIGURATION"
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x64 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		makeStaticAndSharedFlavor "darwin.x86_64" "macx-clang-libc++-64" "$QTBASE_CONFIGURATION"
	fi

	if [ ! -d "$SRCLOC/upstream.patched.darwin.intel.shared" ]; then
		# Make link to cmake stuff and includes from already built target (any is suitable)
		mkdir -p "$SRCLOC/upstream.patched.darwin.intel.shared/lib"
		ln -s "$SRCLOC/upstream.patched.darwin.i386.shared/lib/cmake" "$SRCLOC/upstream.patched.darwin.intel.shared/lib/cmake"
		ln -s "$SRCLOC/upstream.patched.darwin.i386.shared/include" "$SRCLOC/upstream.patched.darwin.intel.shared/include"
		ln -s "$SRCLOC/upstream.patched.darwin.i386.shared/bin" "$SRCLOC/upstream.patched.darwin.intel.shared/bin"

		# Make universal libraries using lipo
		libraries=(Core Concurrent Network Sql Xml)
		for libName in "${libraries[@]}" ; do
			echo "Packing '$libName'..."
			lipo -create \
				"$SRCLOC/upstream.patched.darwin.x86_64.shared/lib/libQt5${libName}.5.2.0.dylib" \
				"$SRCLOC/upstream.patched.darwin.i386.shared/lib/libQt5${libName}.5.2.0.dylib" \
				-output "$SRCLOC/upstream.patched.darwin.intel.shared/lib/libQt5${libName}.5.2.0.dylib"
			(cd "$SRCLOC/upstream.patched.darwin.intel.shared/lib" && \
				ln -s "libQt5${libName}.5.2.0.dylib" "libQt5${libName}.5.2.dylib" && \
				ln -s "libQt5${libName}.5.2.0.dylib" "libQt5${libName}.5.dylib" && \
				ln -s "libQt5${libName}.5.2.0.dylib" "libQt5${libName}.dylib")
			
			lipo -create \
				"$SRCLOC/upstream.patched.darwin.x86_64.shared/lib/libQt5${libName}_debug.5.2.0.dylib" \
				"$SRCLOC/upstream.patched.darwin.i386.shared/lib/libQt5${libName}_debug.5.2.0.dylib" \
				-output "$SRCLOC/upstream.patched.darwin.intel.shared/lib/libQt5${libName}_debug.5.2.0.dylib"
			(cd "$SRCLOC/upstream.patched.darwin.intel.shared/lib" && \
				ln -s "libQt5${libName}_debug.5.2.0.dylib" "libQt5${libName}_debug.5.2.dylib" && \
				ln -s "libQt5${libName}_debug.5.2.0.dylib" "libQt5${libName}_debug.5.dylib" && \
				ln -s "libQt5${libName}_debug.5.2.0.dylib" "libQt5${libName}_debug.dylib")
		done
	fi
	if [ ! -d "$SRCLOC/upstream.patched.darwin.intel.static" ]; then
		# Make link to cmake stuff and includes from already built target (any is suitable)
		mkdir -p "$SRCLOC/upstream.patched.darwin.intel.static/lib"
		ln -s "$SRCLOC/upstream.patched.darwin.i386.static/lib/cmake" "$SRCLOC/upstream.patched.darwin.intel.static/lib/cmake"
		ln -s "$SRCLOC/upstream.patched.darwin.i386.static/include" "$SRCLOC/upstream.patched.darwin.intel.static/include"
		ln -s "$SRCLOC/upstream.patched.darwin.i386.static/bin" "$SRCLOC/upstream.patched.darwin.intel.static/bin"

		# Make universal libraries using lipo
		libraries=(Core Concurrent Network Sql Xml)
		for libName in "${libraries[@]}" ; do
			echo "Packing '$libName'..."
			lipo -create \
				"$SRCLOC/upstream.patched.darwin.x86_64.static/lib/libQt5${libName}.a" \
				"$SRCLOC/upstream.patched.darwin.i386.static/lib/libQt5${libName}.a" \
				-output "$SRCLOC/upstream.patched.darwin.intel.static/lib/libQt5${libName}.a"
			lipo -create \
				"$SRCLOC/upstream.patched.darwin.x86_64.static/lib/libQt5${libName}_debug.a" \
				"$SRCLOC/upstream.patched.darwin.i386.static/lib/libQt5${libName}_debug.a" \
				-output "$SRCLOC/upstream.patched.darwin.intel.static/lib/libQt5${libName}_debug.a"
		done
	fi
fi
