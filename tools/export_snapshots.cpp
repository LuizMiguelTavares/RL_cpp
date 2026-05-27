#include "checkpointing.hpp"
#include "plot_export.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CheckpointEntry {
    int episode{0};
    std::filesystem::path path;
};

void print_usage(const char* program_name) {
    std::cout
        << "Usage:\n"
        << "  " << program_name << " --run <run_dir> [--out <output_dir>]\n\n"
        << "Examples:\n"
        << "  " << program_name << " --run runs/run_001\n"
        << "  " << program_name << " --run runs/run_001 --out runs/run_001/animation_data\n";
}

bool parse_episode_checkpoint_name(const std::filesystem::path& path, int& episode) {
    const std::string name = path.filename().string();

    static const std::regex pattern(R"(^ep_([0-9]+)$)");
    std::smatch match;

    if (!std::regex_match(name, match, pattern)) {
        return false;
    }

    episode = std::stoi(match[1].str());
    return true;
}

std::string zero_padded_episode_dir(int episode) {
    std::ostringstream oss;
    oss << "ep_" << std::setw(10) << std::setfill('0') << episode;
    return oss.str();
}

std::vector<CheckpointEntry> find_numbered_checkpoints(
    const std::filesystem::path& checkpoints_dir
) {
    if (!std::filesystem::exists(checkpoints_dir)) {
        throw std::runtime_error(
            "Checkpoints directory does not exist: " + checkpoints_dir.string()
        );
    }

    std::vector<CheckpointEntry> checkpoints;

    for (const auto& entry : std::filesystem::directory_iterator(checkpoints_dir)) {
        if (!entry.is_directory()) {
            continue;
        }

        int episode = 0;
        if (!parse_episode_checkpoint_name(entry.path(), episode)) {
            continue;
        }

        checkpoints.push_back(CheckpointEntry{episode, entry.path()});
    }

    std::sort(
        checkpoints.begin(),
        checkpoints.end(),
        [](const CheckpointEntry& a, const CheckpointEntry& b) {
            return a.episode < b.episode;
        }
    );

    return checkpoints;
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::filesystem::path run_dir;
        std::filesystem::path output_dir;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--run") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--run requires a value");
                }

                run_dir = argv[++i];
            } else if (arg == "--out") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--out requires a value");
                }

                output_dir = argv[++i];
            } else if (arg == "--help" || arg == "-h") {
                print_usage(argv[0]);
                return 0;
            } else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        if (run_dir.empty()) {
            throw std::runtime_error("Missing required argument: --run <run_dir>");
        }

        if (output_dir.empty()) {
            output_dir = run_dir / "animation_data";
        }

        const std::filesystem::path checkpoints_dir = run_dir / "checkpoints";

        std::filesystem::create_directories(output_dir);

        const std::vector<CheckpointEntry> checkpoints =
            find_numbered_checkpoints(checkpoints_dir);

        if (checkpoints.empty()) {
            throw std::runtime_error(
                "No numbered checkpoints found in: " + checkpoints_dir.string()
            );
        }

        const std::filesystem::path manifest_path = output_dir / "manifest.csv";
        std::ofstream manifest(manifest_path);

        if (!manifest) {
            throw std::runtime_error(
                "Failed to open manifest file: " + manifest_path.string()
            );
        }

        manifest << "episode,frame_dir\n";

        std::cout << "Run directory: " << run_dir << '\n';
        std::cout << "Checkpoints directory: " << checkpoints_dir << '\n';
        std::cout << "Output directory: " << output_dir << '\n';
        std::cout << "Found " << checkpoints.size() << " checkpoints.\n\n";

        for (const CheckpointEntry& checkpoint : checkpoints) {
            rl::LoadedCheckpoint loaded = rl::load_checkpoint(checkpoint.path);

            const int episode = loaded.last_completed_episode;
            const std::string frame_dir_name = zero_padded_episode_dir(episode);
            const std::filesystem::path frame_dir = output_dir / frame_dir_name;

            rl::export_gridworld_plot_data(
                frame_dir,
                loaded.env,
                loaded.agent,
                episode
            );

            manifest << episode << ',' << frame_dir_name << '\n';

            std::cout
                << "Exported episode "
                << episode
                << " -> "
                << frame_dir
                << '\n';
        }

        std::cout << "\nManifest saved to: " << manifest_path << '\n';
        std::cout << "Export finished.\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        print_usage(argv[0]);
        return 1;
    }
}