#pragma once

#include <filesystem>

#include "gridworld.hpp"
#include "mc_agent.hpp"

namespace rl {

struct LoadedCheckpoint {
    GridWorld env;
    MonteCarloAgent agent;
    int last_completed_episode{0};
};

void save_checkpoint(
    const std::filesystem::path& checkpoint_dir,
    const GridWorld& env,
    const MonteCarloAgent& agent,
    int last_completed_episode
);

LoadedCheckpoint load_checkpoint(
    const std::filesystem::path& checkpoint_dir
);

}  // namespace rl