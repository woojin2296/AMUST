#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build first
"$ROOT_DIR/build.sh"

APP_BIN="$ROOT_DIR/build/amust.app/Contents/MacOS/amust"
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

