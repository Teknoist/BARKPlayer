#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-$ROOT_DIR/armhf-toolchain.cmake}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
MINIMAL="${BARKPLAYER_BUILD_MINIMAL:-ON}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "Missing toolchain file: $TOOLCHAIN_FILE" >&2
    echo "Create armhf-toolchain.cmake or set TOOLCHAIN_FILE=/path/to/file." >&2
    exit 2
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBARKPLAYER_BUILD_MINIMAL="$MINIMAL"

cmake --build "$BUILD_DIR" -- -j"$JOBS"
