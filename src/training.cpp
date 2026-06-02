#include "training.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace rl {

namespace {

template <typename T>
double average_last_n(const std::vector<T>& values, int n) {
    if (values.empty()) {
        return 0.0;
    }

    const int count = std::min<int>(n, static_cast<int>(values.size()));
    const auto begin_it = values.end() - count;

    double sum = 0.0;
    for (auto it = begin_it; it != values.end(); ++it) {
        sum += static_cast<double>(*it);
    }

    return sum / static_cast<double>(count);
}

}  // namespace

Episode generate_episode(
    GridWorld& env,
    MonteCarloAgent& agent,
    int max_steps
) {
    if (max_steps <= 0) {
        throw std::runtime_error("generate_episode: max_steps must be positive");
    }

    Episode episode;
    episode.reserve(static_cast<std::size_t>(max_steps));

    State state = env.reset();

    agent.record_state_visit(state);

    for (int step = 0; step < max_steps; ++step) {
        const Action action = agent.behavior_action(state);
        const StepResult result = env.step(action);

        episode.add_transition(state, action, result.reward);
        state = result.next_state;
        agent.record_state_visit(result.next_state);

        if (result.terminated || result.truncated) {
            episode.terminated = result.terminated;
            episode.truncated = result.truncated;
            break;
        }
    }

    episode.validate();
    return episode;
}

TrainingHistory train_monte_carlo(
    GridWorld& env,
    MonteCarloAgent& agent,
    const TrainingConfig& config,
    CSVLogger* logger
) {
    if (config.episodes <= 0) {
        throw std::runtime_error("train_monte_carlo: episodes must be positive");
    }
    if (config.max_steps <= 0) {
        throw std::runtime_error("train_monte_carlo: max_steps must be positive");
    }
    if (config.print_every <= 0) {
        throw std::runtime_error("train_monte_carlo: print_every must be positive");
    }
    if (config.flush_every <= 0) {
        throw std::runtime_error("train_monte_carlo: flush_every must be positive");
    }

    TrainingHistory history;
    history.episode_return.reserve(static_cast<std::size_t>(config.episodes));
    history.episode_length.reserve(static_cast<std::size_t>(config.episodes));
    history.success.reserve(static_cast<std::size_t>(config.episodes));
    history.updates_applied.reserve(static_cast<std::size_t>(config.episodes));
    history.break_happened.reserve(static_cast<std::size_t>(config.episodes));
    history.episode_time_sec.reserve(static_cast<std::size_t>(config.episodes));
    history.generation_time_sec.reserve(static_cast<std::size_t>(config.episodes));
    history.update_time_sec.reserve(static_cast<std::size_t>(config.episodes));

    using clock = std::chrono::steady_clock;
    const auto train_start = clock::now();

    if (logger != nullptr) {
        logger->write_header();
    }

    for (int ep = 1; ep <= config.episodes; ++ep) {
        const auto ep_start = clock::now();

        const auto gen_start = clock::now();
        Episode episode = generate_episode(env, agent, config.max_steps);
        const auto gen_end = clock::now();

        const auto upd_start = clock::now();
        const UpdateStats stats = agent.update_from_episode(episode);
        const auto upd_end = clock::now();

        const std::chrono::duration<double> episode_time = upd_end - ep_start;
        const std::chrono::duration<double> generation_time = gen_end - gen_start;
        const std::chrono::duration<double> update_time = upd_end - upd_start;

        const double ep_return = episode.total_reward();
        const int ep_length = static_cast<int>(episode.length());
        const int ep_success = episode.terminated ? 1 : 0;
        const int ep_updates = stats.updates_applied;
        const int ep_break = stats.break_happened ? 1 : 0;

        history.episode_return.push_back(ep_return);
        history.episode_length.push_back(ep_length);
        history.success.push_back(ep_success);
        history.updates_applied.push_back(ep_updates);
        history.break_happened.push_back(ep_break);

        history.episode_time_sec.push_back(episode_time.count());
        history.generation_time_sec.push_back(generation_time.count());
        history.update_time_sec.push_back(update_time.count());

        if (logger != nullptr) {
            EpisodeLogRow row;
            row.episode = ep;
            row.episode_return = ep_return;
            row.episode_length = ep_length;
            row.success = ep_success;
            row.updates_applied = ep_updates;
            row.break_happened = ep_break;
            row.episode_time_sec = episode_time.count();
            row.generation_time_sec = generation_time.count();
            row.update_time_sec = update_time.count();

            logger->append_row(row);

            if (ep % config.flush_every == 0) {
                logger->flush();
            }
        }

        if (ep % config.print_every == 0) {
            const double avg_return = average_last_n(history.episode_return, config.print_every);
            const double avg_len = average_last_n(history.episode_length, config.print_every);
            const double success_rate = 100.0 * average_last_n(history.success, config.print_every);
            const double avg_updates = average_last_n(history.updates_applied, config.print_every);
            const double break_rate = 100.0 * average_last_n(history.break_happened, config.print_every);

            const double avg_ep_time = average_last_n(history.episode_time_sec, config.print_every);
            const double avg_gen_time = average_last_n(history.generation_time_sec, config.print_every);
            const double avg_upd_time = average_last_n(history.update_time_sec, config.print_every);

            std::cout
                << "Ep " << ep
                << " | AvgReturn=" << avg_return
                << " | AvgLen=" << avg_len
                << " | Success=" << success_rate << "%"
                << " | AvgUpdates=" << avg_updates
                << " | BreakRate=" << break_rate << "%"
                << " | EpTime=" << avg_ep_time << "s"
                << " | GenTime=" << avg_gen_time << "s"
                << " | UpdTime=" << avg_upd_time << "s"
                << '\n';
        }

        history.completed_episodes = ep;

        if (config.after_episode_callback) {
            config.after_episode_callback(ep);
        }

        if (config.should_stop && config.should_stop()) {
            history.interrupted = true;

            std::cout
                << "\nStop requested. Training will exit after completed episode "
                << ep << ".\n";

            break;
        }
    }

    if (logger != nullptr) {
        logger->flush();
    }

    const std::chrono::duration<double> total_train_time = clock::now() - train_start;

    std::cout << "\nTotal training time: " << total_train_time.count() << "s\n";
    std::cout << "Average time per episode: "
              << average_last_n(history.episode_time_sec, static_cast<int>(history.episode_time_sec.size()))
              << "s\n";

    return history;
}

}  // namespace rl