# Configuration

This document explains the main experiment configuration file used by `RL_cpp`.

The main training configuration lives in:

```text
config.json
```

This file controls:

- run name
- resume behavior
- GridWorld environment
- Monte Carlo agent
- training length
- checkpoint frequency
- snapshot frequency for visualization

Build and animation options are documented separately:

```text
build.json       # see docs/scripts.md
animation.json   # see docs/visualization.md
```

---

## Top-Level Fields

A typical `config.json` has this structure:

```json
{
  "run_name": "run_001",
  "resume": false,

  "environment": {},
  "agent": {},
  "training": {}
}
```

---

## `run_name`

```json
"run_name": "run_001"
```

This defines the output directory for the experiment:

```text
runs/<run_name>/
```

Example:

```json
"run_name": "run_animation_test"
```

creates:

```text
runs/run_animation_test/
```

---

## `resume`

```json
"resume": false
```

Controls whether the program should try to continue from:

```text
runs/<run_name>/checkpoints/latest
```

Semantics:

```text
resume = false
    Start a new run using the environment and agent settings from config.json.

resume = true
    If a valid latest checkpoint exists, load it and continue training.
    If no valid latest checkpoint exists, start a new run instead.
```

The command-line flag:

```bash
./build/bin/rl_main --config config.json --resume
```

forces resume mode even if `resume` is set to `false` in `config.json`.

---

## `environment`

The `environment` block configures the GridWorld.

Example:

```json
"environment": {
  "width": 25,
  "height": 25,
  "obstacle_density": 0.22,
  "obstacle_mode": "cluster",
  "seed": 30,
  "cluster_size": 5,

  "start_row": 0,
  "start_col": 0,
  "goal_row": -1,
  "goal_col": -1,

  "allow_diagonal": false,
  "diagonal_cost": -1.0,

  "reward_goal": 100.0,
  "reward_obstacle": -10.0,
  "reward_step": -0.0001,
  "shaping": "euclidean",

  "ensure_solvable": true,
  "obstacle_generation_tries": 100
}
```

### Main fields

| Field | Meaning |
|---|---|
| `width` | Number of grid columns |
| `height` | Number of grid rows |
| `obstacle_density` | Approximate fraction of obstacle cells |
| `obstacle_mode` | Obstacle generation mode |
| `seed` | Environment random seed |
| `cluster_size` | Cluster size used by clustered obstacle generation |
| `start_row`, `start_col` | Start state |
| `goal_row`, `goal_col` | Goal state |
| `allow_diagonal` | Enables diagonal actions when true |
| `diagonal_cost` | Cost/reward used for diagonal motion |
| `reward_goal` | Reward for reaching the goal |
| `reward_obstacle` | Reward/penalty for hitting obstacles |
| `reward_step` | Per-step reward/penalty |
| `shaping` | Reward shaping mode |
| `ensure_solvable` | Regenerate maps until start-goal connectivity is valid |
| `obstacle_generation_tries` | Maximum attempts to create a valid map |

### Goal position

Using:

```json
"goal_row": -1,
"goal_col": -1
```

means the goal is placed automatically at the bottom-right corner:

```text
(height - 1, width - 1)
```

---

## `agent`

The `agent` block configures the Monte Carlo Off-Policy Control agent.

Example:

```json
"agent": {
  "gamma": 0.99,
  "epsilon_behavior": 0.20,
  "visit_mode": "first_visit",
  "seed": 42
}
```

### Fields

| Field | Meaning |
|---|---|
| `gamma` | Discount factor |
| `epsilon_behavior` | Epsilon used by the behavior policy |
| `visit_mode` | Monte Carlo update mode |
| `seed` | Agent random seed |

### `visit_mode`

Supported values:

```text
first_visit
every_visit
```

---

## `training`

The `training` block controls the execution length, logging, checkpoints, and visualization snapshots.

Example:

