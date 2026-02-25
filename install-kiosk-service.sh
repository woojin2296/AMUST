#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_NAME="amust-kiosk"
SOURCE_SERVICE="$ROOT_DIR/amust-kiosk.service"
TARGET_SERVICE="/etc/systemd/system/${SERVICE_NAME}.service"

if [[ $EUID -ne 0 ]]; then
  echo "This script must be run with root privileges. Example: sudo $0" >&2
  exit 1
fi

# Detect the login user who invoked sudo, or fallback to current user.
SERVICE_USER="${SUDO_USER:-$USER}"
SERVICE_HOME="$(getent passwd "$SERVICE_USER" | cut -d: -f6)"

if [[ -z "$SERVICE_HOME" ]]; then
  echo "Unable to resolve home directory for service user: $SERVICE_USER" >&2
  exit 1
fi

sed -e "s|@SERVICE_USER@|${SERVICE_USER}|g" \
    -e "s|@SERVICE_USER_HOME@|${SERVICE_HOME}|g" \
    -e "s|@ROOT_DIR@|${ROOT_DIR}|g" \
    "$SOURCE_SERVICE" > "$TARGET_SERVICE"

systemctl daemon-reload
systemctl enable "$SERVICE_NAME"
systemctl restart "$SERVICE_NAME"

echo "Installed and restarted systemd service: $SERVICE_NAME"
