# RL_cpp

A C++ project for studying **tabular Reinforcement Learning**, currently focused on **GridWorld** and **Monte Carlo Off-Policy Control**.

The project keeps the **training core in C++** and uses external tools, mainly Python, for plotting, visualization, and post-analysis.

---

## Current Focus

- Tabular Reinforcement Learning
- GridWorld environments
- Monte Carlo Off-Policy Control
- CSV training logs
- Checkpoint/resume support
- Periodic snapshots for visualization
- Offline export for 2D plots and animations

---

## Quick Start

From the project root:

```bash
./scripts/build.sh
./scripts/train.sh
```

To generate visualization data and animations:

```bash
./scripts/animate.sh
```

---

## Main Configuration Files

```text
config.json      # experiment, environment, agent, training, checkpointing
build.json       # build options
animation.json   # animation/export options
```

---

## Main Outputs

For a run named `run_001`, outputs are written to:

```text
runs/run_001/
```

Typical contents:

```text
runs/run_001/
├── checkpoints/       # latest checkpoint and numbered snapshots
├── export/            # final exported GridWorld/Q-table data
├── animation_data/    # exported data from checkpoint snapshots
├── figures/           # generated plots and animations
└── train_history*.csv # per-episode training logs
```

---

## Main Commands

Build:

```bash
./scripts/build.sh
```

Train:

```bash
./scripts/train.sh
```

Generate animation:

```bash
./scripts/animate.sh
```

Run the binary manually:

```bash
./build/bin/rl_main --config config.json
```

Force resume manually:

```bash
./build/bin/rl_main --config config.json --resume
```

---

## Current Status

The project currently supports:

- GridWorld creation and obstacle generation
- Monte Carlo Off-Policy Control
- deterministic greedy target policy
- epsilon-greedy behavior policy
- weighted importance sampling
- first-visit and every-visit modes
- JSON-based configuration
- CSV logging
- checkpoint save/load
- resume training
- graceful `Ctrl+C` interruption
- periodic safety checkpoints
- numbered snapshots for visualization
- offline snapshot export
- 2D GridWorld animation workflow

---

## Documentation

Detailed documentation is organized in `docs/`:

```text
docs/
├── configuration.md
├── training.md
├── checkpointing.md
├── visualization.md
├── scripts.md
└── project_structure.md
```

Recommended reading order:

1. [Configuration](docs/configuration.md)
2. [Training](docs/training.md)
3. [Checkpointing and snapshots](docs/checkpointing.md)
4. [Visualization](docs/visualization.md)
5. [Scripts](docs/scripts.md)
6. [Project structure](docs/project_structure.md)

---

## Important Semantics

`episodes_this_run` means the number of episodes to train **in the current execution**, not the total lifetime training budget.

`runs/<run_name>/checkpoints/latest` is the official resume checkpoint.

Numbered checkpoints such as `ep_100000` are used as snapshots for visualization and analysis.

---

## Project Direction

Planned next steps include:

- polishing the visualization workflow
- adding training-curve plots
- improving multi-run and multi-seed analysis
- adding more tabular RL algorithms
- improving experimental documentation