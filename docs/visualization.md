# Visualization

This document explains the visualization workflow used by `RL_cpp`.

The project keeps training in C++ and uses Python for plotting, animation, and post-analysis.

---

## Overview

The visualization workflow has three stages:

```text
1. Train in C++
2. Export checkpoint snapshots offline
3. Generate plots/animations in Python
```

The main animation command is:

```bash
./scripts/animate.sh
```

This script can:

1. export numbered checkpoints into animation data;
2. generate a GridWorld value/policy animation.

---

## Output Directories

For a run named `run_001`, visualization-related outputs are stored in:

```text
runs/run_001/
├── export/
├── animation_data/
└── figures/
```

### `export/`

Contains the final exported state after training.

```text
runs/run_001/export/
├── grid_map.json
└── q_table.csv
```

This is useful for static plots of the final policy/value function.

### `animation_data/`

Contains exported data from numbered checkpoint snapshots.

```text
runs/run_001/animation_data/
├── manifest.csv
├── ep_0000000500/
│   ├── grid_map.json
│   └── q_table.csv
├── ep_0000001000/
│   ├── grid_map.json
│   └── q_table.csv
└── ...
```

This is used to generate animations.

### `figures/`

Contains generated figures and videos.

Example:

```text
runs/run_001/figures/
└── value_policy_animation.mp4
```

---

## Final Export

At the end of each training execution, the C++ program exports the final environment and Q-table to:

```text
runs/<run_name>/export/
```

Typical files:

```text
grid_map.json
q_table.csv
```

This export represents only the final state of the agent after the current execution.

---

## Snapshot Export

Animations require multiple snapshots across training time.

The training program saves numbered checkpoints such as:

```text
runs/<run_name>/checkpoints/ep_500/
runs/<run_name>/checkpoints/ep_1000/
runs/<run_name>/checkpoints/ep_5000/
```

These checkpoints are converted into visualization data by the offline exporter:

```bash
./build/bin/rl_export_snapshots --run runs/<run_name>
```

The exporter creates:

```text
runs/<run_name>/animation_data/
```

The recommended way to call this is through:

```bash
./scripts/animate.sh
```

---

## `animation.json`

Animation options are configured in:

```text
animation.json
```

Example:

```json
{
  "config_path": "config.json",
  "run_dir": "",
  "output": "",
  "fps": 10,
  "dpi": 160,
  "max_frames": 0,
  "export_snapshots": true
}
```

---

## Animation Options

| Field | Meaning |
|---|---|
| `config_path` | Training config used to infer `run_name` when `run_dir` is empty |
| `run_dir` | Run directory to animate |
| `output` | Output video/GIF path |
| `fps` | Frames per second |
| `dpi` | Output resolution |
| `max_frames` | Limit number of frames for quick tests |
| `export_snapshots` | Whether to run the C++ snapshot exporter before animation |

---

## Automatic Run Directory

If `run_dir` is empty:

```json
"run_dir": ""
```

then `scripts/animate.sh` reads `config_path`, gets `run_name`, and uses:

```text
runs/<run_name>
```

Example:

```json
"config_path": "config.json",
"run_dir": ""
```

with:

```json
"run_name": "run_animation_test"
```

uses:

```text
runs/run_animation_test/
```

---

## Automatic Output Path

If `output` is empty:

```json
"output": ""
```

then the animation is saved to:

```text
runs/<run_name>/figures/value_policy_animation.mp4
```

---

## Generate Animation

Recommended command:

```bash
./scripts/animate.sh
```

Manual command:

```bash
python3 scripts/animate_gridworld.py \
  --data runs/<run_name>/animation_data \
  --output runs/<run_name>/figures/value_policy_animation.mp4 \
  --fps 10 \
  --dpi 160
```

---

## Quick Test

To test the animation script without rendering all frames, set:

```json
"max_frames": 30
```

in `animation.json`.

Or manually:

