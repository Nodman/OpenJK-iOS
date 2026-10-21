#!/usr/bin/env bash
# Keep the build-dir/set/mode invocation used by build-ios.sh and P9.
set -eu
exec python3 "$(dirname "$0")/check-modules.py" "$@"
