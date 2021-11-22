#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../../../build/utils/functions.sh"

prepareUpstreamFromGit "$SRCLOC" "https://github.com/qt/qtbase" "v5.15.2"
patchUpstream "$SRCLOC"

# Check if tools are present
if [[ "$(uname -a)" == *Cygwin* ]]; then
	if [ ! -f "$SRCLOC/tools.windows.pack" ]; then
		echo "Downloading Qt build tools for Windows..."
		curl -L https://qt.gitorious.org/qt/qt5/archive-tarball/dev > "$SRCLOC/tools.windows.pack" || { echo "Failure" 1>&2; exit; }
	fi
	if [ ! -d "$SRCLOC/tools.windows" ]; then
		echo "Extracting Qt build tools for Windows..."
		mkdir -p "$SRCLOC/tools.windows"
		tar -xf "$SRCLOC/tools.windows.pack" -C "$SRCLOC/tools.windows" "qt-qt5/gnuwin32" --strip=2
		chmod +x "$SRCLOC/tools.windows/bin"/*
	fi
fi
