#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# Run prebuilt Linux binary directly (use for boot/kiosk startup).
/bin/bash "$ROOT_DIR/build/linux-Release/amust"
