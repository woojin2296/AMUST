#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# Run prebuilt Linux binary directly (use for boot/kiosk startup).
APP_PATH="$ROOT_DIR/build/linux-Release/amust"

if [[ ! -x "$APP_PATH" ]]; then
  echo "error: executable not found: $APP_PATH" >&2
  exit 126
fi

exec "$APP_PATH"
