#include "plot_export.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace rl {
namespace {

std::string action_name_from_delta(const State& delta) {
    if (delta.row == -1 && delta.col == 0) {
        return "up";
    }
    if (delta.row == 1 && delta.col == 0) {
        return "down";
    }
    if (delta.row == 0 && delta.col == -1) {
        return "left";
    }
    if (delta.row == 0 && delta.col == 1) {
        return "right";
    }
    if (delta.row == -1 && delta.col == -1) {
        return "up_left";
    }
    if (delta.row == -1 && delta.col == 1) {
        return "up_right";
    }
    if (delta.row == 1 && delta.col == -1) {
        return "down_left";
    }
    if (delta.row == 1 && delta.col == 1) {
        return "down_right";
    }

    return "unknown";
}

double state_value(const GridWorld& env, const MonteCarloOffPolicyAgent& agent, const State& state) {
    double best = -std::numeric_limits<double>::infinity();

    for (Action a = 0; a < env.num_actions(); ++a) {
        best = std::max(best, agent.q_value(state, a));
    }

    return best;
}

double state_confidence(const GridWorld& env, const MonteCarloOffPolicyAgent& agent, const State& state) {
    if (env.num_actions() < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double best = -std::numeric_limits<double>::infinity();
    double second_best = -std::numeric_limits<double>::infinity();

    for (Action a = 0; a < env.num_actions(); ++a) {
        const double q = agent.q_value(state, a);

        if (q > best) {
            second_best = best;
            best = q;
        } else if (q > second_best) {
            second_best = q;
        }
    }

    return best - second_best;
}

void export_grid_map_json(
    const std::filesystem::path& path,
    const GridWorld& env,
    int last_completed_episode
) {
    nlohmann::json j;

    j["width"] = env.width();
    j["height"] = env.height();
    j["num_actions"] = env.num_actions();
    j["last_completed_episode"] = last_completed_episode;

    j["start"] = {
        {"row", env.start().row},
        {"col", env.start().col}
    };

    j["goal"] = {
        {"row", env.goal().row},
        {"col", env.goal().col}
    };

    j["allow_diagonal"] = env.allow_diagonal();
    j["max_steps"] = env.max_steps();

    j["rewards"] = {
        {"goal", env.reward_goal()},
        {"obstacle", env.reward_obstacle()},
        {"step", env.reward_step()}
    };

    j["shaping"] = env.shaping();

    j["actions"] = nlohmann::json::array();

    for (Action a = 0; a < env.num_actions(); ++a) {
        const State delta = env.action_delta(a);

        j["actions"].push_back({
            {"index", a},
            {"name", action_name_from_delta(delta)},
            {"delta_row", delta.row},
            {"delta_col", delta.col}
        });
    }

    j["obstacles"] = nlohmann::json::array();

    for (int r = 0; r < env.height(); ++r) {
        for (int c = 0; c < env.width(); ++c) {
            if (env.is_obstacle(State{r, c})) {
                j["obstacles"].push_back({
                    {"row", r},
                    {"col", c}
                });
            }
        }
    }

    std::ofstream out(path);

    if (!out) {
        throw std::runtime_error("Failed to open grid map export file: " + path.string());
    }

    out << std::setw(2) << j << '\n';
}

void export_q_table_csv(
    const std::filesystem::path& path,
    const GridWorld& env,
    const MonteCarloOffPolicyAgent& agent
) {
    std::ofstream out(path);

    if (!out) {
        throw std::runtime_error("Failed to open Q-table export file: " + path.string());
    }

    out << std::setprecision(17);

    out
        << "row,col,"
        << "is_obstacle,is_start,is_goal,"
        << "value,confidence,"
        << "greedy_action,greedy_delta_row,greedy_delta_col";

    for (Action a = 0; a < env.num_actions(); ++a) {
        out << ",q_a" << a;
    }

    for (Action a = 0; a < env.num_actions(); ++a) {
        out << ",c_a" << a;
    }

    out << '\n';

    for (int r = 0; r < env.height(); ++r) {
        for (int c = 0; c < env.width(); ++c) {
            const State s{r, c};

            const bool is_obstacle = env.is_obstacle(s);
            const bool is_start = (s == env.start());
            const bool is_goal = (s == env.goal());

            double value = std::numeric_limits<double>::quiet_NaN();
            double confidence = std::numeric_limits<double>::quiet_NaN();
            Action greedy_action = -1;
            State greedy_delta{0, 0};

            if (!is_obstacle) {
                value = state_value(env, agent, s);
                confidence = state_confidence(env, agent, s);
                greedy_action = agent.greedy_action(s);
                greedy_delta = env.action_delta(greedy_action);
            }

            out
                << r << ','
                << c << ','
                << static_cast<int>(is_obstacle) << ','
                << static_cast<int>(is_start) << ','
                << static_cast<int>(is_goal) << ','
                << value << ','
                << confidence << ','
                << greedy_action << ','
                << greedy_delta.row << ','
                << greedy_delta.col;

            for (Action a = 0; a < env.num_actions(); ++a) {
                out << ',' << agent.q_value(s, a);
            }

            for (Action a = 0; a < env.num_actions(); ++a) {
                out << ',' << agent.c_value(s, a);
            }

            out << '\n';
        }
    }
}

} // namespace

void export_gridworld_plot_data(
    const std::filesystem::path& export_dir,
    const GridWorld& env,
    const MonteCarloOffPolicyAgent& agent,
    int last_completed_episode
) {
    std::filesystem::create_directories(export_dir);

    export_grid_map_json(
        export_dir / "grid_map.json",
        env,
        last_completed_episode
    );

    export_q_table_csv(
        export_dir / "q_table.csv",
        env,
        agent
    );
}

} // namespace rl