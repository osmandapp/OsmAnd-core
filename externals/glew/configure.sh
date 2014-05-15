#!/bin/bash

echo "Checking for bash..."
if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi
SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SRCLOC/../functions.sh"

configureExternalFromTarArchive "$SRCLOC" "https://sourceforge.net/projects/glew/files/glew/1.10.0/glew-1.10.0.tgz/download"
patchExternal "$SRCLOC"
