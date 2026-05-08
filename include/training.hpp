#pragma once

#include <functional>
#include <vector>

#include "episode.hpp"
#include "gridworld.hpp"
#include "logging.hpp"
#include "mc_agent.hpp"

namespace rl {

struct TrainingConfig {
    int episodes{5000};
    int max_steps{500};
    int print_every{100};
    int flush_every{100};

    // 0 disables periodic checkpoints.
    int checkpoint_every{0};

    // Called after a completed episode when a periodic checkpoint is due.
    // Argument: number of episodes completed in the current execution.
    std::function<void(int)> checkpoint_callback{};

    // Should return true when training should stop after the current episode.
    std::function<bool()> should_stop{};
};

struct TrainingHistory {
    std::vector<double> episode_return;
    std::vector<int> episode_length;
    std::vector<int> success;
    std::vector<int> updates_applied;
    std::vector<int> break_happened;
    std::vector<double> episode_time_sec;
    std::vector<double> generation_time_sec;
    std::vector<double> update_time_sec;

    int completed_episodes{0};
    bool interrupted{false};
};

Episode generate_episode(
    GridWorld& env,
    MonteCarloOffPolicyAgent& agent,
    int max_steps
);

TrainingHistory train_mc_offpolicy(
    GridWorld& env,
    MonteCarloOffPolicyAgent& agent,
    const TrainingConfig& config,
    CSVLogger* logger = nullptr
);

}  // namespace rl