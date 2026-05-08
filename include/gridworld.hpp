#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "types.hpp"

namespace rl {

class GridWorld {
public:
    GridWorld(
        int width,
        int height,
        double obstacle_density = 0.2,
        const std::string& obstacle_mode = "cluster",   // "random" | "cluster"
        unsigned int seed = 0,
        int cluster_size = 5,
        State start = State{0, 0},
        State goal = State{-1, -1},                     // if {-1,-1}, use bottom-right
        bool allow_diagonal = false,
        double diagonal_cost = -1.0,                   // if < 0, auto-choose
        double reward_goal = 100.0,
        double reward_obstacle = -10.0,
        double reward_step = -0.001,
        const std::string& shaping = "euclidean",      // "none" | "manhattan" | "euclidean"
        int max_steps = 500,
        bool ensure_solvable = true,
        int obstacle_generation_tries = 100
    );

    // --------------------------------------------------
    // Core environment API
    // --------------------------------------------------
    State reset();
    StepResult step(Action action);

    // --------------------------------------------------
    // Basic getters
    // --------------------------------------------------
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] int num_actions() const noexcept;
    [[nodiscard]] int max_steps() const noexcept;

    [[nodiscard]] const State& start() const noexcept;
    [[nodiscard]] const State& goal() const noexcept;
    [[nodiscard]] const State& agent_pos() const noexcept;

    [[nodiscard]] bool allow_diagonal() const noexcept;
    [[nodiscard]] bool is_terminal_state(const State& state) const noexcept;

    // --------------------------------------------------
    // State/action helpers
    // --------------------------------------------------
    [[nodiscard]] bool in_bounds(const State& state) const noexcept;
    [[nodiscard]] bool is_obstacle(const State& state) const noexcept;
    [[nodiscard]] bool is_free(const State& state) const noexcept;

    [[nodiscard]] State action_delta(Action action) const;
    [[nodiscard]] std::vector<Action> valid_actions(const State& state) const;

    [[nodiscard]] Index state_index(const State& state) const noexcept;
    [[nodiscard]] State index_to_state(Index index) const;

    // --------------------------------------------------
    // Grid access
    // --------------------------------------------------
    [[nodiscard]] std::uint8_t cell(int row, int col) const;
    [[nodiscard]] const std::vector<std::uint8_t>& grid_data() const noexcept;

    // --------------------------------------------------
    // Distances / shaping
    // --------------------------------------------------
    [[nodiscard]] double potential(const State& state) const;
    [[nodiscard]] double distance_to_goal(const State& state) const;

    // --------------------------------------------------
    // Debug / utility
    // --------------------------------------------------
    void render_ascii() const;
    void set_grid(const std::vector<std::uint8_t>& new_grid);


    [[nodiscard]] double diagonal_cost() const noexcept;
    [[nodiscard]] double reward_goal() const noexcept;
    [[nodiscard]] double reward_obstacle() const noexcept;
    [[nodiscard]] double reward_step() const noexcept;
    [[nodiscard]] const std::string& shaping() const noexcept;

private:
    // --------------------------------------------------
    // Internal helpers
    // --------------------------------------------------
    void validate_config() const;
    void initialize_goal_if_needed();
    void initialize_actions();
    void initialize_potential_map();

    void build_valid_grid();
    void populate_obstacles();
    bool has_path(const State& from, const State& to) const;

    [[nodiscard]] std::vector<State> neighbors_for_path_check(const State& state) const;

private:
    // --------------------------------------------------
    // Configuration
    // --------------------------------------------------
    int width_{0};
    int height_{0};

    double obstacle_density_{0.2};
    std::string obstacle_mode_{"cluster"};
    int cluster_size_{5};

    unsigned int seed_{0};
    std::mt19937 rng_;

    bool allow_diagonal_{false};
    double diagonal_cost_{1.0};

    double reward_goal_{100.0};
    double reward_obstacle_{-10.0};
    double reward_step_{-0.001};

    std::string shaping_{"euclidean"};

    int max_steps_{500};
    bool ensure_solvable_{true};
    int obstacle_generation_tries_{100};

    // --------------------------------------------------
    // Dynamic state
    // --------------------------------------------------
    State start_{0, 0};
    State goal_{0, 0};
    State agent_pos_{0, 0};

    int steps_taken_{0};

    // --------------------------------------------------
    // Environment data
    // --------------------------------------------------
    std::vector<std::uint8_t> grid_;   // 0 free, 1 obstacle
    std::vector<State> actions_;       // deltas
    std::vector<double> potential_map_;
};

}  // namespace rl