#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

# Cleanup environment
cleanupEnvironment

# Verify input
targetOS=$1
compiler=$2

QTBASE_CONFIGURATION=$(echo "
	-platform macx-clang -release -opensource -confirm-license -c++std c++11 -static -mp -silent -no-accessibility -sql-sqlite -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
	-nomake examples -nomake tools -nomake tests -no-feature-testlib -no-gui -no-widgets -no-cups -no-iconv -no-icu -no-dbus
	-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib -DQT_NO_FOREACH
	-v
" | tr '\n' ' ')

if [[ "$targetOS" == "ios" ]]; then
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
	
	if [[ "$compiler" == "clang" ]]; then
		if [ -z "$NOT_BUILD_QT_IOS_IF_PRESENT" ] || [ ! -f upstream.patched.ios.simulator.${compiler}.static/lib/libQt5Core.a ]; then
			echo "Going to build embedded Qt simulator for ${targetOS}/${compiler}"
			makeFlavor "ios.simulator.${compiler}.static" "macx-ios-${compiler}-simulator" "$QTBASE_CONFIGURATION -sdk iphonesimulator"
		fi
		if [ -z "$NOT_BUILD_QT_IOS_IF_PRESENT" ] || [ ! -f upstream.patched.ios.device.${compiler}.static/lib/libQt5Core.a ]; then
                        echo "Going to build embedded Qt device for ${targetOS}/${compiler}"
			makeFlavor "ios.device.${compiler}.static" "macx-ios-${compiler}-device" "$QTBASE_CONFIGURATION -sdk iphoneos"
		fi
	else
		echo "Only 'clang' is supported compiler for '${targetOS}' target, while '${compiler}' was specified"
		exit 1
	fi

	if [ ! -h "$SRCLOC/upstream.patched.ios.${compiler}-iphoneos" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.device.${compiler}.static" "upstream.patched.ios.${compiler}-iphoneos")
	fi

	if [ ! -h "$SRCLOC/upstream.patched.ios.${compiler}-iphonesimulator" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.simulator.${compiler}.static" "upstream.patched.ios.${compiler}-iphonesimulator")
	fi

	if [ ! -d "$SRCLOC/upstream.patched.ios.${compiler}" ]; then
		# Make link to cmake stuff and include, src and bin from already built target (any is suitable)
		mkdir -p "$SRCLOC/upstream.patched.ios.${compiler}"
		(cd "$SRCLOC/upstream.patched.ios.${compiler}" && \
			ln -s "../upstream.patched.ios.simulator.${compiler}.static/include" "include")
		(cd "$SRCLOC/upstream.patched.ios.${compiler}" && \
			ln -s "../upstream.patched.ios.simulator.${compiler}.static/src" "src")
		(cd "$SRCLOC/upstream.patched.ios.${compiler}" && \
			ln -s "../upstream.patched.ios.simulator.${compiler}.static/bin" "bin")
		mkdir -p "$SRCLOC/upstream.patched.ios.${compiler}/lib"
		(cd "$SRCLOC/upstream.patched.ios.${compiler}/lib" && \
			ln -s "../../upstream.patched.ios.simulator.${compiler}.static/lib/cmake" "cmake")

		# Make universal libraries using lipo
		libraries=(qtpcre2 Qt5Core Qt5Concurrent Qt5Network Qt5Sql Qt5Xml)
		for libName in "${libraries[@]}" ; do
			echo "Packing '$libName'..."
			lipo -create \
				"$SRCLOC/upstream.patched.ios.simulator.${compiler}.static/lib/lib${libName}.a" \
				"$SRCLOC/upstream.patched.ios.device.${compiler}.static/lib/lib${libName}.a" \
				-output "$SRCLOC/upstream.patched.ios.${compiler}/lib/lib${libName}.a"
			retcode=$?
			if [ $retcode -ne 0 ]; then
				echo "Failed to lipo 'lib${libName}.a', aborting..."
				rm -rf "$path"
				exit $retcode
			fi
		done
	fi
	if [ ! -d "$SRCLOC/upstream.patched.ios.${compiler}-fat.static" ]; then
		(cd "$SRCLOC" && \
			ln -s "upstream.patched.ios.${compiler}" "upstream.patched.ios.${compiler}-fat.static")
	fi
else
	echo "Only 'ios' is supported target, while '${targetOS}' was specified"
	exit 1
fi
