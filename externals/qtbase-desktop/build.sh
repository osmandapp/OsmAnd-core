#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)
OSMAND_ARCHITECTURES_SET=($*)

if [[ "$(uname -a)" =~ Cygwin ]]; then
	echo "Please execute build.bat from required environments for i686 and amd64"
	exit
fi

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
		if [ ! -d "$SRCLOC/upstream.patched.linux.i686.shared" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.i686.shared"
			(cd "$SRCLOC/upstream.patched.linux.i686.shared" && \
				./configure -xplatform linux-g++-32 -shared $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.linux.i686.shared" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

		if [ ! -d "$SRCLOC/upstream.patched.linux.i686.static" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.i686.static"
			(cd "$SRCLOC/upstream.patched.linux.i686.static" && \
				./configure -xplatform linux-g++-32 -static $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.linux.i686.static" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x64 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		if [ ! -d "$SRCLOC/upstream.patched.linux.amd64.shared" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.amd64.shared"
			(cd "$SRCLOC/upstream.patched.linux.amd64.shared" && \
				./configure -xplatform linux-g++-64 -shared $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.linux.amd64.shared" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

		if [ ! -d "$SRCLOC/upstream.patched.linux.amd64.static" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.amd64.static"
			(cd "$SRCLOC/upstream.patched.linux.amd64.static" && \
				./configure -xplatform linux-g++-64 -static $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.linux.amd64.static" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
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
		if [ ! -d "$SRCLOC/upstream.patched.darwin.i386.shared" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin.i386.shared"
			(cd "$SRCLOC/upstream.patched.darwin.i386.shared" && \
				./configure -xplatform macx-clang-libc++-32 -shared $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.darwin.i386.shared" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

		if [ ! -d "$SRCLOC/upstream.patched.darwin.i386.static" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin.i386.static"
			(cd "$SRCLOC/upstream.patched.darwin.i386.static" && \
				./configure -xplatform macx-clang-libc++-32 -static $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.darwin.i386.static" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi

	if [[ ${OSMAND_ARCHITECTURES_SET[*]} =~ x64 ]] || [[ -z "$OSMAND_ARCHITECTURES_SET" ]]; then
		if [ ! -d "$SRCLOC/upstream.patched.darwin.x86_64.shared" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin.x86_64.shared"
			(cd "$SRCLOC/upstream.patched.darwin.x86_64.shared" && \
				./configure -xplatform macx-clang-libc++-64 -shared $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.darwin.x86_64.shared" && make -j$OSMAND_BUILD_CPU_CORES_NUM)

		if [ ! -d "$SRCLOC/upstream.patched.darwin.x86_64.static" ]; then
			cp -rpf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin.x86_64.static"
			(cd "$SRCLOC/upstream.patched.darwin.x86_64.static" && \
				./configure -xplatform macx-clang-libc++-64 -static $QTBASE_CONFIGURATION)
		fi
		(cd "$SRCLOC/upstream.patched.darwin.x86_64.static" && make -j$OSMAND_BUILD_CPU_CORES_NUM)
	fi
fi
