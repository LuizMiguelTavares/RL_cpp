#!/usr/bin/env bash

set -euo pipefail

BUILD_CONFIG="${1:-build.json}"

read_json() {
    local key="$1"
    local default="$2"

    python3 - "$BUILD_CONFIG" "$key" "$default" <<'PY'
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])
key = sys.argv[2]
default = sys.argv[3]

if not path.exists():
    print(default)
    sys.exit(0)

with open(path, "r") as f:
    cfg = json.load(f)

value = cfg.get(key, default)

if isinstance(value, bool):
    print("true" if value else "false")
else:
    print(value)
PY
}

CLEAN="$(read_json clean true)"
BUILD_DIR="$(read_json build_dir build)"
GENERATOR="$(read_json generator Ninja)"
BUILD_TYPE="$(read_json build_type Release)"
JOBS="$(read_json jobs 0)"

if [[ "$JOBS" == "0" ]]; then
    JOBS="$(nproc)"
fi

echo "Build config: $BUILD_CONFIG"
echo "Build dir:    $BUILD_DIR"
echo "Generator:    $GENERATOR"
echo "Build type:   $BUILD_TYPE"
echo "Clean build:  $CLEAN"
echo "Jobs:         $JOBS"
echo

if [[ "$CLEAN" == "true" ]]; then
    echo "Removing previous build directory..."
    rm -rf "$BUILD_DIR"
fi

echo "Configuring..."
cmake -S . -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo
echo "Building..."
cmake --build "$BUILD_DIR" -j"$JOBS"

echo
echo "Build finished."
