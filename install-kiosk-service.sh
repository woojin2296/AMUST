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

cp "$SOURCE_SERVICE" "$TARGET_SERVICE"
systemctl daemon-reload
systemctl enable "$SERVICE_NAME"
systemctl restart "$SERVICE_NAME"

echo "Installed and restarted systemd service: $SERVICE_NAME"
