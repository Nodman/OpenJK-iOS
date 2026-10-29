#!/usr/bin/env bash
# Thin wrapper; CI runs it after the iOS build.
set -eu
exec python3 "$(dirname "$0")/check-modules.py" "$@"
