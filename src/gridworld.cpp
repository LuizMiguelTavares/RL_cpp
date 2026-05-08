#include "gridworld.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <vector>

namespace rl {

namespace {

inline Index flat_index(int row, int col, int width) {
    return static_cast<Index>(row) * static_cast<Index>(width) + static_cast<Index>(col);
}

}  // namespace

GridWorld::GridWorld(
    int width,
    int height,
    double obstacle_density,
    const std::string& obstacle_mode,
    unsigned int seed,
    int cluster_size,
    State start,
    State goal,
    bool allow_diagonal,
    double diagonal_cost,
    double reward_goal,
    double reward_obstacle,
    double reward_step,
    const std::string& shaping,
    int max_steps,
    bool ensure_solvable,
    int obstacle_generation_tries
)
    : width_(width),
      height_(height),
      obstacle_density_(obstacle_density),
      obstacle_mode_(obstacle_mode),
      cluster_size_(cluster_size),
      seed_(seed),
      rng_(seed),
      allow_diagonal_(allow_diagonal),
      diagonal_cost_(diagonal_cost),
      reward_goal_(reward_goal),
      reward_obstacle_(reward_obstacle),
      reward_step_(reward_step),
      shaping_(shaping),
      max_steps_(max_steps),
      ensure_solvable_(ensure_solvable),
      obstacle_generation_tries_(obstacle_generation_tries),
      start_(start),
      goal_(goal),
      agent_pos_(start) {
    initialize_goal_if_needed();
    validate_config();
    initialize_actions();

    if (diagonal_cost_ < 0.0) {
        diagonal_cost_ = allow_diagonal_ ? std::sqrt(2.0) : 1.0;
    }

    grid_.assign(static_cast<Index>(width_ * height_), static_cast<std::uint8_t>(0));
    build_valid_grid();
    initialize_potential_map();
}

State GridWorld::reset() {
    agent_pos_ = start_;
    steps_taken_ = 0;
    return agent_pos_;
}

StepResult GridWorld::step(Action action) {
    if (action < 0 || action >= num_actions()) {
        throw std::out_of_range("GridWorld::step: invalid action index");
    }

    ++steps_taken_;

    const State current = agent_pos_;
    const State delta = action_delta(action);
    const State candidate{current.row + delta.row, current.col + delta.col};

    StepResult result{};
    result.next_state = current;
    result.reward = 0.0;
    result.terminated = false;
    result.truncated = false;
    result.invalid_move = false;

    if (!in_bounds(candidate) || is_obstacle(candidate)) {
        result.next_state = current;
        result.reward = reward_obstacle_;
        result.invalid_move = true;
    } else {
        result.next_state = candidate;
        agent_pos_ = candidate;

        if (candidate == goal_) {
            result.reward = reward_goal_;
            result.terminated = true;
        } else {
            const bool diagonal_move = (delta.row != 0 && delta.col != 0);
            if (allow_diagonal_ && diagonal_move && reward_step_ < 0.0) {
                result.reward = -std::abs(diagonal_cost_);
            } else {
                result.reward = reward_step_;
            }
        }
    }

    if (shaping_ != "none") {
        result.reward += potential(current) - potential(result.next_state);
    }

    if (!result.terminated && steps_taken_ >= max_steps_) {
        result.truncated = true;
    }

    return result;
}

int GridWorld::width() const noexcept {
    return width_;
}

int GridWorld::height() const noexcept {
    return height_;
}

int GridWorld::num_actions() const noexcept {
    return static_cast<int>(actions_.size());
}

int GridWorld::max_steps() const noexcept {
    return max_steps_;
}

const State& GridWorld::start() const noexcept {
    return start_;
}

const State& GridWorld::goal() const noexcept {
    return goal_;
}

const State& GridWorld::agent_pos() const noexcept {
    return agent_pos_;
}

bool GridWorld::allow_diagonal() const noexcept {
    return allow_diagonal_;
}

bool GridWorld::is_terminal_state(const State& state) const noexcept {
    return state == goal_;
}

bool GridWorld::in_bounds(const State& state) const noexcept {
    return state.row >= 0 && state.row < height_ && state.col >= 0 && state.col < width_;
}

bool GridWorld::is_obstacle(const State& state) const noexcept {
    return grid_[flat_index(state.row, state.col, width_)] == 1;
}

bool GridWorld::is_free(const State& state) const noexcept {
    return in_bounds(state) && !is_obstacle(state);
}

State GridWorld::action_delta(Action action) const {
    if (action < 0 || action >= num_actions()) {
        throw std::out_of_range("GridWorld::action_delta: invalid action index");
    }
    return actions_[static_cast<Index>(action)];
}

std::vector<Action> GridWorld::valid_actions(const State& state) const {
    std::vector<Action> valid;
    valid.reserve(actions_.size());

    for (Action a = 0; a < num_actions(); ++a) {
        const State d = action_delta(a);
        const State nxt{state.row + d.row, state.col + d.col};
        if (in_bounds(nxt) && !is_obstacle(nxt)) {
            valid.push_back(a);
        }
    }

    return valid;
}

Index GridWorld::state_index(const State& state) const noexcept {
    return flat_index(state.row, state.col, width_);
}

State GridWorld::index_to_state(Index index) const {
    const Index total = static_cast<Index>(width_ * height_);
    if (index >= total) {
        throw std::out_of_range("GridWorld::index_to_state: index out of range");
    }

    const int row = static_cast<int>(index / static_cast<Index>(width_));
    const int col = static_cast<int>(index % static_cast<Index>(width_));
    return State{row, col};
}

std::uint8_t GridWorld::cell(int row, int col) const {
    if (row < 0 || row >= height_ || col < 0 || col >= width_) {
        throw std::out_of_range("GridWorld::cell: coordinates out of range");
    }
    return grid_[flat_index(row, col, width_)];
}

const std::vector<std::uint8_t>& GridWorld::grid_data() const noexcept {
    return grid_;
}

double GridWorld::potential(const State& state) const {
    if (!in_bounds(state)) {
        throw std::out_of_range("GridWorld::potential: state out of bounds");
    }
    return potential_map_[flat_index(state.row, state.col, width_)];
}

double GridWorld::distance_to_goal(const State& state) const {
    if (shaping_ == "manhattan") {
        return static_cast<double>(std::abs(state.row - goal_.row) + std::abs(state.col - goal_.col));
    }
    return std::hypot(static_cast<double>(state.row - goal_.row),
                      static_cast<double>(state.col - goal_.col));
}

void GridWorld::render_ascii() const {
    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            const State s{r, c};

            char ch = '.';
            if (is_obstacle(s)) {
                ch = '#';
            }
            if (s == start_) {
                ch = 'S';
            }
            if (s == goal_) {
                ch = 'G';
            }
            if (s == agent_pos_ && s != start_ && s != goal_) {
                ch = 'A';
            }

            std::cout << ch << ' ';
        }
        std::cout << '\n';
    }
    std::cout << std::endl;
}

