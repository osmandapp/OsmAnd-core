#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $SRCLOC)

QTBASE_CONFIGURATION=\
"-opensource -confirm-license "\
"-nomake examples -nomake demos -nomake tests -nomake docs "\
"-qt-sql-sqlite "\
"-no-accessibility -no-opengl -no-kms -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit "\
"-c++11 -shared -release "\
"-v"

if [[ "$(uname -a)" == *Cygwin* ]]; then
	echo "Please execute build.bat from required environments for i686 and amd64"
	exit
fi

if [[ "$(uname -a)" == *Linux* ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.linux.i686" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.i686"
		(cd "$SRCLOC/upstream.patched.linux.i686" && \
			./configure -xplatform linux-g++-32 $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.linux.i686" && make -j`nproc`)

	if [ ! -d "$SRCLOC/upstream.patched.linux.amd64" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux.amd64"
		(cd "$SRCLOC/upstream.patched.linux.amd64" && \
			./configure -xplatform linux-g++-64 $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.linux.amd64" && make -j`nproc`)
fi

if [[ "$(uname -a)" == *Darwin* ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.darwin" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin"
		(cd "$SRCLOC/upstream.patched.darwin" && \
			./configure -xplatform macx-clang-libc++ $QTBASE_CONFIGURATION -accessibility -opengl)
	fi
	(cd "$SRCLOC/upstream.patched.darwin" && make -j`nproc`)
fi
