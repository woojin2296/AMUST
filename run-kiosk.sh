#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

APP_PATH="$ROOT_DIR/build/linux-Release/amust"
X_DISPLAY=${DISPLAY:-:0}
export DISPLAY="$X_DISPLAY"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

if [[ -z "${XAUTHORITY:-}" ]]; then
  export XAUTHORITY="$HOME/.Xauthority"
fi

if [[ ! -x "$APP_PATH" ]]; then
  echo "error: executable not found: $APP_PATH" >&2
  exit 126
fi

if [[ ! -S "/tmp/.X11-unix/X${X_DISPLAY#:}" ]]; then
  for _ in $(seq 1 20); do
    [[ -S "/tmp/.X11-unix/X${X_DISPLAY#:}" ]] && break
    sleep 1
  done
fi

if [[ ! -S "/tmp/.X11-unix/X${X_DISPLAY#:}" ]]; then
  echo "error: X socket not ready: /tmp/.X11-unix/X${X_DISPLAY#:}" >&2
  exit 1
fi

# Run prebuilt Linux binary directly (use for boot/kiosk startup).
exec "$APP_PATH"
