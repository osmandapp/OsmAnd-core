#!/bin/bash

SRCLOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
"$SRCLOC/embed-resources.py" "$@"
