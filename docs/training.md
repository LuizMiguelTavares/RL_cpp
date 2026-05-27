# Training

This document explains how training works in `RL_cpp`, including how to start a run, resume a run, interrupt training safely, and interpret the main training logs.

---

## Basic Training Command

The recommended way to train is:

```bash
./scripts/train.sh
```

By default, this uses:

```text
config.json
```

You can also pass a different config file:

```bash
./scripts/train.sh config/my_experiment.json
```

The script expects that the project has already been built. If the executable is missing, run:

```bash
./scripts/build.sh
```

---

## Manual Training Command

The main executable can also be called directly:

```bash
./build/bin/rl_main --config config.json
```

To force resume mode from the command line:

```bash
./build/bin/rl_main --config config.json --resume
```

The command-line `--resume` flag overrides the `resume` value in `config.json`.

---

## Run Directory

Each experiment writes to:

```text
runs/<run_name>/
```

The `run_name` comes from `config.json`:

```json
"run_name": "run_001"
```

Example output directory:

```text
runs/run_001/
```

---

## New Run

A new run is created when:

```json
"resume": false
```

In this mode, the program:

1. creates a new `GridWorld`;
2. creates a new `MonteCarloOffPolicyAgent`;
3. trains for `episodes_this_run`;
4. writes CSV logs;
5. saves checkpoints;
6. exports final plot data.

Typical command:

```bash
./scripts/train.sh
```

or manually:

```bash
./build/bin/rl_main --config config.json
```

---

## Resume Run

Resume mode is enabled with:

```json
"resume": true
```

or with:

```bash
./build/bin/rl_main --config config.json --resume
```

In resume mode, the program tries to load:

```text
runs/<run_name>/checkpoints/latest/
```

If this checkpoint exists, training continues from it.

If `resume` is true but the latest checkpoint does not exist, the program starts a new run instead of failing.

This makes it safe to keep:

```json
"resume": true
```

while testing new runs with new `run_name` values.

---

## `episodes_this_run`

The most important training field is:

```json
"episodes_this_run": 10000000
```

This means:

```text
number of episodes to train in this execution
```

It does not mean the total number of episodes the run should ever contain.

Example:

```text
First execution:
    last_completed_episode = 0
    episodes_this_run = 100000
    final total = 100000

Second execution with resume:
    last_completed_episode = 100000
    episodes_this_run = 100000
    final total = 200000

Third execution with resume:
    last_completed_episode = 200000
    episodes_this_run = 100000
    final total = 300000
```

---

## `last_completed_episode`

`last_completed_episode` is stored in checkpoint metadata.

It represents:

```text
the accumulated number of completed episodes for the run
```

It is global for the run, not local to the current execution.

When training resumes, the program loads this value from:

```text
runs/<run_name>/checkpoints/latest/meta.txt
```

and continues counting from there.

---

## Graceful Interruption

Training supports graceful interruption with:

```text
Ctrl+C
```

When `Ctrl+C` is pressed, the program does not stop in the middle of an episode.

Instead, it:

1. finishes the current episode;
2. updates the agent;
3. writes the episode log;
4. exits the training loop;
5. saves the latest checkpoint;
6. saves a numbered snapshot checkpoint;
7. exits cleanly.

Expected terminal message:

```text
Stop requested. Training will exit after completed episode ...
Checkpoint saved to: ...
Snapshot checkpoint saved to: ...
Graceful stop completed after episode ...
```

This makes interrupted runs resumable.

---

## Training Logs

Training logs are stored as CSV files inside:

```text
runs/<run_name>/
```

For a new run:

```text
runs/<run_name>/train_history.csv
```

For a resumed run:

```text
runs/<run_name>/train_history_resume_from_<episode>.csv
```

Example:

```text
runs/run_001/train_history_resume_from_100001.csv
```

---

## CSV Columns

Each row corresponds to one completed episode.

Typical columns:

| Column | Meaning |
|---|---|
| `episode` | Episode index within the current execution |
| `episode_return` | Total return of the episode |
| `episode_length` | Number of steps in the episode |
| `success` | Whether the goal was reached |
| `updates_applied` | Number of effective Monte Carlo updates |
| `break_happened` | Whether the off-policy update was truncated |
| `episode_time_sec` | Total episode processing time |
| `generation_time_sec` | Time spent generating the episode |
| `update_time_sec` | Time spent updating the agent |

---

## Terminal Output

During training, summary statistics are printed every `print_every` episodes.

Example:

```text
Ep 45000 | AvgReturn=... | AvgLen=... | Success=...% | AvgUpdates=... | BreakRate=...%
```

These values are computed over the most recent `print_every` episodes.

The print frequency is configured with:

```json
"print_every": 45000
```

---

## CSV Flush Frequency

The CSV logger is flushed every:

```json
"flush_every": 1000
```

Smaller values write data to disk more often.

Larger values reduce disk I/O but may lose more recent log rows if the process is killed unexpectedly.

Graceful `Ctrl+C` is safe, but hard termination with `kill -9`, a system crash, or power loss can still lose buffered data.

---

## Checkpoints During Training

There are two checkpoint-related mechanisms:

```text
checkpoint_every
snapshot_every / snapshot_schedule
```

They have different purposes.

### Safety checkpoint

```json
"checkpoint_every": 100000
```

This periodically updates:

```text
runs/<run_name>/checkpoints/latest/
```

Purpose:

```text
safe resume
```

### Snapshot checkpoints

```json
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 }
]
```

This saves numbered checkpoints:

```text
runs/<run_name>/checkpoints/ep_500/
runs/<run_name>/checkpoints/ep_1000/
...
```

Purpose:

```text
visualization
animation
offline analysis
```

More details are documented in:

```text
docs/checkpointing.md
docs/visualization.md
```

---

## Final Export

At the end of each execution, the program exports the final GridWorld/Q-table data to:

```text
runs/<run_name>/export/
```

Typical files:

```text
grid_map.json
q_table.csv
```

These files are used by Python visualization tools.

---

## Greedy Path Output

At the end of training, the program prints the greedy path induced by the current policy.

Example:

```text
Greedy path:
(0, 0) (0, 1) (1, 1) ...
```

This is useful for quickly checking whether the final greedy policy reaches the goal or falls into a loop.

For better analysis, use the exported visualization workflow described in:

```text
docs/visualization.md
```

---

## Current Algorithm Behavior

The current Monte Carlo Off-Policy Control implementation is functional, but the following behavior can still occur:

- high `break_rate`;
- few effective updates per episode;
- local loops in the greedy rollout.

This is currently treated as algorithm behavior, not an infrastructure bug.

The infrastructure supports logging, checkpointing, resuming, exporting, and visualizing this behavior.

---

## Recommended Workflow

Typical development workflow:

```bash
./scripts/build.sh
./scripts/train.sh
./scripts/animate.sh
```

When only changing `config.json`, there is usually no need to rebuild.

When changing C++ source files or `CMakeLists.txt`, rebuild with:

```bash
./scripts/build.sh
```

---

## Practical Tips

- Use a new `run_name` when testing a substantially different experiment.
- Keep `resume` true when you want to continue long experiments easily.
- If a run does not exist and `resume` is true, the program starts a new run.
- Use `Ctrl+C` rather than killing the process.
- Do not compare algorithms using only one random seed.
- For stronger experimental claims, run multiple seeds and analyze variability.
