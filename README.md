# RL_cpp

A C++ project for studying **tabular Reinforcement Learning**, currently focused on:

- **GridWorld**
- **Monte Carlo Off-Policy Control**
- **checkpoint save/load**
- **resume training**
- **CSV logging**
- **JSON-based configuration**

The project is designed to keep the **training core in C++** and leave **plotting and post-analysis** to external tools such as Python or MATLAB.

---

## Quick Start

### 1. Install dependencies (Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake nlohmann-json3-dev
```

### 2. Build

From the project root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc)
```

### 3. Run

From the project root:

```bash
cd ..
./build/bin/rl_main --config config.json
```

To resume from the latest checkpoint:

```bash
./build/bin/rl_main --config config.json --resume
```

---

## Current Status

At the moment, the project already supports:

- GridWorld environment
- Monte Carlo off-policy control with:
  - deterministic greedy target policy
  - epsilon-greedy behavior policy
  - weighted importance sampling
  - first-visit / every-visit modes
- Training configuration through `config.json`
- Saving logs to CSV
- Saving checkpoints
- Resuming training from the latest checkpoint

---

## Project Structure

```text
RL_cpp/
├── build/
├── include/
│   ├── checkpointing.hpp
│   ├── config.hpp
│   ├── episode.hpp
│   ├── gridworld.hpp
│   ├── logging.hpp
│   ├── mc_agent.hpp
│   ├── training.hpp
│   └── types.hpp
├── runs/
├── scripts/
├── src/
│   ├── checkpointing.cpp
│   ├── config.cpp
│   ├── gridworld.cpp
│   ├── logging.cpp
│   ├── main.cpp
│   ├── mc_agent.cpp
│   ├── training.cpp
│   └── types.cpp
├── CMakeLists.txt
└── config.json
```

---

## Main Idea

The project is split into clear components:

### `GridWorld`
Responsible for:
- map generation
- obstacles
- rewards
- terminal/truncated logic
- agent motion inside the grid

### `Episode`
Stores one full episode as:
- rows
- cols
- actions
- rewards
- terminated / truncated flags

### `MonteCarloOffPolicyAgent`
Responsible for:
- Q table
- cumulative weights table `C`
- tie-breaking noise
- greedy action selection
- behavior action selection
- off-policy Monte Carlo update from a full episode

### `training`
Responsible for:
- generating episodes
- running the training loop
- collecting metrics
- optionally logging metrics to CSV

### `checkpointing`
Responsible for:
- saving environment state
- saving agent state
- loading checkpoints
- restoring the training continuation point

### `config`
Responsible for:
- loading experiment settings from `config.json`

### `logging`
Responsible for:
- writing per-episode metrics to CSV files

---

## Dependencies

### Required
- C++17
- CMake >= 3.16
- `nlohmann/json`

### Ubuntu installation

```bash
sudo apt update
sudo apt install build-essential cmake nlohmann-json3-dev
```

---

## Build

