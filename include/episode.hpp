#pragma once

#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "types.hpp"

namespace rl {

struct Episode {
    std::vector<int> rows;
    std::vector<int> cols;
    std::vector<Action> actions;
    std::vector<double> rewards;

    bool terminated{false};
    bool truncated{false};

    void clear() {
        rows.clear();
        cols.clear();
        actions.clear();
        rewards.clear();
        terminated = false;
        truncated = false;
    }

    void reserve(std::size_t n_steps) {
        rows.reserve(n_steps);
        cols.reserve(n_steps);
        actions.reserve(n_steps);
        rewards.reserve(n_steps);
    }

    void add_transition(const State& state, Action action, double reward) {
        rows.push_back(state.row);
        cols.push_back(state.col);
        actions.push_back(action);
        rewards.push_back(reward);
    }

    [[nodiscard]] std::size_t length() const noexcept {
        return actions.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return actions.empty();
    }

    [[nodiscard]] double total_reward() const {
        return std::accumulate(rewards.begin(), rewards.end(), 0.0);
    }

    [[nodiscard]] State state_at(std::size_t t) const {
        if (t >= length()) {
            throw std::out_of_range("Episode::state_at: index out of range");
        }
        return State{rows[t], cols[t]};
    }

    void validate() const {
        const std::size_t n = actions.size();

        if (rows.size() != n || cols.size() != n || rewards.size() != n) {
            throw std::runtime_error(
                "Episode::validate: rows, cols, actions, and rewards must have the same size"
            );
        }

        if (terminated && truncated) {
            throw std::runtime_error(
                "Episode::validate: episode cannot be both terminated and truncated"
            );
        }
    }
};

}  // namespace rl