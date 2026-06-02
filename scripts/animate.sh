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

list_enabled_animations() {
    python3 - "$ANIMATION_CONFIG" <<'PY'
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])

if not path.exists():
    raise SystemExit(f"Animation config not found: {path}")

with open(path, "r") as f:
    cfg = json.load(f)

animations = cfg.get("animations", [])

if not animations:
    raise SystemExit("No animations configured in animation.json")

for item in animations:
    enabled = item.get("enabled", True)

    if not enabled:
        continue

    plot_type = item.get("type", "")
    output_name = item.get("output_name", "")

    if not plot_type:
        raise SystemExit("Animation entry missing required field: type")

    if not output_name:
        output_name = f"{plot_type}_animation"

    print(f"{plot_type}|{output_name}")
PY
}

RUN_NAME="$(read_anim_json run_name run_001)"
FORMAT="$(read_anim_json format mp4)"
FPS="$(read_anim_json fps 10)"
DPI="$(read_anim_json dpi 140)"
MAX_FRAMES="$(read_anim_json max_frames 0)"
EXPORT_SNAPSHOTS="$(read_anim_json export_snapshots true)"
ENCODER="$(read_anim_json encoder cpu)"

if [[ "$FORMAT" != "mp4" && "$FORMAT" != "gif" ]]; then
    echo "Invalid animation format: $FORMAT"
    echo "Allowed formats: mp4, gif"
    exit 1
fi

if [[ "$ENCODER" != "cpu" && "$ENCODER" != "nvenc" ]]; then
    echo "Invalid encoder: $ENCODER"
    echo "Allowed encoders: cpu, nvenc"
    exit 1
fi

RUN_DIR="runs/$RUN_NAME"
DATA_DIR="$RUN_DIR/animation_data"
OUTPUT_DIR="$RUN_DIR/figures"
EXPORTER="./build/bin/rl_export_snapshots"
MPLCONFIGDIR="${MPLCONFIGDIR:-/tmp/rl_cpp_matplotlib}"

echo "Animation config: $ANIMATION_CONFIG"
echo "Run name:         $RUN_NAME"
echo "Run dir:          $RUN_DIR"
echo "Data dir:         $DATA_DIR"
echo "Output dir:       $OUTPUT_DIR"
echo "Format:           $FORMAT"
echo "FPS:              $FPS"
echo "DPI:              $DPI"
echo "Max frames:       $MAX_FRAMES"
echo "Encoder:          $ENCODER"
echo "Export snapshots: $EXPORT_SNAPSHOTS"
echo

mkdir -p "$MPLCONFIGDIR"

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
echo "Generating enabled animations..."

while IFS="|" read -r PLOT_TYPE OUTPUT_NAME; do
    OUTPUT="$OUTPUT_DIR/$OUTPUT_NAME.$FORMAT"

    echo
    echo "----------------------------------------"
    echo "Plot type: $PLOT_TYPE"
    echo "Output:    $OUTPUT"
    echo "----------------------------------------"

    CMD=(
        env
        PYTHONNOUSERSITE=1
        MPLCONFIGDIR="$MPLCONFIGDIR"
        python3 scripts/animate_gridworld.py
        --data "$DATA_DIR"
        --output "$OUTPUT"
        --fps "$FPS"
        --dpi "$DPI"
        --plot-type "$PLOT_TYPE"
        --encoder "$ENCODER"
    )

    if [[ "$MAX_FRAMES" != "0" ]]; then
        CMD+=(--max-frames "$MAX_FRAMES")
    fi

    "${CMD[@]}"
done < <(list_enabled_animations)

echo
echo "All enabled animations finished."
