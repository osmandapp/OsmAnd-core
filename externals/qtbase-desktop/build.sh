#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

targetOS=$1
compiler=$2
targetArch=$3

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
		(cd "$path" && ./configure -xplatform $platform $configuration -prefix $path)
		retcode=$?
		if [ $retcode -ne 0 ]; then
			echo "Failed to configure 'qtbase-desktop' for '$name', aborting..."
			rm -rf "$path"
			exit $retcode
		fi
	fi
	
	# Build
	(cd "$path" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	retcode=$?
	if [ $retcode -ne 0 ]; then
		echo "Failed to build 'qtbase-desktop' for '$name', aborting..."
		rm -rf "$path"
		exit $retcode
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

if [[ "$targetOS" == "linux" ]]; then
	QTBASE_CONFIGURATION=$(echo "
		-release -opensource -confirm-license -c++11 -largefile -no-accessibility -qt-sql-sqlite
		-no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
		-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
		-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib
		-v
	" | tr '\n' ' ')

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi
	
	if [[ "$compiler" == "gcc" ]]; then
		if [[ "$targetArch" == "i686" ]]; then
			echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"
			makeStaticAndSharedFlavor "linux.gcc-i686" "linux-g++-32" "$QTBASE_CONFIGURATION"
		elif [[ "$targetArch" == "amd64" ]]; then
			echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"
			makeStaticAndSharedFlavor "linux.gcc-amd64" "linux-g++-64" "$QTBASE_CONFIGURATION"
		else
			echo "Only 'i686' and 'amd64' are supported target architectures for '${compiler}' on '${targetOS}', while '${targetArch}' was specified"
			exit 1
		fi
	elif [[ "$compiler" == "clang" ]]; then
		if [[ "$targetArch" == "i686" ]]; then
			echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"
			makeStaticAndSharedFlavor "linux.clang-i686" "linux-clang-32" "$QTBASE_CONFIGURATION"
		elif [[ "$targetArch" == "amd64" ]]; then
			echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"
			makeStaticAndSharedFlavor "linux.clang-amd64" "linux-clang-64" "$QTBASE_CONFIGURATION"
		else
			echo "Only 'i686' and 'amd64' are supported target architectures for '${compiler}' on '${targetOS}', while '${targetArch}' was specified"
			exit 1
		fi
	else
		echo "Only 'gcc' and 'clang' are supported compilers for '${targetOS}' target, while '${compiler}' was specified"
		exit 1
	fi
elif [[ "$targetOS" == "macosx" ]]; then
	QTBASE_CONFIGURATION=$(echo "
		-debug-and-release -opensource -confirm-license -c++11 -largefile -no-accessibility -qt-sql-sqlite
		-no-qml-debug -qt-zlib -no-gif -no-libpng -no-libjpeg -no-openssl -qt-pcre
		-nomake examples -nomake tools -no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus
		-no-xcb -no-eglfs -no-directfb -no-linuxfb -no-kms -no-opengl -no-glib -no-framework
		-v
	" | tr '\n' ' ')

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi
	
	if [[ "$compiler" == "clang" ]]; then
		if [[ "$targetArch" == "i386" ]]; then
			echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"
			makeStaticAndSharedFlavor "macosx.clang-i386" "macx-clang-libc++-32" "$QTBASE_CONFIGURATION"
		elif [[ "$targetArch" == "x86_64" ]]; then
			echo "Going to build embedded Qt for ${targetOS}/${compiler}/${targetArch}"
			makeStaticAndSharedFlavor "macosx.clang-x86_64" "macx-clang-libc++-64" "$QTBASE_CONFIGURATION"
		else
			echo "Only 'i386' and 'x86_64' are supported target architectures for '${compiler}' on '${targetOS}', while '${targetArch}' was specified"
			exit 1
		fi
	else
		echo "Only 'clang' is supported compiler for '${targetOS}' target, while '${compiler}' was specified"
		exit 1
	fi
else
	echo "Only 'linux' and 'macosx' are supported targets, while '${targetOS}' was specified"
	exit 1
fi