void GridWorld::set_grid(const std::vector<std::uint8_t>& new_grid) {
    if (new_grid.size() != static_cast<Index>(width_ * height_)) {
        throw std::runtime_error("GridWorld::set_grid: wrong grid size");
    }

    grid_ = new_grid;

    grid_[flat_index(start_.row, start_.col, width_)] = 0;
    grid_[flat_index(goal_.row, goal_.col, width_)] = 0;

    if (ensure_solvable_ && !has_path(start_, goal_)) {
        throw std::runtime_error("GridWorld::set_grid: provided grid is not solvable");
    }
}

void GridWorld::validate_config() const {
    if (width_ <= 0 || height_ <= 0) {
        throw std::runtime_error("GridWorld: width and height must be positive");
    }

    if (obstacle_density_ < 0.0 || obstacle_density_ >= 1.0) {
        throw std::runtime_error("GridWorld: obstacle_density must be in [0, 1)");
    }

    if (obstacle_mode_ != "random" && obstacle_mode_ != "cluster") {
        throw std::runtime_error("GridWorld: obstacle_mode must be 'random' or 'cluster'");
    }

    if (shaping_ != "none" && shaping_ != "manhattan" && shaping_ != "euclidean") {
        throw std::runtime_error("GridWorld: shaping must be 'none', 'manhattan', or 'euclidean'");
    }

    if (max_steps_ <= 0) {
        throw std::runtime_error("GridWorld: max_steps must be positive");
    }

    if (!in_bounds(start_)) {
        throw std::runtime_error("GridWorld: start out of bounds");
    }

    if (!in_bounds(goal_)) {
        throw std::runtime_error("GridWorld: goal out of bounds");
    }

    if (start_ == goal_) {
        throw std::runtime_error("GridWorld: start and goal cannot be equal");
    }
}

void GridWorld::initialize_goal_if_needed() {
    if (goal_.row == -1 && goal_.col == -1) {
        goal_ = State{height_ - 1, width_ - 1};
    }
}

