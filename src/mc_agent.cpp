#include "mc_agent.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <sstream>

namespace rl {

MonteCarloOffPolicyAgent::MonteCarloOffPolicyAgent(
    const GridWorld& env,
    double gamma,
    double epsilon_behavior,
    const std::string& visit_mode,
    unsigned int seed
)
    : width_(env.width()),
      height_(env.height()),
      num_actions_(env.num_actions()),
      gamma_(gamma),
      epsilon_behavior_(epsilon_behavior),
      visit_mode_(visit_mode),
      seed_(seed),
      rng_(seed) {
    if (width_ <= 0 || height_ <= 0 || num_actions_ <= 0) {
        throw std::runtime_error("MonteCarloOffPolicyAgent: invalid environment dimensions");
    }

    if (gamma_ < 0.0 || gamma_ > 1.0) {
        throw std::runtime_error("MonteCarloOffPolicyAgent: gamma must be in [0, 1]");
    }

    if (epsilon_behavior_ < 0.0 || epsilon_behavior_ > 1.0) {
        throw std::runtime_error("MonteCarloOffPolicyAgent: epsilon_behavior must be in [0, 1]");
    }

    if (visit_mode_ != "first_visit" && visit_mode_ != "every_visit") {
        throw std::runtime_error("MonteCarloOffPolicyAgent: visit_mode must be 'first_visit' or 'every_visit'");
    }

    const Index total_sa =
        static_cast<Index>(width_) *
        static_cast<Index>(height_) *
        static_cast<Index>(num_actions_);

    Q_.assign(total_sa, 0.0);
    C_.assign(total_sa, 0.0);
    tie_noise_.assign(total_sa, 0.0);

    // Pequeno ruído fixo para desempate estável e não enviesado para ação 0
    std::normal_distribution<double> noise_dist(0.0, 1e-12);
    for (auto& v : tie_noise_) {
        v = noise_dist(rng_);
    }
}

// --------------------------------------------------
// Basic getters
// --------------------------------------------------

double MonteCarloOffPolicyAgent::gamma() const noexcept {
    return gamma_;
}

double MonteCarloOffPolicyAgent::epsilon_behavior() const noexcept {
    return epsilon_behavior_;
}

const std::string& MonteCarloOffPolicyAgent::visit_mode() const noexcept {
    return visit_mode_;
}

int MonteCarloOffPolicyAgent::width() const noexcept {
    return width_;
}

int MonteCarloOffPolicyAgent::height() const noexcept {
    return height_;
}

int MonteCarloOffPolicyAgent::num_actions() const noexcept {
    return num_actions_;
}

unsigned int MonteCarloOffPolicyAgent::seed() const noexcept {
    return seed_;
}

// --------------------------------------------------
// Internal helpers
// --------------------------------------------------

Index MonteCarloOffPolicyAgent::state_action_index(const State& state, Action action) const noexcept {
    return (
        (static_cast<Index>(state.row) * static_cast<Index>(width_) + static_cast<Index>(state.col))
        * static_cast<Index>(num_actions_)
        + static_cast<Index>(action)
    );
}

Action MonteCarloOffPolicyAgent::greedy_action_from_index(Index state_idx) const noexcept {
    const Index base = state_idx * static_cast<Index>(num_actions_);

    Action best_a = 0;
    double best_score = Q_[base] + tie_noise_[base];

    for (Action a = 1; a < num_actions_; ++a) {
        const double score = Q_[base + static_cast<Index>(a)] + tie_noise_[base + static_cast<Index>(a)];
        if (score > best_score) {
            best_score = score;
            best_a = a;
        }
    }

    return best_a;
}

// --------------------------------------------------
// Policy methods
// --------------------------------------------------

Action MonteCarloOffPolicyAgent::greedy_action(const State& state) const {
    if (state.row < 0 || state.row >= height_ || state.col < 0 || state.col >= width_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::greedy_action: state out of bounds");
    }

    const Index state_idx =
        static_cast<Index>(state.row) * static_cast<Index>(width_) + static_cast<Index>(state.col);

    return greedy_action_from_index(state_idx);
}

Action MonteCarloOffPolicyAgent::behavior_action(const State& state) {
    std::uniform_real_distribution<double> u01(0.0, 1.0);

    if (u01(rng_) < epsilon_behavior_) {
        std::uniform_int_distribution<int> action_dist(0, num_actions_ - 1);
        return action_dist(rng_);
    }

    return greedy_action(state);
}

double MonteCarloOffPolicyAgent::behavior_prob(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::behavior_prob: action out of range");
    }

    const Action greedy = greedy_action(state);

    if (action == greedy) {
        return (1.0 - epsilon_behavior_) + (epsilon_behavior_ / static_cast<double>(num_actions_));
    }

    return epsilon_behavior_ / static_cast<double>(num_actions_);
}

double MonteCarloOffPolicyAgent::target_prob(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::target_prob: action out of range");
    }

    return (action == greedy_action(state)) ? 1.0 : 0.0;
}

// --------------------------------------------------
// Learning helpers
// --------------------------------------------------

