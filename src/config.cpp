#include "config.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace rl {

using json = nlohmann::json;

RunConfig load_config(const std::filesystem::path& config_path) {
    std::ifstream in(config_path);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_path.string());
    }

    json j;
    in >> j;

    RunConfig cfg;

    cfg.run_name = j.value("run_name", cfg.run_name);
    cfg.resume = j.value("resume", cfg.resume);

    if (j.contains("environment")) {
        const auto& e = j.at("environment");

        cfg.environment.width = e.value("width", cfg.environment.width);
        cfg.environment.height = e.value("height", cfg.environment.height);
        cfg.environment.obstacle_density = e.value("obstacle_density", cfg.environment.obstacle_density);
        cfg.environment.obstacle_mode = e.value("obstacle_mode", cfg.environment.obstacle_mode);
        cfg.environment.seed = e.value("seed", cfg.environment.seed);
        cfg.environment.cluster_size = e.value("cluster_size", cfg.environment.cluster_size);

        cfg.environment.start_row = e.value("start_row", cfg.environment.start_row);
        cfg.environment.start_col = e.value("start_col", cfg.environment.start_col);
        cfg.environment.goal_row = e.value("goal_row", cfg.environment.goal_row);
        cfg.environment.goal_col = e.value("goal_col", cfg.environment.goal_col);

        cfg.environment.allow_diagonal = e.value("allow_diagonal", cfg.environment.allow_diagonal);
        cfg.environment.diagonal_cost = e.value("diagonal_cost", cfg.environment.diagonal_cost);

        cfg.environment.reward_goal = e.value("reward_goal", cfg.environment.reward_goal);
        cfg.environment.reward_obstacle = e.value("reward_obstacle", cfg.environment.reward_obstacle);
        cfg.environment.reward_step = e.value("reward_step", cfg.environment.reward_step);

        cfg.environment.shaping = e.value("shaping", cfg.environment.shaping);
        cfg.environment.ensure_solvable = e.value("ensure_solvable", cfg.environment.ensure_solvable);
        cfg.environment.obstacle_generation_tries =
            e.value("obstacle_generation_tries", cfg.environment.obstacle_generation_tries);
    }

    if (j.contains("agent")) {
        const auto& a = j.at("agent");

        cfg.agent.gamma = a.value("gamma", cfg.agent.gamma);
        cfg.agent.epsilon_behavior = a.value("epsilon_behavior", cfg.agent.epsilon_behavior);
        cfg.agent.visit_mode = a.value("visit_mode", cfg.agent.visit_mode);
        cfg.agent.seed = a.value("seed", cfg.agent.seed);
    }

    if (j.contains("training")) {
        const auto& t = j.at("training");

        cfg.training.episodes_this_run = t.value("episodes_this_run", cfg.training.episodes_this_run);
        cfg.training.max_steps = t.value("max_steps", cfg.training.max_steps);
        cfg.training.print_every = t.value("print_every", cfg.training.print_every);
        cfg.training.flush_every = t.value("flush_every", cfg.training.flush_every);
        cfg.training.checkpoint_every = t.value("checkpoint_every", cfg.training.checkpoint_every);
    }

    return cfg;
}

}  // namespace rl