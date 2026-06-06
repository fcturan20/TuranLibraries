#!/usr/bin/env sh
set -eu

if ! command -v python3 >/dev/null 2>&1; then
  echo "python3 is not installed or not on PATH."
  exit 1
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SCRIPT_PATH="$SCRIPT_DIR/Scripts/regen.py"

if [ ! -f "$SCRIPT_PATH" ]; then
  echo "Script file not found: $SCRIPT_PATH"
  exit 1
fi

python3 "$SCRIPT_PATH" --gen
