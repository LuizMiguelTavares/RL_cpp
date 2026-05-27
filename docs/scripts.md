# Scripts

This document explains the helper scripts used by `RL_cpp`.

The goal of these scripts is to avoid remembering long terminal commands and to keep the project workflow reproducible.

The project currently uses three main scripts:

```text
scripts/
├── build.sh
├── train.sh
└── animate.sh
```

Each script is intentionally small and focused.

---

## Overview

| Script | Purpose |
|---|---|
| `scripts/build.sh` | Configure and build the C++ project |
| `scripts/train.sh` | Run training using `config.json` |
| `scripts/animate.sh` | Export snapshots and generate the GridWorld animation |

Configuration files:

```text
build.json       # build options
config.json      # training and experiment options
animation.json   # animation options
```

---

## Permissions

After creating or modifying scripts, make sure they are executable:

```bash
chmod +x scripts/*.sh
```

---

## `build.sh`

Build command:

```bash
./scripts/build.sh
```

Optional custom build config:

```bash
./scripts/build.sh build.json
```

This script reads:

```text
build.json
```

and runs CMake.

---

## `build.json`

Example:

```json
{
  "clean": false,
  "build_dir": "build",
  "generator": "Ninja",
  "build_type": "Release",
  "jobs": 0
}
```

### Fields

| Field | Meaning |
|---|---|
| `clean` | Whether to remove the build directory before building |
| `build_dir` | CMake build directory |
| `generator` | CMake generator |
| `build_type` | CMake build type |
| `jobs` | Number of parallel jobs; `0` means use `nproc` |

---

## Clean Build

To force a clean build, set:

```json
"clean": true
```

in `build.json`, then run:

```bash
./scripts/build.sh
```

This removes the build directory before configuring and compiling.

For normal development, it is usually better to keep:

```json
"clean": false
```

because rebuilding from scratch every time is slower.

---

## Build Output

The main executables are generated in:

```text
build/bin/
```

Expected executables:

```text
build/bin/rl_main
build/bin/rl_export_snapshots
```

---

## `train.sh`

Training command:

```bash
./scripts/train.sh
```

Optional custom config:

```bash
./scripts/train.sh config.json
```

This script runs:

```bash
./build/bin/rl_main --config config.json
```

It does not build the project.

If the executable does not exist, run:

```bash
./scripts/build.sh
```

first.

---

## Resume Behavior

Resume behavior is controlled by `config.json`:

```json
"resume": true
```

or by manually running:

```bash
./build/bin/rl_main --config config.json --resume
```

The `train.sh` script does not add `--resume` automatically.

This keeps all experiment behavior centralized in `config.json`.

---

## `animate.sh`

Animation command:

```bash
./scripts/animate.sh
```

Optional custom animation config:

```bash
./scripts/animate.sh animation.json
```

This script reads:

```text
animation.json
```

and can:

1. run the C++ snapshot exporter;
2. generate the Python animation.

---

## `animation.json`

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

### Fields

| Field | Meaning |
|---|---|
| `config_path` | Path to the training config used to infer `run_name` |
| `run_dir` | Run directory to animate; empty means infer from `config_path` |
| `output` | Output animation file; empty means default path |
| `fps` | Frames per second |
| `dpi` | Output resolution |
| `max_frames` | Limit frame count for quick tests; `0` means all frames |
| `export_snapshots` | Whether to run `rl_export_snapshots` before generating the animation |

---

## Default Run Directory

If `run_dir` is empty in `animation.json`, the script reads `run_name` from `config_path`.

Example:

```json
{
  "config_path": "config.json",
  "run_dir": ""
}
```

If `config.json` contains:

```json
"run_name": "run_animation_test"
```

then the script uses:

```text
runs/run_animation_test/
```

---

## Default Output Path

If `output` is empty:

```json
"output": ""
```

then the animation is saved to:

```text
runs/<run_name>/figures/value_policy_animation.mp4
```

---

## Snapshot Export Step

When:

```json
"export_snapshots": true
```

`animate.sh` runs:

```bash
./build/bin/rl_export_snapshots --run runs/<run_name>
```

This creates or updates:

```text
runs/<run_name>/animation_data/
```

If this data already exists and you only want to regenerate the video, set:

```json
"export_snapshots": false
```

---

## Quick Animation Test

To test the animation without rendering all frames, set:

```json
"max_frames": 30
```

Then run:

```bash
./scripts/animate.sh
```

This is useful when adjusting visualization style, frame rate, or output format.

---

## Recommended Workflow

When C++ code changed:

```bash
./scripts/build.sh
./scripts/train.sh
./scripts/animate.sh
```

When only `config.json` changed:

```bash
./scripts/train.sh
./scripts/animate.sh
```

When only `animation.json` changed:

```bash
./scripts/animate.sh
```

---

## Manual Commands

The scripts are wrappers around these manual commands.

Build:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Train:

```bash
./build/bin/rl_main --config config.json
```

Export snapshots:

```bash
./build/bin/rl_export_snapshots --run runs/<run_name>
```

Animate:

```bash
python3 scripts/animate_gridworld.py \
  --data runs/<run_name>/animation_data \
  --output runs/<run_name>/figures/value_policy_animation.mp4 \
  --fps 10
```

---

## Why Scripts?

The scripts exist to:

- reduce command-line mistakes;
- avoid running the wrong binary;
- standardize the workflow;
- make experiments easier to repeat;
- keep project usage simple.

They are not part of the RL algorithm itself.

They are a project workflow layer.

---

## Practical Tips

- Run scripts from the project root.
- Do not call `train.sh` before building at least once.
- Keep `build.json`, `config.json`, and `animation.json` under version control.
- Use new `run_name` values for experiments you want to keep separate.
- Use `max_frames` when testing animation generation.
- Use `export_snapshots=false` when only regenerating the video from existing `animation_data`.
