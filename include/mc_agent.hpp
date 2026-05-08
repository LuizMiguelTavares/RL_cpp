#pragma once

#include <random>
#include <string>
#include <vector>

#include "episode.hpp"
#include "gridworld.hpp"
#include "types.hpp"

namespace rl {

struct UpdateStats {
    int updates_applied{0};
    bool break_happened{false};
};

class MonteCarloOffPolicyAgent {
public:
    MonteCarloOffPolicyAgent(
        const GridWorld& env,
        double gamma = 0.99,
        double epsilon_behavior = 0.2,
        const std::string& visit_mode = "first_visit",   // "first_visit" | "every_visit"
        unsigned int seed = 0
    );

    // --------------------------------------------------
    // Basic getters
    // --------------------------------------------------
    [[nodiscard]] double gamma() const noexcept;
    [[nodiscard]] double epsilon_behavior() const noexcept;
    [[nodiscard]] const std::string& visit_mode() const noexcept;

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] int num_actions() const noexcept;
    [[nodiscard]] unsigned int seed() const noexcept;

    // --------------------------------------------------
    // Policy methods
    // --------------------------------------------------
    [[nodiscard]] Action greedy_action(const State& state) const;
    [[nodiscard]] Action behavior_action(const State& state);

    [[nodiscard]] double behavior_prob(const State& state, Action action) const;
    [[nodiscard]] double target_prob(const State& state, Action action) const;

    // --------------------------------------------------
    // Learning
    // --------------------------------------------------
    [[nodiscard]] bool should_update_visit(const Episode& episode, std::size_t t) const;
    [[nodiscard]] UpdateStats update_from_episode(const Episode& episode);

    [[nodiscard]] std::vector<State> greedy_path(GridWorld& env, int max_steps = 500) const;

    // --------------------------------------------------
    // Table access
    // --------------------------------------------------
    [[nodiscard]] double q_value(const State& state, Action action) const;
    [[nodiscard]] double c_value(const State& state, Action action) const;
    [[nodiscard]] double tie_noise_value(const State& state, Action action) const;

    void set_q_value(const State& state, Action action, double value);
    void set_c_value(const State& state, Action action, double value);

    [[nodiscard]] const std::vector<double>& q_data() const noexcept;
    [[nodiscard]] const std::vector<double>& c_data() const noexcept;
    [[nodiscard]] const std::vector<double>& tie_noise_data() const noexcept;

    void set_q_data(const std::vector<double>& q_values);
    void set_c_data(const std::vector<double>& c_values);
    void set_tie_noise_data(const std::vector<double>& noise_values);

    [[nodiscard]] std::string rng_state_string() const;
    void set_rng_state_string(const std::string& state_str);

private:
    // --------------------------------------------------
    // Internal helpers
    // --------------------------------------------------
    [[nodiscard]] Index state_action_index(const State& state, Action action) const noexcept;
    [[nodiscard]] Action greedy_action_from_index(Index state_idx) const noexcept;

private:
    // --------------------------------------------------
    // Static environment dimensions
    // --------------------------------------------------
    int width_{0};
    int height_{0};
    int num_actions_{0};

    // --------------------------------------------------
    // Hyperparameters / config
    // --------------------------------------------------
    double gamma_{0.99};
    double epsilon_behavior_{0.2};
    std::string visit_mode_{"first_visit"};
    unsigned int seed_{0};

    // --------------------------------------------------
    // Random generator
    // --------------------------------------------------
    mutable std::mt19937 rng_;

    // --------------------------------------------------
    // Tables (linear storage)
    // index = ((row * width + col) * num_actions + action)
    // --------------------------------------------------
    std::vector<double> Q_;
    std::vector<double> C_;
    std::vector<double> tie_noise_;
};

}  // namespace rl