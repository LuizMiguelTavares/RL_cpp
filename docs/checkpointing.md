# Checkpointing and Snapshots

This document explains how `RL_cpp` saves training state, resumes experiments, and stores numbered snapshots for visualization.

---

## Overview

The project uses two related but different concepts:

```text
latest checkpoint     -> used for resume
numbered snapshots    -> used for visualization and offline analysis
```

Both are stored under:

```text
runs/<run_name>/checkpoints/
```

Example:

```text
runs/run_001/checkpoints/
├── latest/
├── ep_500/
├── ep_1000/
├── ep_5000/
└── ep_100000/
```

---

## Latest Checkpoint

The latest checkpoint lives at:

```text
runs/<run_name>/checkpoints/latest/
```

This is the official checkpoint used by resume mode.

When training resumes, the program loads:

```text
runs/<run_name>/checkpoints/latest/
```

and continues from the stored `last_completed_episode`.

---

## Numbered Snapshots

Numbered snapshots are saved as:

```text
runs/<run_name>/checkpoints/ep_<episode>/
```

Examples:

```text
ep_500/
ep_1000/
ep_10000/
ep_100000/
```

These snapshots are valid checkpoints, but their main purpose is:

```text
visualization
animation
offline analysis
```

They allow the Python tools to reconstruct how the value function and greedy policy evolved during training.

---

## What a Checkpoint Stores

Each checkpoint stores enough information to resume training consistently.

A checkpoint includes:

### Environment

- grid dimensions
- obstacle map
- start state
- goal state
- rewards
- shaping settings
- action settings
- maximum steps

### Agent

- Q-table
- cumulative weight table `C`
- tie-breaking noise
- discount factor
- behavior epsilon
- visit mode
- random generator state

### Metadata

- checkpoint version
- `last_completed_episode`

---

## `last_completed_episode`

`last_completed_episode` is the accumulated number of completed episodes for the run.

Example:

```text
Start:
    last_completed_episode = 0

After training 100000 episodes:
    last_completed_episode = 100000

After resuming and training 50000 more:
    last_completed_episode = 150000
```

This number is stored in checkpoint metadata and is used to continue counting correctly after resume.

---

## Safety Checkpointing

Safety checkpointing is configured with:

```json
"checkpoint_every": 100000
```

This periodically updates only:

```text
runs/<run_name>/checkpoints/latest/
```

Purpose:

```text
safe resume during long training runs
```

This checkpoint is overwritten periodically.

It is not intended to create animation frames.

---

## Snapshot Checkpointing

Snapshots are configured with either:

```json
"snapshot_every": 50000
```

or:

```json
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 },
  { "until": 1000000, "every": 50000 },
  { "until": 10000000, "every": 100000 }
]
```

These create numbered checkpoints:

```text
runs/<run_name>/checkpoints/ep_<episode>/
```

Purpose:

```text
2D plots
animations
analysis at different training stages
```

---

## `checkpoint_every` vs `snapshot_every`

These options intentionally do different jobs.

| Option | Saves | Purpose |
|---|---|---|
| `checkpoint_every` | `checkpoints/latest/` | Resume safety |
| `snapshot_every` | `checkpoints/ep_N/` | Visualization snapshots |
| `snapshot_schedule` | `checkpoints/ep_N/` | Variable-resolution visualization snapshots |

Use `checkpoint_every` to protect long runs.

Use `snapshot_every` or `snapshot_schedule` to control animation resolution.

---

## Uniform Snapshots

Uniform snapshots are configured with:

```json
"snapshot_every": 50000
```

This saves one numbered snapshot every 50,000 episodes of the current execution.

Example:

```text
ep_50000/
ep_100000/
ep_150000/
...
```

If `snapshot_schedule` is non-empty, `snapshot_every` is ignored.

---

## Variable-Resolution Snapshots

Variable-resolution snapshots are configured with:

```json
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 },
  { "until": 1000000, "every": 50000 },
  { "until": 10000000, "every": 100000 }
]
```

This is useful because early learning often changes faster than late learning.

Example interpretation:

```text
0 to 10,000 episodes:
    save every 500 episodes

10,001 to 100,000 episodes:
    save every 5,000 episodes

100,001 to 1,000,000 episodes:
    save every 50,000 episodes

1,000,001 to 10,000,000 episodes:
    save every 100,000 episodes
```

---

## Relative and Global Episode Numbers

The snapshot schedule is evaluated using:

```text
completed_this_run
```

The checkpoint directory is named using:

```text
completed_overall
```

Example:

```text
Starting checkpoint:
    last_completed_episode = 1,000,000

During current execution:
    completed_this_run = 50,000

Saved snapshot:
    ep_1050000
```

This preserves correct global episode numbers while still allowing each execution to use the configured schedule from the beginning of that execution.

---

## Final Checkpoint

At the end of each training execution, the program saves:

```text
checkpoints/latest/
checkpoints/ep_<last_completed_episode>/
```

This happens even if the final episode is not aligned with `checkpoint_every` or `snapshot_schedule`.

This guarantees that the final state is always saved.

---

## Graceful Stop

When `Ctrl+C` is pressed, training exits cleanly after the current episode.

The program then saves:

```text
checkpoints/latest/
checkpoints/ep_<last_completed_episode>/
```

This means interrupted runs can be resumed safely.

---

## Exporting Snapshots

Numbered checkpoints are not used directly by the animation script.

First, they are converted by the offline exporter:

```bash
./build/bin/rl_export_snapshots --run runs/<run_name>
```

or through:

```bash
./scripts/animate.sh
```

The exporter reads:

```text
runs/<run_name>/checkpoints/ep_N/
```

and writes:

```text
runs/<run_name>/animation_data/
```

Example:

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

---

## Manifest

The exporter also writes:

```text
runs/<run_name>/animation_data/manifest.csv
```

This file lists the exported frames in episode order.

Example:

```csv
episode,frame_dir
500,ep_0000000500
1000,ep_0000001000
5000,ep_0000005000
```

The animation script reads this manifest to build the video.

---

## Recommended Settings

For long runs where early training is visually important:

```json
"checkpoint_every": 100000,

"snapshot_every": 0,
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 },
  { "until": 1000000, "every": 50000 },
  { "until": 10000000, "every": 100000 }
]
```

This gives:

- frequent snapshots early;
- fewer snapshots later;
- reasonable disk usage;
- enough frames for a useful animation.

---

## Disk Usage Notes

Numbered snapshots can create many files.

If the run is very long and the snapshot interval is small, disk usage can grow quickly.

Use:

```bash
du -sh runs/<run_name>/checkpoints
du -sh runs/<run_name>/animation_data
```

to inspect storage usage.

---

## Practical Tips

- Use `checkpoint_every` for safety, not animation quality.
- Use `snapshot_schedule` when early training needs higher temporal resolution.
- Use a new `run_name` when experimenting with different snapshot settings.
- Do not manually edit checkpoint files.
- If animation data looks stale, delete `runs/<run_name>/animation_data/` and rerun the exporter.