bool MonteCarloOffPolicyAgent::should_update_visit(const Episode& episode, std::size_t t) const {
    if (visit_mode_ == "every_visit") {
        return true;
    }

    const int row_t = episode.rows[t];
    const int col_t = episode.cols[t];
    const Action action_t = episode.actions[t];

    for (std::size_t k = 0; k < t; ++k) {
        if (
            episode.rows[k] == row_t &&
            episode.cols[k] == col_t &&
            episode.actions[k] == action_t
        ) {
            return false;
        }
    }

    return true;
}

// --------------------------------------------------
// Main MC update
// --------------------------------------------------

UpdateStats MonteCarloOffPolicyAgent::update_from_episode(const Episode& episode) {
    episode.validate();

    UpdateStats stats{};

    if (episode.empty()) {
        return stats;
    }

    double G = 0.0;
    double W = 1.0;

    for (std::size_t t_rev = 0; t_rev < episode.length(); ++t_rev) {
        const std::size_t t = episode.length() - 1 - t_rev;

        const State state_t{episode.rows[t], episode.cols[t]};
        const Action action_t = episode.actions[t];
        const double reward_t = episode.rewards[t];

        G = gamma_ * G + reward_t;

        if (!should_update_visit(episode, t)) {
            continue;
        }

        const Index idx = state_action_index(state_t, action_t);

        C_[idx] += W;
        Q_[idx] += (W / C_[idx]) * (G - Q_[idx]);
        ++stats.updates_applied;

        const Action greedy = greedy_action(state_t);

        if (action_t != greedy) {
            stats.break_happened = true;
            break;
        }

        const double bprob = behavior_prob(state_t, action_t);
        if (bprob <= 0.0) {
            throw std::runtime_error("MonteCarloOffPolicyAgent::update_from_episode: behavior probability <= 0");
        }

        W *= 1.0 / bprob;
    }

    return stats;
}

// --------------------------------------------------
// Greedy rollout
// --------------------------------------------------

std::vector<State> MonteCarloOffPolicyAgent::greedy_path(GridWorld& env, int max_steps) const {
    if (max_steps <= 0) {
        throw std::runtime_error("MonteCarloOffPolicyAgent::greedy_path: max_steps must be positive");
    }

    std::vector<State> path;
    path.reserve(static_cast<std::size_t>(max_steps + 1));

    State state = env.reset();
    path.push_back(state);

    for (int step = 0; step < max_steps; ++step) {
        const Action action = greedy_action(state);
        const StepResult result = env.step(action);

        path.push_back(result.next_state);
        state = result.next_state;

        if (result.terminated || result.truncated) {
            break;
        }
    }

    return path;
}

// --------------------------------------------------
// Table access
// --------------------------------------------------

double MonteCarloOffPolicyAgent::q_value(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::q_value: action out of range");
    }
    return Q_[state_action_index(state, action)];
}

double MonteCarloOffPolicyAgent::c_value(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::c_value: action out of range");
    }
    return C_[state_action_index(state, action)];
}

double MonteCarloOffPolicyAgent::tie_noise_value(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::tie_noise_value: action out of range");
    }
    return tie_noise_[state_action_index(state, action)];
}

void MonteCarloOffPolicyAgent::set_q_value(const State& state, Action action, double value) {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::set_q_value: action out of range");
    }
    Q_[state_action_index(state, action)] = value;
}

void MonteCarloOffPolicyAgent::set_c_value(const State& state, Action action, double value) {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloOffPolicyAgent::set_c_value: action out of range");
    }
    C_[state_action_index(state, action)] = value;
}

const std::vector<double>& MonteCarloOffPolicyAgent::q_data() const noexcept {
    return Q_;
}

const std::vector<double>& MonteCarloOffPolicyAgent::c_data() const noexcept {
    return C_;
}

const std::vector<double>& MonteCarloOffPolicyAgent::tie_noise_data() const noexcept {
    return tie_noise_;
}

void MonteCarloOffPolicyAgent::set_q_data(const std::vector<double>& q_values) {
    if (q_values.size() != Q_.size()) {
        throw std::runtime_error("MonteCarloOffPolicyAgent::set_q_data: wrong vector size");
    }
    Q_ = q_values;
}

void MonteCarloOffPolicyAgent::set_c_data(const std::vector<double>& c_values) {
    if (c_values.size() != C_.size()) {
        throw std::runtime_error("MonteCarloOffPolicyAgent::set_c_data: wrong vector size");
    }
    C_ = c_values;
}

void MonteCarloOffPolicyAgent::set_tie_noise_data(const std::vector<double>& noise_values) {
    if (noise_values.size() != tie_noise_.size()) {
        throw std::runtime_error("MonteCarloOffPolicyAgent::set_tie_noise_data: wrong vector size");
    }
    tie_noise_ = noise_values;
}

std::string MonteCarloOffPolicyAgent::rng_state_string() const {
    std::ostringstream oss;
    oss << rng_;
    return oss.str();
}

void MonteCarloOffPolicyAgent::set_rng_state_string(const std::string& state_str) {
    std::istringstream iss(state_str);
    iss >> rng_;
    if (!iss) {
        throw std::runtime_error("MonteCarloOffPolicyAgent::set_rng_state_string: failed to restore RNG state");
    }
}

}  // namespace rl