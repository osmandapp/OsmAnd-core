#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename $(dirname "${BASH_SOURCE[0]}") )

(cd "$SRCLOC/upstream.patched.armeabi" && make -j`nproc`)
(cd "$SRCLOC/upstream.patched.armeabi-v7a" && make -j`nproc`)
(cd "$SRCLOC/upstream.patched.x86" && make -j`nproc`)
(cd "$SRCLOC/upstream.patched.mips" && make -j`nproc`)
