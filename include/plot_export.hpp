#pragma once

#include <filesystem>

#include "gridworld.hpp"
#include "mc_agent.hpp"

namespace rl {

void export_gridworld_plot_data(
    const std::filesystem::path& export_dir,
    const GridWorld& env,
    const MonteCarloOffPolicyAgent& agent,
    int last_completed_episode
);

} // namespace rl