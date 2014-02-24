#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Switch to default protoc if not provided
if [ -z "$PROTOC" ]; then
    PROTOC=`which protoc`
fi
if [ -z "$PROTOC" ]; then
	echo "'protoc' not found, terminating"
	exit 1
fi
echo "Using $PROTOC"

"$PROTOC" --cpp_out="$SRCLOC" --proto_path="$SRCLOC/../../resources/protos" "$SRCLOC/../../resources/protos/OBF.proto"
