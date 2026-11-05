#!/usr/bin/env bash
# Builds StudioEncoder and the AW+ test program on Linux (preset ubuntu-x86_64, output in build_x86_64/).
#
#   bash scripts/linux-build.sh
set -euo pipefail
cd "$(dirname "$0")/.."

[ -f "$HOME/.cargo/env" ] && source "$HOME/.cargo/env"

cmake --preset ubuntu-x86_64
cmake --build --preset ubuntu-x86_64
cmake --build build_x86_64 --target prores-aw-test

echo
so=$(find build_x86_64 -maxdepth 2 -name 'obs-studioencoder.so*' -type f | head -1)
echo "plugin:       $so"
file "$so" || true
echo "test program: build_x86_64/prores-aw-test"