```json
"training": {
  "episodes_this_run": 10000000,
  "max_steps": 200,
  "print_every": 45000,
  "flush_every": 1000,

  "checkpoint_every": 100000,

  "snapshot_every": 0,
  "snapshot_schedule": [
    { "until": 10000, "every": 500 },
    { "until": 100000, "every": 5000 },
    { "until": 1000000, "every": 50000 },
    { "until": 10000000, "every": 100000 }
  ]
}
```

---

## `episodes_this_run`

```json
"episodes_this_run": 10000000
```

This means:

```text
number of episodes to train in this execution
```

It is not the total lifetime training budget.

Example:

```text
Run 1:
    last_completed_episode = 0
    episodes_this_run = 100000
    final total = 100000

Run 2 with resume:
    last_completed_episode = 100000
    episodes_this_run = 100000
    final total = 200000
```

---

## `max_steps`

```json
"max_steps": 200
```

Maximum number of steps allowed in one episode.

If the agent does not reach the goal before this limit, the episode is truncated.

---

## `print_every`

```json
"print_every": 45000
```

Controls how often training statistics are printed to the terminal.

Example:

```text
Ep 45000 | AvgReturn=... | AvgLen=... | Success=...%
```

---

## `flush_every`

```json
"flush_every": 1000
```

Controls how often the CSV logger is flushed to disk.

A smaller value reduces the chance of losing logs if the process stops unexpectedly, but may increase disk I/O.

---

## `checkpoint_every`

```json
"checkpoint_every": 100000
```

Controls periodic safety checkpointing.

This updates:

```text
runs/<run_name>/checkpoints/latest/
```

Purpose:

```text
resume safety
```

This is not primarily for animation. It exists so long training runs can be resumed safely.

---

## `snapshot_every`

```json
"snapshot_every": 50000
```

Controls uniform numbered snapshot saving.

This creates checkpoints such as:

```text
runs/<run_name>/checkpoints/ep_50000/
runs/<run_name>/checkpoints/ep_100000/
runs/<run_name>/checkpoints/ep_150000/
```

Purpose:

```text
visualization
animation
offline analysis
```

If `snapshot_schedule` is non-empty, `snapshot_every` is ignored.

---

## `snapshot_schedule`

```json
"snapshot_schedule": [
  { "until": 10000, "every": 500 },
  { "until": 100000, "every": 5000 },
  { "until": 1000000, "every": 50000 },
  { "until": 10000000, "every": 100000 }
]
```

Controls variable-resolution snapshot saving.

This is useful because early learning often changes quickly, while later learning changes more slowly.

Semantics:

```text
until 10,000 episodes:
    save snapshot every 500 episodes

until 100,000 episodes:
    save snapshot every 5,000 episodes

until 1,000,000 episodes:
    save snapshot every 50,000 episodes

until 10,000,000 episodes:
    save snapshot every 100,000 episodes
```

The values are relative to the current execution.

Example with resume:

```text
starting_completed_episode = 1,000,000
completed_this_run = 50,000
completed_overall = 1,050,000
```

The schedule decision uses `completed_this_run`, while the saved checkpoint name uses `completed_overall`.

---

## Recommended Training Configuration

For a long run with useful animation snapshots:

```json
"training": {
  "episodes_this_run": 10000000,
  "max_steps": 200,
  "print_every": 45000,
  "flush_every": 1000,

  "checkpoint_every": 100000,

  "snapshot_every": 0,
  "snapshot_schedule": [
    { "until": 10000, "every": 500 },
    { "until": 100000, "every": 5000 },
    { "until": 1000000, "every": 50000 },
    { "until": 10000000, "every": 100000 }
  ]
}
```

---

## Practical Notes

- Use a new `run_name` when testing new experiment configurations.
- Keep `checkpoint_every` reasonably large to avoid excessive disk writes.
- Use `snapshot_schedule` when you want high animation resolution early in training.
- Use `snapshot_every` only when a uniform snapshot interval is enough.
- If a run already exists and `resume` is false, the program may overwrite logs/checkpoints depending on the same output paths being reused.
