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
	if [ ! -d "$SRCLOC/upstream.patched.windows" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.windows"
	fi
	echo "Please execute build.bat from required environment"
fi

if [[ "$(uname -a)" == *Linux* ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.linux" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.linux"
		(cd "$SRCLOC/upstream.patched.linux" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.linux" && make -j`nproc`)
fi

if [[ "$(uname -a)" == *Darwin* ]]; then
	if [ ! -d "$SRCLOC/upstream.patched.darwin" ]; then
		cp -rf "$SRCLOC/upstream.patched" "$SRCLOC/upstream.patched.darwin"
		(cd "$SRCLOC/upstream.patched.darwin" && \
			./configure $QTBASE_CONFIGURATION)
	fi
	(cd "$SRCLOC/upstream.patched.darwin" && make -j`nproc`)
fi
