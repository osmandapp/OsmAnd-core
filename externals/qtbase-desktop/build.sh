#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $(dirname "${BASH_SOURCE[0]}") )

QTBASE_CONFIGURATION=\
"-opensource -confirm-license "\
"-nomake examples -nomake demos -nomake tests -nomake docs "\
"-qt-sql-sqlite "\
"-no-gui -no-widgets -no-opengl -no-accessibility -no-linuxfb -no-directfb -no-eglfs -no-xcb -no-qml-debug -no-javascript-jit "\
"-c++11 -shared -release "\
"-v"

if [[ "$(uname -a)" == *Cygwin* ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.windows.i686" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.windows.i686"
	fi
	if [ ! -d "$SRCLOC/upstream.patched.windows.amd64" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.windows.amd64"
	fi
	echo "Please execute build.bat from required environments for i686 and amd64"
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
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.darwin" && make -j`nproc`)
fi
