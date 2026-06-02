# HANDOFF

## Project

`RL_cpp`

## Purpose

`RL_cpp` is a C++ project for studying **tabular Reinforcement Learning**, currently focused on:

- GridWorld environments
- Monte Carlo Off-Policy Control
- checkpoint/resume infrastructure
- CSV logging
- experiment organization
- 2D visualization and animation of learned policies/value functions

The main design principle is:

```text
training core      -> C++
analysis/plotting  -> Python or MATLAB
configuration      -> JSON
```

---

## Current Architecture

The project is organized around a small C++ RL core plus helper tools and scripts.

```text
RL_cpp/
├── include/
├── src/
├── tools/
├── scripts/
├── docs/
├── runs/
├── CMakeLists.txt
├── config.json
├── build.json
├── animation.json
├── README.md
└── HANDOFF.md
```

---

## Main C++ Components

### `GridWorld`

Responsible for:

- grid dimensions
- obstacle generation
- start/goal states
- action set
- transition logic
- reward logic
- episode truncation through `max_steps`
- optional reward shaping
- solvability checks

### `Episode`

Stores one generated trajectory:

- rows
- cols
- actions
- rewards
- termination flag
- truncation flag

### `MonteCarloAgent`

Current agent implementation.

Supports:

- tabular `Q`
- cumulative importance weights `C`
- deterministic greedy target policy
- epsilon-greedy behavior policy
- weighted importance sampling
- first-visit and every-visit modes
- fixed `tie_noise` for deterministic but less biased tie-breaking
- greedy rollout for policy inspection

### `training`

Responsible for:

- generating episodes
- running the training loop
- updating the agent
- collecting metrics
- writing CSV rows
- calling an after-episode callback
- handling graceful stop after the current episode

### `checkpointing`

Responsible for:

- saving environment state
- saving agent state
- saving RNG-related state
- saving metadata
- loading checkpoints for resume

### `plot_export`

Responsible for exporting data used by Python visualization:

- `grid_map.json`
- `q_table.csv`

### `config`

Responsible for loading:

```text
config.json
```

into typed C++ config structures.

---

## Executables

The CMake build currently produces:

```text
build/bin/rl_main
build/bin/rl_export_snapshots
```

### `rl_main`

Main training executable.

Typical use:

```bash
./build/bin/rl_main --config config.json
```

Optional CLI resume override:

```bash
./build/bin/rl_main --config config.json --resume
```

### `rl_export_snapshots`

Offline exporter for animation data.

Typical use:

```bash
./build/bin/rl_export_snapshots --run runs/<run_name>
```

It reads numbered checkpoints:

```text
runs/<run_name>/checkpoints/ep_N/
```

and writes:

```text
runs/<run_name>/animation_data/
```

---

## Helper Scripts

The project currently uses three main Bash scripts:

```text
scripts/build.sh
scripts/train.sh
scripts/animate.sh
```

### `build.sh`

Builds the project using `build.json`.

Typical use:

```bash
./scripts/build.sh
```

### `train.sh`

Runs training using `config.json`.

Typical use:

```bash
./scripts/train.sh
```

Important: `train.sh` does **not** build automatically. Build first when C++ code changes.

### `animate.sh`

Exports snapshots and generates the GridWorld animation using `animation.json`.

Typical use:

```bash
./scripts/animate.sh
```

Important: `animate.sh` does **not** build automatically. It expects `rl_export_snapshots` to already exist.

---

## Configuration Files

### `config.json`

Controls:

- `run_name`
- `resume`
- environment settings
- agent settings
- training settings
- checkpointing
- visualization snapshot schedule

### `build.json`

Controls:

- build directory
- CMake generator
- build type
- clean build flag
- number of build jobs

### `animation.json`

Controls:

- which run to animate
- output file
- FPS
- DPI
- maximum number of frames
- whether to export snapshots before animation

---

## Important Training Semantics

### `episodes_this_run`

This is one of the most important semantics in the project.

```text
episodes_this_run = number of episodes to train in this execution
```

It is **not** the total lifetime training budget.

Example:

```text
First execution:
    last_completed_episode = 0
    episodes_this_run = 100000
    final total = 100000

Resume execution:
    last_completed_episode = 100000
    episodes_this_run = 100000
    final total = 200000
```

Do not change this semantic unless explicitly intended.

### `last_completed_episode`

Stored in checkpoint metadata.

Represents:

```text
global accumulated number of completed episodes for the run
```

It is used to resume correctly and to name numbered checkpoints.

### `resume`

Resume can be enabled by:

```json
"resume": true
```

or by:

```bash
--resume
```

Current behavior:

```text
if resume is true and checkpoints/latest exists:
    load latest checkpoint and continue

if resume is true and checkpoints/latest does not exist:
    start a new run instead of failing
```

This makes it safe to keep `"resume": true` while creating new runs.

---

## Checkpointing

There are two checkpoint concepts.

### Latest checkpoint

Stored at:

```text
runs/<run_name>/checkpoints/latest/
```

Purpose:

```text
resume safety
```

This is the official checkpoint loaded by resume.

### Numbered snapshots

Stored as:

```text
runs/<run_name>/checkpoints/ep_<episode>/
```

Purpose:

```text
visualization
animation
offline analysis
```

They are also valid checkpoints, but they are primarily used as snapshots of the learning process.

---

## `checkpoint_every` vs `snapshot_every` / `snapshot_schedule`

