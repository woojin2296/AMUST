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
  XAUTHORITY_CANDIDATE="$HOME/.Xauthority"
else
  XAUTHORITY_CANDIDATE="$XAUTHORITY"
fi

if [[ ! -f "$XAUTHORITY_CANDIDATE" && -f /var/lib/lightdm/.Xauthority ]]; then
  XAUTHORITY_CANDIDATE="/var/lib/lightdm/.Xauthority"
fi

export XAUTHORITY="$XAUTHORITY_CANDIDATE"

for attempt in $(seq 1 120); do
  if [[ -x "$APP_PATH" ]]; then
    break
  fi
  echo "wait: executable not ready: $APP_PATH (attempt=$attempt/120)" >&2
  sleep 3
done

if [[ ! -x "$APP_PATH" ]]; then
  echo "error: executable still not found: $APP_PATH" >&2
  exit 1
fi

for attempt in $(seq 1 120); do
  if [[ -f "$XAUTHORITY" ]]; then
    break
  fi
  echo "wait: xauth file not ready: $XAUTHORITY (attempt=$attempt/120)" >&2
  sleep 3
done

if [[ ! -f "$XAUTHORITY" ]]; then
  echo "error: xauth file not found: $XAUTHORITY" >&2
  exit 1
fi

for attempt in $(seq 1 120); do
  if [[ -S "/tmp/.X11-unix/X${X_DISPLAY#:}" ]]; then
    break
  fi
  echo "wait: x11 socket not ready: /tmp/.X11-unix/X${X_DISPLAY#:} (attempt=$attempt/120)" >&2
  sleep 1
done

if [[ ! -S "/tmp/.X11-unix/X${X_DISPLAY#:}" ]]; then
  echo "error: x11 socket still not ready: /tmp/.X11-unix/X${X_DISPLAY#:}" >&2
  exit 1
fi

# Enable verbose Qt X logging for startup diagnostics.
export QT_LOGGING_RULES="qt.qpa.*=true"

# Run prebuilt Linux binary directly (use for boot/kiosk startup).
exec "$APP_PATH"