void GridWorld::initialize_actions() {
    actions_.clear();

    if (allow_diagonal_) {
        actions_ = {
            State{-1, 0},
            State{ 1, 0},
            State{ 0,-1},
            State{ 0, 1},
            State{-1,-1},
            State{-1, 1},
            State{ 1,-1},
            State{ 1, 1}
        };
    } else {
        actions_ = {
            State{-1, 0},
            State{ 1, 0},
            State{ 0,-1},
            State{ 0, 1}
        };
    }
}

void GridWorld::initialize_potential_map() {
    potential_map_.assign(static_cast<Index>(width_ * height_), 0.0);

    if (shaping_ == "none") {
        return;
    }

    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            const State s{r, c};
            potential_map_[flat_index(r, c, width_)] = distance_to_goal(s);
        }
    }
}

void GridWorld::build_valid_grid() {
    for (int attempt = 0; attempt < obstacle_generation_tries_; ++attempt) {
        std::fill(grid_.begin(), grid_.end(), static_cast<std::uint8_t>(0));

        populate_obstacles();

        grid_[flat_index(start_.row, start_.col, width_)] = 0;
        grid_[flat_index(goal_.row, goal_.col, width_)] = 0;

        if (!ensure_solvable_ || has_path(start_, goal_)) {
            return;
        }
    }

    throw std::runtime_error("GridWorld: failed to generate a solvable grid");
}

void GridWorld::populate_obstacles() {
    const int total_cells = width_ * height_;
    const int n_obs = static_cast<int>(std::floor(obstacle_density_ * static_cast<double>(total_cells)));

    if (n_obs <= 0) {
        return;
    }

    if (obstacle_mode_ == "random") {
        std::vector<Index> candidates;
        candidates.reserve(static_cast<Index>(total_cells));

        for (int r = 0; r < height_; ++r) {
            for (int c = 0; c < width_; ++c) {
                const State s{r, c};
                if (s != start_ && s != goal_) {
                    candidates.push_back(flat_index(r, c, width_));
                }
            }
        }

        std::shuffle(candidates.begin(), candidates.end(), rng_);

        const int to_place = std::min<int>(n_obs, static_cast<int>(candidates.size()));
        for (int i = 0; i < to_place; ++i) {
            grid_[candidates[static_cast<Index>(i)]] = 1;
        }

        return;
    }

    std::uniform_int_distribution<int> row_dist(0, height_ - 1);
    std::uniform_int_distribution<int> col_dist(0, width_ - 1);
    std::uniform_int_distribution<int> offset_dist(-cluster_size_, cluster_size_);

    int placed = 0;
    while (placed < n_obs) {
        const int center_r = row_dist(rng_);
        const int center_c = col_dist(rng_);

        for (int i = 0; i < cluster_size_ && placed < n_obs; ++i) {
            const int r = center_r + offset_dist(rng_);
            const int c = center_c + offset_dist(rng_);

            const State s{r, c};
            if (!in_bounds(s) || s == start_ || s == goal_) {
                continue;
            }

            const Index idx = flat_index(r, c, width_);
            if (grid_[idx] == 0) {
                grid_[idx] = 1;
                ++placed;
            }
        }
    }
}

bool GridWorld::has_path(const State& from, const State& to) const {
    if (from == to) {
        return true;
    }

    std::vector<std::uint8_t> visited(static_cast<Index>(width_ * height_), 0);
    std::queue<State> q;

    q.push(from);
    visited[state_index(from)] = 1;

    while (!q.empty()) {
        const State current = q.front();
        q.pop();

        for (const State& nxt : neighbors_for_path_check(current)) {
            const Index idx = state_index(nxt);
            if (visited[idx]) {
                continue;
            }

            if (nxt == to) {
                return true;
            }

            visited[idx] = 1;
            q.push(nxt);
        }
    }

    return false;
}

std::vector<State> GridWorld::neighbors_for_path_check(const State& state) const {
    std::vector<State> neighbors;
    neighbors.reserve(actions_.size());

    for (const State& d : actions_) {
        const State nxt{state.row + d.row, state.col + d.col};
        if (in_bounds(nxt) && !is_obstacle(nxt)) {
            neighbors.push_back(nxt);
        }
    }

    return neighbors;
}

double GridWorld::diagonal_cost() const noexcept {
    return diagonal_cost_;
}

double GridWorld::reward_goal() const noexcept {
    return reward_goal_;
}

double GridWorld::reward_obstacle() const noexcept {
    return reward_obstacle_;
}

double GridWorld::reward_step() const noexcept {
    return reward_step_;
}

const std::string& GridWorld::shaping() const noexcept {
    return shaping_;
}

}  // namespace rl