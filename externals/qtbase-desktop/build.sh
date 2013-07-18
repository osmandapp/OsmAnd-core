#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)
OSMAND_ARCHITECTURES_SET=($*)

if [[ "$(uname -a)" =~ Cygwin ]]; then
	echo "Please execute build.bat from required environments for i686 and amd64"
	exit
fi

if [[ "$(uname -a)" =~ Linux ]]; then
	QTBASE_CONFIGURATION=\
		"-release -opensource -confirm-license -c++11 -static -largefile -no-accessibility -qt-sql-sqlite -no-javascript-jit -no-qml-debug "\
		"-qt-zlib -no-gif -no-libpng -no-libjpeg -openssl-linked -qt-pcre -nomake examples -nomake tools "\
		"-no-gui -no-widgets -no-nis -no-cups -no-iconv -no-icu -no-dbus -no-xcb -no-eglfs -no-directfb "\
		"-no-linuxfb -no-kms -no-opengl -no-glib "\
		"-v"

	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`nproc`
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		if [ ! -d "$SRCLOC/upstream.patched.linux.i686" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.i686"
			(cd "$SRCLOC/upstream.patched.linux.i686" && \
				./configure -xplatform linux-g++-32 $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.linux.i686" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x64 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		if [ ! -d "$SRCLOC/upstream.patched.linux.amd64" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.amd64"
			(cd "$SRCLOC/upstream.patched.linux.amd64" && \
				./configure -xplatform linux-g++-64 $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.linux.amd64" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi
fi

if [[ "$(uname -a)" =~ Darwin ]]; then
	if [[ -z "$OSMAND_BUILD_CPU_CORES_NUM" ]]; then
		OSMAND_BUILD_CPU_CORES_NUM=`sysctl hw.ncpu | awk '{print $2}'`
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x86 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		if [ ! -d "$SRCLOC/upstream.patched.darwin.i386" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin.i386"
			(cd "$SRCLOC/upstream.patched.darwin.i386" && \
				./configure -xplatform macx-clang-libc++-32 $QTBASE_CONFIGURATION -debug-and-release -no-framework)
		fi
		(cd "$SRCLOC/upstream.patched.darwin.i386" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x64 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		if [ ! -d "$SRCLOC/upstream.patched.darwin.x86_64" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin.x86_64"
			(cd "$SRCLOC/upstream.patched.darwin.x86_64" && \
				./configure -xplatform macx-clang-libc++-64 $QTBASE_CONFIGURATION -debug-and-release -no-framework)
		fi
		(cd "$SRCLOC/upstream.patched.darwin.x86_64" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi
fi
