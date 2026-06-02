#include "mc_agent.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <cstdint>


namespace {

rl::MCControlMode parse_control_mode(const std::string& value) {
    if (value == "on_policy") {
        return rl::MCControlMode::OnPolicy;
    }

    if (value == "off_policy") {
        return rl::MCControlMode::OffPolicy;
    }

    throw std::runtime_error(
        "MonteCarloAgent: control_mode must be 'on_policy' or 'off_policy'"
    );
}

rl::ImportanceSamplingMode parse_importance_sampling_mode(const std::string& value) {
    if (value == "weighted") {
        return rl::ImportanceSamplingMode::Weighted;
    }

    if (value == "ordinary") {
        return rl::ImportanceSamplingMode::Ordinary;
    }

    throw std::runtime_error(
        "MonteCarloAgent: importance_sampling must be 'weighted' or 'ordinary'"
    );
}

} // namespace

namespace rl {

MonteCarloAgent::MonteCarloAgent(
    const GridWorld& env,
    double gamma,
    double epsilon_behavior,
    const std::string& visit_mode,
    const std::string& control_mode,
    const std::string& importance_sampling,
    unsigned int seed
)
    : width_(env.width()),
      height_(env.height()),
      num_actions_(env.num_actions()),
      gamma_(gamma),
      epsilon_behavior_(epsilon_behavior),
      visit_mode_(visit_mode),
      control_mode_name_(control_mode),
      importance_sampling_name_(importance_sampling),
      control_mode_(parse_control_mode(control_mode)),
      importance_sampling_mode_(parse_importance_sampling_mode(importance_sampling)),
      seed_(seed),
      rng_(seed)
{
    if (width_ <= 0 || height_ <= 0 || num_actions_ <= 0) {
        throw std::runtime_error("MonteCarloAgent: invalid environment dimensions");
    }

    if (gamma_ < 0.0 || gamma_ > 1.0) {
        throw std::runtime_error("MonteCarloAgent: gamma must be in [0, 1]");
    }

    if (epsilon_behavior_ < 0.0 || epsilon_behavior_ > 1.0) {
        throw std::runtime_error("MonteCarloAgent: epsilon_behavior must be in [0, 1]");
    }

    if (visit_mode_ != "first_visit" && visit_mode_ != "every_visit") {
        throw std::runtime_error(
            "MonteCarloAgent: visit_mode must be 'first_visit' or 'every_visit'"
        );
    }

    const Index total_sa =
        static_cast<Index>(width_) *
        static_cast<Index>(height_) *
        static_cast<Index>(num_actions_);

    Q_.assign(total_sa, 0.0);
    C_.assign(total_sa, 0.0);
    tie_noise_.assign(total_sa, 0.0);

    const Index total_states =
        static_cast<Index>(width_) *
        static_cast<Index>(height_);

    state_visits_.assign(total_states, 0);
    state_update_counts_.assign(total_states, 0);

    std::normal_distribution<double> noise_dist(0.0, 1e-12);

    for (auto& v : tie_noise_) {
        v = noise_dist(rng_);
    }
}

// --------------------------------------------------
// Basic getters
// --------------------------------------------------

double MonteCarloAgent::gamma() const noexcept {
    return gamma_;
}

double MonteCarloAgent::epsilon_behavior() const noexcept {
    return epsilon_behavior_;
}

const std::string& MonteCarloAgent::visit_mode() const noexcept {
    return visit_mode_;
}

int MonteCarloAgent::width() const noexcept {
    return width_;
}

int MonteCarloAgent::height() const noexcept {
    return height_;
}

int MonteCarloAgent::num_actions() const noexcept {
    return num_actions_;
}

unsigned int MonteCarloAgent::seed() const noexcept {
    return seed_;
}

const std::string& MonteCarloAgent::control_mode() const noexcept {
    return control_mode_name_;
}

const std::string& MonteCarloAgent::importance_sampling() const noexcept {
    return importance_sampling_name_;
}

void MonteCarloAgent::set_epsilon_behavior(double epsilon_behavior) {
    if (epsilon_behavior < 0.0 || epsilon_behavior > 1.0) {
        throw std::runtime_error("MonteCarloAgent::set_epsilon_behavior: epsilon_behavior must be in [0, 1]");
    }

    epsilon_behavior_ = epsilon_behavior;
}

// --------------------------------------------------
// Internal helpers
// --------------------------------------------------   

Index MonteCarloAgent::state_index(const State& state) const noexcept {
    return (
        static_cast<Index>(state.row) * static_cast<Index>(width_) +
        static_cast<Index>(state.col)
    );
}

Index MonteCarloAgent::state_action_index(const State& state, Action action) const noexcept {
    return (
        (static_cast<Index>(state.row) * static_cast<Index>(width_) + static_cast<Index>(state.col))
        * static_cast<Index>(num_actions_)
        + static_cast<Index>(action)
    );
}

Action MonteCarloAgent::greedy_action_from_index(Index state_idx) const noexcept {
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

Action MonteCarloAgent::greedy_action(const State& state) const {
    if (state.row < 0 || state.row >= height_ || state.col < 0 || state.col >= width_) {
        throw std::out_of_range("MonteCarloAgent::greedy_action: state out of bounds");
    }

    const Index state_idx =
        static_cast<Index>(state.row) * static_cast<Index>(width_) + static_cast<Index>(state.col);

    return greedy_action_from_index(state_idx);
}

Action MonteCarloAgent::behavior_action(const State& state) {
    std::uniform_real_distribution<double> u01(0.0, 1.0);

    if (u01(rng_) < epsilon_behavior_) {
        std::uniform_int_distribution<int> action_dist(0, num_actions_ - 1);
        return action_dist(rng_);
    }

    return greedy_action(state);
}

double MonteCarloAgent::behavior_prob(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::behavior_prob: action out of range");
    }

    const Action greedy = greedy_action(state);

    if (action == greedy) {
        return (1.0 - epsilon_behavior_) + (epsilon_behavior_ / static_cast<double>(num_actions_));
    }

    return epsilon_behavior_ / static_cast<double>(num_actions_);
}

double MonteCarloAgent::target_prob(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::target_prob: action out of range");
    }

