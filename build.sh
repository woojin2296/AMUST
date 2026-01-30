#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

OS_NAME="$(uname -s)"
case "$OS_NAME" in
  Darwin) PLATFORM="macos" ;;
  Linux) PLATFORM="linux" ;;
  *) PLATFORM="$(echo "$OS_NAME" | tr '[:upper:]' '[:lower:]')" ;;
esac

BUILD_TYPE="${AMUST_BUILD_TYPE:-Release}"
BUILD_DIR="$ROOT_DIR/build/${PLATFORM}-${BUILD_TYPE}"
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"

# If this folder was copied from another machine/path, the CMake cache will
# reference the old source/build directories and CMake will refuse to proceed.
if [[ -f "$CACHE_FILE" ]]; then
  CACHED_SRC_DIR="$(awk -F= '/^CMAKE_HOME_DIRECTORY:INTERNAL=/{print $2; exit}' "$CACHE_FILE" || true)"
  if [[ -n "${CACHED_SRC_DIR:-}" && "$CACHED_SRC_DIR" != "$ROOT_DIR" ]]; then
    echo "cmake: stale cache detected (was: $CACHED_SRC_DIR, now: $ROOT_DIR); cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
  fi
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"
