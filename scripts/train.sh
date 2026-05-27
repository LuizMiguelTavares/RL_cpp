#!/usr/bin/env bash

set -euo pipefail

CONFIG_PATH="${1:-config.json}"

EXECUTABLE="./build/bin/rl_main"

if [[ ! -x "$EXECUTABLE" ]]; then
    echo "Executable not found: $EXECUTABLE"
    echo "Run './scripts/build.sh' first."
    exit 1
fi

echo "Running training..."
echo "Config: $CONFIG_PATH"
echo

"$EXECUTABLE" --config "$CONFIG_PATH"