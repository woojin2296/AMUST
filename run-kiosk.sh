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

while [[ ! -x "$APP_PATH" ]]; do
  echo "wait: executable not ready: $APP_PATH" >&2
  sleep 3
done

if [[ -z "${XAUTHORITY:-}" ]]; then
  export XAUTHORITY="$HOME/.Xauthority"
fi

while [[ ! -S "/tmp/.X11-unix/X${X_DISPLAY#:}" ]]; do
  echo "wait: x11 socket not ready: /tmp/.X11-unix/X${X_DISPLAY#:}" >&2
  sleep 1
done

# Run prebuilt Linux binary directly (use for boot/kiosk startup).
exec "$APP_PATH"
