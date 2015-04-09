#!/bin/bash

if [ -z "$BASH_VERSION" ]; then
	echo "Invalid shell, re-running using bash..."
	exec bash "$0" "$@"
	exit $?
fi

grep -rl 'namespace protobuf' . | xargs sed -i 's/namespace protobuf/namespace obf_protobuf/g'
grep -rl 'protobuf::' . | xargs sed -i 's/protobuf::/obf_protobuf::/g'
grep -rl '::protobuf' . | xargs sed -i 's/::protobuf/::obf_protobuf/g'