    if (control_mode_ == MCControlMode::OnPolicy) {
        return behavior_prob(state, action);
    }

    return (action == greedy_action(state)) ? 1.0 : 0.0;
}

// --------------------------------------------------
// Learning helpers
// --------------------------------------------------

bool MonteCarloAgent::should_update_visit(const Episode& episode, std::size_t t) const {
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

UpdateStats MonteCarloAgent::update_from_episode(const Episode& episode) {
    episode.validate();

    if (episode.empty()) {
        return {};
    }

    if (control_mode_ == MCControlMode::OnPolicy) {
        return update_on_policy(episode);
    }

    return update_off_policy(episode);
}

void MonteCarloAgent::apply_mc_update(
    Index idx,
    double denominator_increment,
    double numerator_weight,
    double target
) {
    C_[idx] += denominator_increment;

    if (C_[idx] <= 0.0) {
        throw std::runtime_error("MonteCarloAgent::apply_mc_update: non-positive denominator");
    }

    Q_[idx] += (numerator_weight / C_[idx]) * (target - Q_[idx]);
}

UpdateStats MonteCarloAgent::update_on_policy(const Episode& episode) {
    UpdateStats stats{};

    double G = 0.0;

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

        apply_mc_update(
            idx,
            1.0,
            1.0,
            G
        );

        ++state_update_counts_[state_index(state_t)];
        ++stats.updates_applied;
    }

    return stats;
}

UpdateStats MonteCarloAgent::update_off_policy(const Episode& episode) {
    UpdateStats stats{};

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

        if (importance_sampling_mode_ == ImportanceSamplingMode::Weighted) {
            apply_mc_update(
                idx,
                W,
                W,
                G
            );
        } else {
            apply_mc_update(
                idx,
                1.0,
                1.0,
                W * G
            );
        }

        ++state_update_counts_[state_index(state_t)];
        ++stats.updates_applied;

        const Action greedy = greedy_action(state_t);

        if (action_t != greedy) {
            stats.break_happened = true;
            break;
        }

        const double bprob = behavior_prob(state_t, action_t);

        if (bprob <= 0.0) {
            throw std::runtime_error(
                "MonteCarloAgent::update_off_policy: behavior probability <= 0"
            );
        }

        W *= 1.0 / bprob;
    }

    return stats;
}

// --------------------------------------------------
// Greedy rollout
// --------------------------------------------------

