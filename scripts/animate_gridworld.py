from pathlib import Path
import argparse
import json

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation


def parse_args():
    parser = argparse.ArgumentParser(
        description="Create a GridWorld value/policy animation from exported snapshots."
    )

    parser.add_argument(
        "--data",
        type=Path,
        required=True,
        help="Path to animation_data directory containing manifest.csv.",
    )

    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output animation path. Use .mp4 or .gif.",
    )

    parser.add_argument(
        "--fps",
        type=int,
        default=10,
        help="Frames per second.",
    )

    parser.add_argument(
        "--dpi",
        type=int,
        default=160,
        help="Output DPI.",
    )

    parser.add_argument(
        "--max-frames",
        type=int,
        default=0,
        help="Maximum number of frames to use. 0 means all frames.",
    )

    return parser.parse_args()


def load_grid_map(frame_dir):
    with open(frame_dir / "grid_map.json", "r") as f:
        grid_map = json.load(f)

    height = int(grid_map["height"])
    width = int(grid_map["width"])

    start = (
        int(grid_map["start"]["row"]),
        int(grid_map["start"]["col"]),
    )

    goal = (
        int(grid_map["goal"]["row"]),
        int(grid_map["goal"]["col"]),
    )

    obstacles = {
        (int(obs["row"]), int(obs["col"]))
        for obs in grid_map["obstacles"]
    }

    return {
        "height": height,
        "width": width,
        "start": start,
        "goal": goal,
        "obstacles": obstacles,
    }


def load_frame(frame_dir, height, width):
    q_df = pd.read_csv(frame_dir / "q_table.csv")

    value = np.full((height, width), np.nan)
    greedy_dr = np.zeros((height, width))
    greedy_dc = np.zeros((height, width))
    is_obstacle = np.zeros((height, width), dtype=bool)
    is_goal = np.zeros((height, width), dtype=bool)

    for _, row in q_df.iterrows():
        r = int(row["row"])
        c = int(row["col"])

        is_obstacle[r, c] = bool(row["is_obstacle"])
        is_goal[r, c] = bool(row["is_goal"])

        value[r, c] = row["value"]
        greedy_dr[r, c] = row["greedy_delta_row"]
        greedy_dc[r, c] = row["greedy_delta_col"]

    value[is_obstacle] = np.nan

    return {
        "value": value,
        "greedy_dr": greedy_dr,
        "greedy_dc": greedy_dc,
        "is_obstacle": is_obstacle,
        "is_goal": is_goal,
    }

def print_progress(label, current, total):
    percent = 100.0 * current / total

    print(
        f"\r{label}: {current}/{total} ({percent:.1f}%)",
        end="",
        flush=True,
    )

    if current == total:
        print()


def setup_grid_axes(ax, height, width):
    ax.set_aspect("equal")

    ax.set_xlim(-0.5, width - 0.5)
    ax.set_ylim(height - 0.5, -0.5)

    ax.set_xticks(np.arange(width))
    ax.set_yticks(np.arange(height))

    ax.set_xticks(np.arange(-0.5, width, 1), minor=True)
    ax.set_yticks(np.arange(-0.5, height, 1), minor=True)
    ax.grid(which="minor", linewidth=0.5)

    ax.tick_params(which="minor", bottom=False, left=False)


def draw_static_elements(ax, grid_info):
    height = grid_info["height"]
    width = grid_info["width"]
    start = grid_info["start"]
    goal = grid_info["goal"]
    obstacles = grid_info["obstacles"]

    for r, c in obstacles:
        rect = plt.Rectangle(
            (c - 0.5, r - 0.5),
            1,
            1,
            fill=True,
            color="black",
            zorder=3,
        )
        ax.add_patch(rect)

    ax.scatter(
        start[1],
        start[0],
        marker="o",
        s=140,
        color="white",
        edgecolor="black",
        linewidth=1.5,
        label="Start",
        zorder=5,
    )

    ax.scatter(
        goal[1],
        goal[0],
        marker="*",
        s=220,
        color="white",
        edgecolor="black",
        linewidth=1.5,
        label="Goal",
        zorder=5,
    )

    setup_grid_axes(ax, height, width)
    ax.legend(loc="upper right")

