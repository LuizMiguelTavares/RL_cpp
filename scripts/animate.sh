#!/usr/bin/env bash

set -euo pipefail

ANIMATION_CONFIG="${1:-animation.json}"

read_anim_json() {
    local key="$1"
    local default="$2"

    python3 - "$ANIMATION_CONFIG" "$key" "$default" <<'PY'
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

get_run_dir_from_config() {
    local config_path="$1"

    python3 - "$config_path" <<'PY'
import json
import sys
from pathlib import Path

config_path = Path(sys.argv[1])

with open(config_path, "r") as f:
    cfg = json.load(f)

run_name = (
    cfg.get("run_name")
    or cfg.get("run", {}).get("name")
    or "run_001"
)

print(Path("runs") / run_name)
PY
}

CONFIG_PATH="$(read_anim_json config_path config.json)"
RUN_DIR="$(read_anim_json run_dir "")"
OUTPUT="$(read_anim_json output "")"
FPS="$(read_anim_json fps 10)"
DPI="$(read_anim_json dpi 160)"
MAX_FRAMES="$(read_anim_json max_frames 0)"
EXPORT_SNAPSHOTS="$(read_anim_json export_snapshots true)"

if [[ -z "$RUN_DIR" ]]; then
    RUN_DIR="$(get_run_dir_from_config "$CONFIG_PATH")"
fi

if [[ -z "$OUTPUT" ]]; then
    OUTPUT="$RUN_DIR/figures/value_policy_animation.mp4"
fi

EXPORTER="./build/bin/rl_export_snapshots"

echo "Animation config: $ANIMATION_CONFIG"
echo "Training config:  $CONFIG_PATH"
echo "Run dir:          $RUN_DIR"
echo "Output:           $OUTPUT"
echo "FPS:              $FPS"
echo "DPI:              $DPI"
echo "Max frames:       $MAX_FRAMES"
echo "Export snapshots: $EXPORT_SNAPSHOTS"
echo

if [[ "$EXPORT_SNAPSHOTS" == "true" ]]; then
    if [[ ! -x "$EXPORTER" ]]; then
        echo "Executable not found: $EXPORTER"
        echo "Run './scripts/build.sh' first."
        exit 1
    fi

    echo "Exporting snapshots..."
    "$EXPORTER" --run "$RUN_DIR"
fi

echo
echo "Generating animation..."

CMD=(
    python3 scripts/animate_gridworld.py
    --data "$RUN_DIR/animation_data"
    --output "$OUTPUT"
    --fps "$FPS"
    --dpi "$DPI"
)

if [[ "$MAX_FRAMES" != "0" ]]; then
    CMD+=(--max-frames "$MAX_FRAMES")
fi

"${CMD[@]}"

echo
echo "Animation finished."
echo "Saved to: $OUTPUT"