These must remain separate.

### `checkpoint_every`

Updates:

```text
checkpoints/latest/
```

Purpose:

```text
safe resume during long training
```

### `snapshot_every`

Creates:

```text
checkpoints/ep_N/
```

Purpose:

```text
uniform visualization snapshots
```

### `snapshot_schedule`

Creates:

```text
checkpoints/ep_N/
```

using variable temporal resolution.

Purpose:

```text
higher frame resolution early in training
lower frame resolution later in training
```

Example:

```json
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 },
  { "until": 1000000, "every": 50000 },
  { "until": 10000000, "every": 100000 }
]
```

The schedule is evaluated using `completed_this_run`, but the snapshot name uses the global `completed_overall`.

---

## Graceful Interruption

Training supports `Ctrl+C`.

When interrupted, the program:

1. finishes the current episode;
2. updates the agent;
3. logs the episode;
4. exits the training loop;
5. saves `checkpoints/latest`;
6. saves `checkpoints/ep_<last_completed_episode>`;
7. exits cleanly.

Do not save checkpoints directly inside the signal handler. The signal handler should only set a stop flag.

---

## Logging

Training logs are written as CSV files in:

```text
runs/<run_name>/
```

Examples:

```text
train_history.csv
train_history_resume_from_100001.csv
```

Current per-episode metrics include:

- `episode`
- `episode_return`
- `episode_length`
- `success`
- `updates_applied`
- `break_happened`
- `episode_time_sec`
- `generation_time_sec`
- `update_time_sec`

---

## Visualization Workflow

The visualization workflow is:

```text
1. train with numbered snapshots
2. export snapshots offline
3. generate animation with Python
```

Main command:

```bash
./scripts/animate.sh
```

Current Python animation:

```text
scripts/animate_gridworld.py
```

Current animation shows:

```text
background = V(s) = max_a Q(s,a)
arrows     = greedy policy argmax_a Q(s,a)
black cells = obstacles
markers    = start and goal
title      = global episode number
```

The script prints progress while loading frames and writing the animation.

---

## Generated Output Structure

For a run named `run_001`:

```text
runs/run_001/
├── checkpoints/
│   ├── latest/
│   ├── ep_500/
│   ├── ep_1000/
│   └── ...
├── export/
│   ├── grid_map.json
│   └── q_table.csv
├── animation_data/
│   ├── manifest.csv
│   ├── ep_0000000500/
│   │   ├── grid_map.json
│   │   └── q_table.csv
│   └── ...
├── figures/
│   └── value_policy_animation.mp4
└── train_history*.csv
```

---

## Documentation Status

The README has been compacted and detailed documentation has been split into:

```text
docs/configuration.md
docs/training.md
docs/checkpointing.md
docs/visualization.md
docs/scripts.md
docs/project_structure.md
```

The README should remain a compact entry point.

Detailed explanations should go into `docs/`.

---

## Current Algorithmic Status

The infrastructure is working.

Current known algorithm behavior:

- high `break_rate` can still happen;
- effective updates per episode may be low;
- greedy rollout can still form local loops;
- these are currently treated as algorithm/setup behavior, not infrastructure bugs.

The current algorithm is useful for studying MC off-policy behavior, but it is not yet a polished benchmark implementation.

---

## Current Development Status

Recently implemented:

- graceful `Ctrl+C` stop
- periodic safety checkpointing
- separated safety checkpoints from visualization snapshots
- variable-resolution snapshot schedule
- offline snapshot exporter
- Python animation with progress output
- helper scripts for build/train/animation
- compact README
- docs split into multiple Markdown files

---

## Recommended Workflow

When C++ code changes:

```bash
./scripts/build.sh
./scripts/train.sh
./scripts/animate.sh
```

When only `config.json` changes:

```bash
./scripts/train.sh
./scripts/animate.sh
```

When only `animation.json` changes:

```bash
./scripts/animate.sh
```

---

## Files Expected to Be Committed

Commit source/config/docs/scripts:

```text
include/
src/
tools/
scripts/
docs/
CMakeLists.txt
README.md
HANDOFF.md
config.json
build.json
animation.json
```

Usually do not commit:

```text
build/
runs/
*.mp4
*.gif
__pycache__/
.ipynb_checkpoints/
```

---

## Suggested Next Steps

Good next steps:

1. verify all scripts from a clean clone;
2. update `.gitignore`;
3. polish animation aesthetics;
4. add static 2D plot generation script;
5. add training-curve plots from CSV logs;
6. add multi-seed experiment organization;
7. add additional tabular algorithms:
   - on-policy MC
   - Q-learning
   - SARSA
   - Dynamic Programming methods;
8. improve analysis of `break_rate` and off-policy update truncation.

---

## Notes for Future Assistants

Preserve these design decisions unless the user explicitly asks otherwise:

- training stays in C++;
- plotting/analysis stays outside C++;
- `episodes_this_run` is per execution;
- `last_completed_episode` is global accumulated total;
- `checkpoints/latest` is the official resume checkpoint;
- `ep_N` checkpoints are numbered snapshots;
- `checkpoint_every` is for resume safety;
- `snapshot_every` / `snapshot_schedule` are for visualization;
- scripts should not automatically rebuild unless explicitly designed to do so;
- avoid large refactors unless there is a strong reason.

Do not move plotting into C++ unless explicitly requested.

Do not break checkpoint/resume semantics.