std::vector<State> MonteCarloAgent::greedy_path(GridWorld& env, int max_steps) const {
    if (max_steps <= 0) {
        throw std::runtime_error("MonteCarloAgent::greedy_path: max_steps must be positive");
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

double MonteCarloAgent::q_value(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::q_value: action out of range");
    }
    return Q_[state_action_index(state, action)];
}

double MonteCarloAgent::c_value(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::c_value: action out of range");
    }
    return C_[state_action_index(state, action)];
}

double MonteCarloAgent::tie_noise_value(const State& state, Action action) const {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::tie_noise_value: action out of range");
    }
    return tie_noise_[state_action_index(state, action)];
}

std::uint64_t MonteCarloAgent::state_visit_count(const State& state) const {
    if (state.row < 0 || state.row >= height_ || state.col < 0 || state.col >= width_) {
        throw std::out_of_range("MonteCarloAgent::state_visit_count: state out of bounds");
    }

    return state_visits_[state_index(state)];
}

std::uint64_t MonteCarloAgent::state_update_count(const State& state) const {
    if (state.row < 0 || state.row >= height_ || state.col < 0 || state.col >= width_) {
        throw std::out_of_range("MonteCarloAgent::state_update_count: state out of bounds");
    }

    return state_update_counts_[state_index(state)];
}

void MonteCarloAgent::record_state_visit(const State& state) {
    if (state.row < 0 || state.row >= height_ || state.col < 0 || state.col >= width_) {
        throw std::out_of_range("MonteCarloAgent::record_state_visit: state out of bounds");
    }

    ++state_visits_[state_index(state)];
}

void MonteCarloAgent::set_q_value(const State& state, Action action, double value) {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::set_q_value: action out of range");
    }
    Q_[state_action_index(state, action)] = value;
}

void MonteCarloAgent::set_c_value(const State& state, Action action, double value) {
    if (action < 0 || action >= num_actions_) {
        throw std::out_of_range("MonteCarloAgent::set_c_value: action out of range");
    }
    C_[state_action_index(state, action)] = value;
}

const std::vector<double>& MonteCarloAgent::q_data() const noexcept {
    return Q_;
}

const std::vector<double>& MonteCarloAgent::c_data() const noexcept {
    return C_;
}

const std::vector<double>& MonteCarloAgent::tie_noise_data() const noexcept {
    return tie_noise_;
}

const std::vector<std::uint64_t>& MonteCarloAgent::state_visit_data() const noexcept {
    return state_visits_;
}

const std::vector<std::uint64_t>& MonteCarloAgent::state_update_count_data() const noexcept {
    return state_update_counts_;
}

void MonteCarloAgent::set_q_data(const std::vector<double>& q_values) {
    if (q_values.size() != Q_.size()) {
        throw std::runtime_error("MonteCarloAgent::set_q_data: wrong vector size");
    }
    Q_ = q_values;
}

void MonteCarloAgent::set_c_data(const std::vector<double>& c_values) {
    if (c_values.size() != C_.size()) {
        throw std::runtime_error("MonteCarloAgent::set_c_data: wrong vector size");
    }
    C_ = c_values;
}

void MonteCarloAgent::set_tie_noise_data(const std::vector<double>& noise_values) {
    if (noise_values.size() != tie_noise_.size()) {
        throw std::runtime_error("MonteCarloAgent::set_tie_noise_data: wrong vector size");
    }
    tie_noise_ = noise_values;
}

void MonteCarloAgent::set_state_visit_data(const std::vector<std::uint64_t>& visits) {
    if (visits.size() != state_visits_.size()) {
        throw std::runtime_error("MonteCarloAgent::set_state_visit_data: wrong vector size");
    }

    state_visits_ = visits;
}

void MonteCarloAgent::set_state_update_count_data(
    const std::vector<std::uint64_t>& update_counts
) {
    if (update_counts.size() != state_update_counts_.size()) {
        throw std::runtime_error("MonteCarloAgent::set_state_update_count_data: wrong vector size");
    }

    state_update_counts_ = update_counts;
}

std::string MonteCarloAgent::rng_state_string() const {
    std::ostringstream oss;
    oss << rng_;
    return oss.str();
}

void MonteCarloAgent::set_rng_state_string(const std::string& state_str) {
    std::istringstream iss(state_str);
    iss >> rng_;
    if (!iss) {
        throw std::runtime_error("MonteCarloAgent::set_rng_state_string: failed to restore RNG state");
    }
}

}  // namespace rl