```bash
python3 scripts/animate_gridworld.py \
  --data runs/<run_name>/animation_data \
  --output runs/<run_name>/figures/test_animation.mp4 \
  --fps 5 \
  --max-frames 30
```

This is useful when checking whether the plot style, scale, and animation speed are reasonable.

---

## MP4 and GIF

The Python script supports:

```text
.mp4
.gif
```

For MP4 output, `ffmpeg` is required.

Ubuntu installation:

```bash
sudo apt update
sudo apt install ffmpeg
```

GIF output requires `pillow`:

```bash
pip3 install --user pillow
```

---

## Python Dependencies

The animation script uses:

```text
numpy
pandas
matplotlib
pillow
```

Install with:

```bash
pip3 install --user numpy pandas matplotlib pillow
```

For MP4:

```bash
sudo apt install ffmpeg
```

---

## What the Animation Shows

The current animation shows:

```text
background = value heatmap V(s) = max_a Q(s, a)
arrows     = greedy policy argmax_a Q(s, a)
markers    = start and goal
black cells = obstacles
title      = global episode number
```

This makes it possible to see:

- how the value function forms;
- how the greedy policy changes;
- when the policy becomes stable;
- whether the final policy looks coherent;
- whether loops appear in the learned policy.

---

## Color Scale

The animation uses a fixed value scale across all frames.

This is important because if each frame used its own color range, the animation could be visually misleading.

Fixed color limits make changes across time easier to interpret.

---

## Snapshot Resolution

The temporal resolution of the animation is controlled by the numbered snapshots saved during training.

Use:

```json
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 },
  { "until": 1000000, "every": 50000 },
  { "until": 10000000, "every": 100000 }
]
```

to get high resolution early in training and lower resolution later.

This is usually better than using a single uniform interval, because early learning often changes faster.

---

## `checkpoint_every` Is Not Animation Resolution

`checkpoint_every` controls:

```text
checkpoints/latest/
```

It is used for resume safety.

Animation resolution comes from:

```text
snapshot_every
snapshot_schedule
```

not from `checkpoint_every`.

---

## Exporter Executable

The offline exporter is:

```text
./build/bin/rl_export_snapshots
```

It reads:

```text
runs/<run_name>/checkpoints/ep_N/
```

and writes:

```text
runs/<run_name>/animation_data/
```

It also writes:

```text
manifest.csv
```

which defines the animation frame order.

---

## Animation Script

The Python animation script is:

```text
scripts/animate_gridworld.py
```

It reads:

```text
animation_data/manifest.csv
```

and uses each frame directory listed in the manifest.

The script prints progress while:

- loading frames;
- writing video frames.

Example:

```text
Loading frame data...
Loading frames: 146/146 (100.0%)
Writing animation...
Writing frames: 146/146 (100.0%)
Animation saved to: ...
```

---

## Recommended Workflow

Typical workflow:

```bash
./scripts/build.sh
./scripts/train.sh
./scripts/animate.sh
```

If the project is already built and training is already done, only run:

```bash
./scripts/animate.sh
```

If `export_snapshots` is true, `animate.sh` will first run the C++ exporter and then generate the animation.

---

## Regenerating Animation Data

If the animation data is stale or inconsistent, remove it:

```bash
rm -rf runs/<run_name>/animation_data
```

Then rerun:

```bash
./scripts/animate.sh
```

---

## Regenerating Figures

If only the video needs to be regenerated, keep:

```text
animation_data/
```

and set:

```json
"export_snapshots": false
```

in `animation.json`.

Then run:

```bash
./scripts/animate.sh
```

This skips the C++ export step and only regenerates the video.

---

## Practical Tips

- Use MP4 for better quality and smaller files.
- Use GIF only for quick sharing or previews.
- Start with `max_frames` when testing plot aesthetics.
- Use `snapshot_schedule` to capture early learning more clearly.
- Keep `fps` around 8-15 for readable learning animations.
- Delete `animation_data/` when changing snapshot exports or checkpoint content.
