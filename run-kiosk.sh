#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

APP_PATH="$ROOT_DIR/build/linux-Release/amust"

if [[ ! -x "$APP_PATH" ]]; then
  echo "error: executable not found: $APP_PATH" >&2
  exit 126
fi

# EGLFS (KMS/DRM direct) — no X11/Wayland needed.
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-eglfs}"
export QT_QPA_EGLFS_ALWAYS_SET_MODE="${QT_QPA_EGLFS_ALWAYS_SET_MODE:-1}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"

exec "$APP_PATH"
