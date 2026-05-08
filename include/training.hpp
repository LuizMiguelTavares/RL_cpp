#pragma once

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