def print_progress(label, current, total):
    percent = 100.0 * current / total

    print(
        f"\r{label}: {current}/{total} ({percent:.1f}%)",
        end="",
        flush=True,
    )

    if current == total:
        print()


def main():
    args = parse_args()

    data_dir = args.data
    manifest_path = data_dir / "manifest.csv"

    if not manifest_path.exists():
        raise FileNotFoundError(f"Manifest not found: {manifest_path}")

    manifest = pd.read_csv(manifest_path)

    if args.max_frames > 0:
        manifest = manifest.iloc[: args.max_frames].copy()

    if manifest.empty:
        raise RuntimeError("Manifest is empty.")

    frame_dirs = [
        data_dir / frame_dir
        for frame_dir in manifest["frame_dir"].tolist()
    ]

    episodes = manifest["episode"].astype(int).tolist()

    grid_info = load_grid_map(frame_dirs[0])
    height = grid_info["height"]
    width = grid_info["width"]

    print("Loading frame data...")

    frames = []

    for idx, frame_dir in enumerate(frame_dirs, start=1):
        frames.append(load_frame(frame_dir, height, width))

        if idx == len(frame_dirs) or idx % max(1, len(frame_dirs) // 100) == 0:
            print_progress("Loading frames", idx, len(frame_dirs))

    all_values = np.array([frame["value"] for frame in frames])
    finite_values = all_values[np.isfinite(all_values)]

    if finite_values.size == 0:
        raise RuntimeError("No finite values found in exported Q-table data.")

    vmin = float(np.min(finite_values))
    vmax = float(np.max(finite_values))

    is_obstacle = frames[0]["is_obstacle"]
    is_goal = frames[0]["is_goal"]

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    arrow_mask = (~is_obstacle) & (~is_goal)

    fig, ax = plt.subplots(figsize=(9, 8))

    initial_value = np.ma.array(frames[0]["value"], mask=is_obstacle)

    im = ax.imshow(
        initial_value,
        origin="upper",
        vmin=vmin,
        vmax=vmax,
        zorder=1,
    )

    draw_static_elements(ax, grid_info)

    arrow_scale = 0.35

    quiver = ax.quiver(
        X[arrow_mask],
        Y[arrow_mask],
        frames[0]["greedy_dc"][arrow_mask] * arrow_scale,
        frames[0]["greedy_dr"][arrow_mask] * arrow_scale,
        angles="xy",
        scale_units="xy",
        scale=1,
        pivot="middle",
        width=0.006,
        zorder=4,
    )

    title = ax.set_title(
        f"Value heatmap + greedy policy | Episode {episodes[0]}"
    )

    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label(r"$V(s) = \max_a Q(s,a)$")

    def update(frame_idx):
        frame = frames[frame_idx]

        value_masked = np.ma.array(frame["value"], mask=is_obstacle)

        im.set_data(value_masked)

        quiver.set_UVC(
            frame["greedy_dc"][arrow_mask] * arrow_scale,
            frame["greedy_dr"][arrow_mask] * arrow_scale,
        )

        title.set_text(
            f"Value heatmap + greedy policy | Episode {episodes[frame_idx]}"
        )

        return im, quiver, title

    args.output.parent.mkdir(parents=True, exist_ok=True)

    suffix = args.output.suffix.lower()

    if suffix == ".gif":
        writer = animation.PillowWriter(fps=args.fps)
    elif suffix == ".mp4":
        writer = animation.FFMpegWriter(fps=args.fps, bitrate=2500)
    else:
        raise RuntimeError("Unsupported output format. Use .mp4 or .gif")

    print("Writing animation...")

    with writer.saving(fig, str(args.output), args.dpi):
        for idx in range(len(frames)):
            update(idx)
            writer.grab_frame()

            if idx + 1 == len(frames) or (idx + 1) % max(1, len(frames) // 100) == 0:
                print_progress("Writing frames", idx + 1, len(frames))

    plt.close(fig)

    print(f"Animation saved to: {args.output}")


if __name__ == "__main__":
    main()