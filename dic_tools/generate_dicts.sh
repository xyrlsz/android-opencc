#!/bin/bash
# Linux/macOS wrapper: generate ALL .ocd2 dictionaries and copy configs
# Usage: ./generate_dicts.sh [--force] [--opencc /path/to/opencc]

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
exec python3 "$SCRIPT_DIR/generate_dicts.py" "$@"