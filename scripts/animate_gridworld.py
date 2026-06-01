from pathlib import Path
import argparse
import json

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation


def parse_args():
    parser = argparse.ArgumentParser(
        description="Create GridWorld animations from exported snapshots."
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
        default=140,
        help="Output DPI.",
    )

    parser.add_argument(
        "--max-frames",
        type=int,
        default=0,
        help="Maximum number of frames to use. 0 means all frames.",
    )

    parser.add_argument(
        "--plot-type",
        type=str,
        default="value_policy",
        choices=["value_policy", "state_visits", "state_updates"],
        help="Animation type.",
    )

    parser.add_argument(
        "--encoder",
        type=str,
        default="cpu",
        choices=["cpu", "nvenc"],
        help="MP4 encoder backend. Use nvenc for NVIDIA GPU encoding.",
    )

    return parser.parse_args()


def print_progress(label, current, total):
    percent = 100.0 * current / total

    print(
        f"\r{label}: {current}/{total} ({percent:.1f}%)",
        end="",
        flush=True,
    )

    if current == total:
        print()


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
    visit_count = np.full((height, width), np.nan)
    update_count = np.full((height, width), np.nan)

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

        visit_count[r, c] = row.get("visit_count", 0)
        update_count[r, c] = row.get("update_count", 0)

    value[is_obstacle] = np.nan
    visit_count[is_obstacle] = np.nan
    update_count[is_obstacle] = np.nan

    return {
        "value": value,
        "visit_count": visit_count,
        "update_count": update_count,
        "greedy_dr": greedy_dr,
        "greedy_dc": greedy_dc,
        "is_obstacle": is_obstacle,
        "is_goal": is_goal,
    }


def frame_scalar(frame, plot_type):
    if plot_type == "value_policy":
        return frame["value"]

    if plot_type == "state_visits":
        return np.log1p(frame["visit_count"])

    if plot_type == "state_updates":
        return np.log1p(frame["update_count"])

    raise RuntimeError(f"Unsupported plot type: {plot_type}")


def plot_label(plot_type):
    if plot_type == "value_policy":
        return r"$V(s) = \max_a Q(s,a)$"

    if plot_type == "state_visits":
        return r"$\log(1 + \mathrm{visits}(s))$"

    if plot_type == "state_updates":
        return r"$\log(1 + \mathrm{updates}(s))$"

    raise RuntimeError(f"Unsupported plot type: {plot_type}")


def plot_title(plot_type):
    if plot_type == "value_policy":
        return "Value heatmap + greedy policy"

    if plot_type == "state_visits":
        return "State visitation heatmap"

    if plot_type == "state_updates":
        return "State update heatmap"

    raise RuntimeError(f"Unsupported plot type: {plot_type}")


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


def make_writer(output_path, fps, encoder):
    suffix = output_path.suffix.lower()

    if suffix == ".gif":
        return animation.PillowWriter(fps=fps)

    if suffix == ".mp4":
        if encoder == "nvenc":
            return animation.FFMpegWriter(
                fps=fps,
                codec="h264_nvenc",
                bitrate=-1,
                extra_args=[
                    "-preset", "fast",
                    "-cq", "23",
                    "-pix_fmt", "yuv420p",
                ],
            )

        return animation.FFMpegWriter(
            fps=fps,
            codec="libx264",
            bitrate=2500,
            extra_args=[
                "-pix_fmt", "yuv420p",
            ],
        )

    raise RuntimeError("Unsupported output format. Use .mp4 or .gif")


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

    all_scalars = np.array([
        frame_scalar(frame, args.plot_type)
        for frame in frames
    ])

    finite_values = all_scalars[np.isfinite(all_scalars)]

    if finite_values.size == 0:
        raise RuntimeError("No finite values found in exported data.")

    vmin = float(np.min(finite_values))
    vmax = float(np.max(finite_values))

    if vmin == vmax:
        vmax = vmin + 1.0

    is_obstacle = frames[0]["is_obstacle"]
    is_goal = frames[0]["is_goal"]

    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    arrow_mask = (~is_obstacle) & (~is_goal)

    fig, ax = plt.subplots(figsize=(9, 8))

    initial_scalar = np.ma.array(
        frame_scalar(frames[0], args.plot_type),
        mask=is_obstacle,
    )

    im = ax.imshow(
        initial_scalar,
        origin="upper",
        vmin=vmin,
        vmax=vmax,
        zorder=1,
    )

    draw_static_elements(ax, grid_info)

    arrow_scale = 0.35
    quiver = None

    if args.plot_type == "value_policy":
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
        f"{plot_title(args.plot_type)} | Episode {episodes[0]}"
    )

    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label(plot_label(args.plot_type))

    def update(frame_idx):
        frame = frames[frame_idx]

        scalar_masked = np.ma.array(
            frame_scalar(frame, args.plot_type),
            mask=is_obstacle,
        )

        im.set_data(scalar_masked)

        if quiver is not None:
            quiver.set_UVC(
                frame["greedy_dc"][arrow_mask] * arrow_scale,
                frame["greedy_dr"][arrow_mask] * arrow_scale,
            )

        title.set_text(
            f"{plot_title(args.plot_type)} | Episode {episodes[frame_idx]}"
        )

        artists = [im, title]

        if quiver is not None:
            artists.append(quiver)

        return artists

    args.output.parent.mkdir(parents=True, exist_ok=True)

    writer = make_writer(args.output, args.fps, args.encoder)

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