From the project root:

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc)
```

Executable:

```bash
./bin/rl_main
```

---

## Recommended Way to Run

Run from the **project root** so relative paths work as expected:

```bash
cd ~/RL_cpp
./build/bin/rl_main --config config.json
```

---

## Configuration

The project uses a `config.json` file.

Example:

```json
{
  "run_name": "run_001",
  "resume": false,

  "environment": {
    "width": 50,
    "height": 50,
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
  },

  "agent": {
    "gamma": 0.99,
    "epsilon_behavior": 0.20,
    "visit_mode": "first_visit",
    "seed": 42
  },

  "training": {
    "episodes_this_run": 27000,
    "max_steps": 200,
    "print_every": 4500,
    "flush_every": 1000
  }
}
```

---

## Running Experiments

### New training run

```bash
./build/bin/rl_main --config config.json
```

If `resume` is `false`, the project:
- creates a new environment
- creates a new agent
- trains for `episodes_this_run`
- saves checkpoints at the end of the run

### Resume training

You can resume in two ways.

#### Option 1: set `resume: true` in `config.json`

```json
"resume": true
```

Then run:

```bash
./build/bin/rl_main --config config.json
```

#### Option 2: force resume from CLI

```bash
./build/bin/rl_main --config config.json --resume
```

This loads:

```text
runs/<run_name>/checkpoints/latest
```

and trains for **more** `episodes_this_run`.

> `--resume` can force resume mode even if `resume` is set to `false` in `config.json`.

---

## Important Training Semantics

### `episodes_this_run`

This means:

> how many episodes should be trained **in this execution**

It does **not** mean the total lifetime budget.

Example:

- first run: `episodes_this_run = 27000`
  - total completed becomes `27000`
- resume with `episodes_this_run = 27000`
  - total completed becomes `54000`
- resume again with `episodes_this_run = 27000`
  - total completed becomes `81000`

### `last_completed_episode`

This value is stored in the checkpoint metadata and represents the **global accumulated total** already trained for that run.

---

## Outputs

For a run named `run_001`, the project writes into:

```text
runs/run_001/
```

### Logs

Examples:
- `train_history.csv`
- `train_history_resume_from_27001.csv`
- `train_history_resume_from_54001.csv`

Each CSV stores one row per episode.

### Checkpoints

Examples:
- `runs/run_001/checkpoints/latest/`
- `runs/run_001/checkpoints/ep_27000/`
- `runs/run_001/checkpoints/ep_54000/`
- `runs/run_001/checkpoints/ep_81000/`

---

## What Is Saved in a Checkpoint

A checkpoint stores:

### Environment
- grid
- start
- goal
- dimensions
- rewards
- shaping settings
- max steps
- diagonal settings

### Agent
- `Q`
- `C`
- `tie_noise`
- gamma
- epsilon behavior
- visit mode
- RNG state

### Metadata
- checkpoint version
- last completed episode

This is enough to resume training consistently.

---

## CSV Metrics Currently Logged

Each episode log row contains:

- `episode`
- `episode_return`
- `episode_length`
- `success`
- `updates_applied`
- `break_happened`
- `episode_time_sec`
- `generation_time_sec`
- `update_time_sec`

These logs are intended to be analyzed later using Python or MATLAB.

---

## Current Algorithm Details

The current implementation uses:

- **target policy**: deterministic greedy with respect to `Q`
- **behavior policy**: epsilon-greedy
- **importance sampling**: weighted
- **visit mode**: configurable
  - `first_visit`
  - `every_visit`

A small fixed `tie_noise` is used to avoid collapsing all greedy ties to action 0.

---

## Current Algorithm Behavior

The current Monte Carlo off-policy control implementation is functional, but it still often exhibits:

- very high `break_rate`
- relatively few effective updates per episode
- local loops in the greedy rollout

At this stage, this is understood as a property of the current method/setup rather than an infrastructure bug.

---

## About Performance

This project is very fast because:

- the environment is tabular
- the data structures are compact
- the training loop is written in C++
- the agent uses linearized tables for:
  - `Q`
  - `C`
  - `tie_noise`

Typical episode times are currently in the order of microseconds in the present setup.

---

## Visualization Strategy

This project intentionally keeps training and logging in C++, while plots and post-analysis are expected to be done externally using Python or MATLAB.

---

## Current Limitations

At the moment, the project **does not yet** support:

- periodic checkpoint saving during training
- graceful interruption with `Ctrl+C`
- plotting inside C++
- external evaluation scripts included in the repository
- other algorithms such as:
  - Q-learning
  - SARSA
  - DP / Value Iteration / Policy Iteration

These are planned next steps.

---

## Recommended Workflow

### Train

```bash
./build/bin/rl_main --config config.json
```

### Resume

```bash
./build/bin/rl_main --config config.json --resume
```

### Analyze

Use the generated CSV files in:
- Python
- MATLAB
- any other plotting environment

---

## Example Development Workflow

1. Edit `config.json`
2. Build

   ```bash
   cd build
   cmake ..
   cmake --build . -j$(nproc)
   ```

3. Run

   ```bash
   cd ..
   ./build/bin/rl_main --config config.json
   ```

4. Resume later if needed

   ```bash
   ./build/bin/rl_main --config config.json --resume
   ```

---

## Notes on Paths

The project currently uses relative paths such as:

```text
runs/<run_name>/
config.json
```

For this reason, it is recommended to execute the binary from the **project root**.

Good:

```bash
cd ~/RL_cpp
./build/bin/rl_main --config config.json
```

Less safe:

```bash
cd ~/RL_cpp/build
./bin/rl_main
```

because relative paths may point somewhere else.

---

## Future Improvements

Planned next steps include:

- periodic checkpointing during training
- graceful stop on `Ctrl+C`
- Python/MATLAB plotting scripts
- more algorithms:
  - on-policy MC
  - Q-learning
  - SARSA
  - Dynamic Programming
- more flexible experiment management