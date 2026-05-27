#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rl {

struct EnvironmentConfig {
    int width{50};
    int height{50};

    double obstacle_density{0.22};
    std::string obstacle_mode{"cluster"};
    unsigned int seed{30};
    int cluster_size{5};

    int start_row{0};
    int start_col{0};
    int goal_row{-1};
    int goal_col{-1};

    bool allow_diagonal{false};
    double diagonal_cost{-1.0};

    double reward_goal{100.0};
    double reward_obstacle{-10.0};
    double reward_step{-0.0001};

    std::string shaping{"euclidean"};

    bool ensure_solvable{true};
    int obstacle_generation_tries{100};
};

struct AgentConfig {
    double gamma{0.99};
    double epsilon_behavior{0.20};
    std::string visit_mode{"first_visit"};
    unsigned int seed{42};
};

struct SnapshotScheduleEntry {
    int until{0};
    int every{0};
};

struct TrainingConfigFile {
    int episodes_this_run{27000};
    int max_steps{200};
    int print_every{4500};
    int flush_every{1000};

    // Periodic safety checkpoint.
    // Saves the latest checkpoint for resume.
    int checkpoint_every{0};

    // Periodic numbered snapshot for plotting/animation.
    // Ignored when snapshot_schedule is non-empty.
    int snapshot_every{0};

    // Variable-resolution numbered snapshots for plotting/animation.
    // Episodes are relative to the current execution.
    std::vector<SnapshotScheduleEntry> snapshot_schedule{};
};

struct RunConfig {
    std::string run_name{"run_001"};
    bool resume{false};

    EnvironmentConfig environment{};
    AgentConfig agent{};
    TrainingConfigFile training{};
};

RunConfig load_config(const std::filesystem::path& config_path);

}  // namespace rl