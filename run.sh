#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build first
"$ROOT_DIR/build.sh"

OS_NAME="$(uname -s)"
case "$OS_NAME" in
  Darwin) PLATFORM="macos" ;;
  Linux) PLATFORM="linux" ;;
  *) PLATFORM="$(echo "$OS_NAME" | tr '[:upper:]' '[:lower:]')" ;;
esac

BUILD_TYPE="${AMUST_BUILD_TYPE:-Release}"
BUILD_DIR="$ROOT_DIR/build/${PLATFORM}-${BUILD_TYPE}"

if [[ "$PLATFORM" == "macos" ]]; then
  APP_BIN="$BUILD_DIR/amust.app/Contents/MacOS/amust"
else
  APP_BIN="$BUILD_DIR/amust"
fi

if [[ ! -x "$APP_BIN" ]]; then
  echo "error: built app not found at: $APP_BIN" >&2
  exit 1
fi

# If already running, terminate then restart.
PIDS="$(pgrep -x amust || true)"
if [[ -n "${PIDS}" ]]; then
  echo "stopping running amust: ${PIDS}"
  kill ${PIDS} 2>/dev/null || true

  for _ in {1..20}; do
    if ! pgrep -x amust >/dev/null; then
      break
    fi
    sleep 0.1
  done

  if pgrep -x amust >/dev/null; then
    echo "force-killing amust"
    pkill -9 -x amust 2>/dev/null || true
  fi
fi

echo "starting: $APP_BIN"
exec "$APP_BIN"
