#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "checkpointing.hpp"
#include "config.hpp"
#include "gridworld.hpp"
#include "logging.hpp"
#include "mc_agent.hpp"
#include "training.hpp"
#include "types.hpp"
#include "plot_export.hpp"
#include <csignal>

namespace {

void print_usage(const char* program_name) {
    std::cout
        << "Usage:\n"
        << "  " << program_name << " [--config <config_path>] [--resume]\n\n"
        << "Examples:\n"
        << "  " << program_name << '\n'
        << "  " << program_name << " --config config.json\n"
        << "  " << program_name << " --config config.json --resume\n";
}

bool checkpoint_exists(const std::filesystem::path& checkpoint_dir) {
    return std::filesystem::exists(checkpoint_dir / "meta.txt") &&
           std::filesystem::exists(checkpoint_dir / "grid.bin") &&
           std::filesystem::exists(checkpoint_dir / "Q.bin") &&
           std::filesystem::exists(checkpoint_dir / "C.bin") &&
           std::filesystem::exists(checkpoint_dir / "tie_noise.bin");
}

volatile std::sig_atomic_t g_stop_requested = 0;

void handle_sigint(int) {
    g_stop_requested = 1;
}

bool stop_requested() {
    return g_stop_requested != 0;
}

void save_latest_checkpoint(
    const std::filesystem::path& latest_ckpt_dir,
    const rl::GridWorld& env,
    const rl::MonteCarloOffPolicyAgent& agent,
    int last_completed_episode
) {
    rl::save_checkpoint(latest_ckpt_dir, env, agent, last_completed_episode);
}

void save_numbered_checkpoint(
    const std::filesystem::path& checkpoints_dir,
    const rl::GridWorld& env,
    const rl::MonteCarloOffPolicyAgent& agent,
    int last_completed_episode
) {
    const std::filesystem::path numbered_ckpt =
        checkpoints_dir / ("ep_" + std::to_string(last_completed_episode));

    rl::save_checkpoint(numbered_ckpt, env, agent, last_completed_episode);
}

bool is_due_every(int completed_this_run, int every) {
    return every > 0 && completed_this_run % every == 0;
}

bool is_due_snapshot_schedule(
    int completed_this_run,
    const std::vector<rl::SnapshotScheduleEntry>& schedule
) {
    for (const auto& entry : schedule) {
        if (completed_this_run <= entry.until) {
            return is_due_every(completed_this_run, entry.every);
        }
    }

    return false;
}

bool should_save_snapshot(
    int completed_this_run,
    int snapshot_every,
    const std::vector<rl::SnapshotScheduleEntry>& schedule
) {
    if (!schedule.empty()) {
        return is_due_snapshot_schedule(completed_this_run, schedule);
    }

    return is_due_every(completed_this_run, snapshot_every);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        using namespace rl;

        std::signal(SIGINT, handle_sigint);

        std::filesystem::path config_path = "config.json";
        bool force_resume = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--config") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--config requires a value");
                }
                config_path = argv[++i];
            }
            else if (arg == "--resume") {
                force_resume = true;
            }
            else if (arg == "--help" || arg == "-h") {
                print_usage(argv[0]);
                return 0;
            }
            else {
                throw std::runtime_error("Unknown argument: " + arg);
            }
        }

        RunConfig cfg = load_config(config_path);
        if (force_resume) {
            cfg.resume = true;
        }

        const std::filesystem::path run_dir = std::filesystem::path("runs") / cfg.run_name;
        const std::filesystem::path checkpoints_dir = run_dir / "checkpoints";
        const std::filesystem::path latest_ckpt_dir = checkpoints_dir / "latest";

        std::filesystem::create_directories(run_dir);
        std::filesystem::create_directories(checkpoints_dir);

        std::cout << "Config path: " << config_path << '\n';
        std::cout << "Run directory: " << run_dir << '\n';
        std::cout << "Resume mode: " << (cfg.resume ? "true" : "false") << "\n\n";

        std::unique_ptr<GridWorld> env_ptr;
        std::unique_ptr<MonteCarloOffPolicyAgent> agent_ptr;
        int last_completed_episode = 0;

        if (cfg.resume && checkpoint_exists(latest_ckpt_dir)) {
            LoadedCheckpoint ckpt = load_checkpoint(latest_ckpt_dir);

            env_ptr = std::make_unique<GridWorld>(std::move(ckpt.env));
            agent_ptr = std::make_unique<MonteCarloOffPolicyAgent>(std::move(ckpt.agent));
            last_completed_episode = ckpt.last_completed_episode;

            std::cout << "Loaded checkpoint from: " << latest_ckpt_dir << '\n';
            std::cout << "Last completed episode: " << last_completed_episode << "\n\n";
        } else {
            if (cfg.resume) {
                std::cout
                    << "Resume requested, but no valid latest checkpoint was found at: "
                    << latest_ckpt_dir << '\n'
                    << "Starting a new run instead.\n\n";

                // From this point on, this execution behaves as a fresh run.
                cfg.resume = false;
            }

            env_ptr = std::make_unique<GridWorld>(
                cfg.environment.width,
                cfg.environment.height,
                cfg.environment.obstacle_density,
                cfg.environment.obstacle_mode,
                cfg.environment.seed,
                cfg.environment.cluster_size,
                State{cfg.environment.start_row, cfg.environment.start_col},
                State{cfg.environment.goal_row, cfg.environment.goal_col},
                cfg.environment.allow_diagonal,
                cfg.environment.diagonal_cost,
                cfg.environment.reward_goal,
                cfg.environment.reward_obstacle,
                cfg.environment.reward_step,
                cfg.environment.shaping,
                cfg.training.max_steps,
                cfg.environment.ensure_solvable,
                cfg.environment.obstacle_generation_tries
            );

            agent_ptr = std::make_unique<MonteCarloOffPolicyAgent>(
                *env_ptr,
                cfg.agent.gamma,
                cfg.agent.epsilon_behavior,
                cfg.agent.visit_mode,
                cfg.agent.seed
            );

            std::cout << "GridWorld created.\n";
            std::cout << "Size: " << env_ptr->width() << " x " << env_ptr->height() << '\n';
            std::cout << "Num actions: " << env_ptr->num_actions() << '\n';
            std::cout << "Start: " << env_ptr->start() << '\n';
            std::cout << "Goal: " << env_ptr->goal() << "\n\n";

            std::cout << "Initial greedy action at start: "
                    << agent_ptr->greedy_action(env_ptr->start()) << '\n';

            for (Action a = 0; a < env_ptr->num_actions(); ++a) {
                std::cout << "Q(start," << a << ") = "
                        << agent_ptr->q_value(env_ptr->start(), a)
                        << " | tie_noise = "
                        << agent_ptr->tie_noise_value(env_ptr->start(), a)
                        << '\n';
            }
            std::cout << '\n';
        }

        GridWorld& env = *env_ptr;
        MonteCarloOffPolicyAgent& agent = *agent_ptr;

        const int episodes_this_run = cfg.training.episodes_this_run;
        if (episodes_this_run < 0) {
            throw std::runtime_error("episodes_this_run must be >= 0");
        }

        std::cout << "Already completed overall: " << last_completed_episode << '\n';
        std::cout << "Episodes to train now: " << episodes_this_run << "\n\n";

        TrainingHistory history;

        if (episodes_this_run > 0) {
            std::filesystem::path log_path;
            if (cfg.resume) {
                log_path = run_dir / ("train_history_resume_from_" +
                                      std::to_string(last_completed_episode + 1) + ".csv");
            } else {
                log_path = run_dir / "train_history.csv";
            }

            CSVLogger logger(log_path);

            TrainingConfig train_cfg;
            train_cfg.episodes = episodes_this_run;
            train_cfg.max_steps = cfg.training.max_steps;
            train_cfg.print_every = cfg.training.print_every;
            train_cfg.flush_every = cfg.training.flush_every;

            std::cout << "Training started...\n\n";

            const int starting_completed_episode = last_completed_episode;

            train_cfg.after_episode_callback =
                [&](int completed_this_run) {
                    const int completed_overall =
                        starting_completed_episode + completed_this_run;

                    // Safety checkpoint for resume.
                    // This updates only checkpoints/latest.
                    if (is_due_every(completed_this_run, cfg.training.checkpoint_every)) {
                        save_latest_checkpoint(
                            latest_ckpt_dir,
                            env,
                            agent,
                            completed_overall
                        );
                    }

                    // Numbered snapshots for plotting/animation.
                    // These create checkpoints/ep_N.
                    if (
                        should_save_snapshot(
                            completed_this_run,
                            cfg.training.snapshot_every,
                            cfg.training.snapshot_schedule
                        )
                    ) {
                        save_numbered_checkpoint(
                            checkpoints_dir,
                            env,
                            agent,
                            completed_overall
                        );
                    }
                };

            train_cfg.should_stop = []() {
                return stop_requested();
            };

            history = train_mc_offpolicy(env, agent, train_cfg, &logger);

            last_completed_episode += history.completed_episodes;

            if (history.completed_episodes > 0) {
                save_latest_checkpoint(
                    latest_ckpt_dir,
                    env,
                    agent,
                    last_completed_episode
                );

                save_numbered_checkpoint(
                    checkpoints_dir,
                    env,
                    agent,
                    last_completed_episode
                );

                const std::filesystem::path numbered_ckpt =
                    checkpoints_dir / ("ep_" + std::to_string(last_completed_episode));

                std::cout << "\nCheckpoint saved to: " << latest_ckpt_dir << '\n';
                std::cout << "Snapshot checkpoint saved to: " << numbered_ckpt << '\n';
            }

            if (history.interrupted) {
                std::cout
                    << "Graceful stop completed after episode "
                    << last_completed_episode << ".\n";
            }

            std::cout << "Training CSV saved to: " << log_path << '\n';
        } else {
            std::cout << "No episodes requested for this run.\n";
        }

        const std::filesystem::path export_dir = run_dir / "export";

        export_gridworld_plot_data(
            export_dir,
            env,
            agent,
            last_completed_episode
        );

        std::cout << "Plot data exported to: " << export_dir << '\n';

        std::vector<State> path = agent.greedy_path(env, cfg.training.max_steps);

        std::cout << "\nGreedy path:\n";
        for (const State& s : path) {
            std::cout << s << ' ';
        }
        std::cout << "\n\n";

        const int n = static_cast<int>(history.success.size());
        const int last_k = (n < 100) ? n : 100;

        double final_success_sum = 0.0;
        for (int i = n - last_k; i < n; ++i) {
            final_success_sum += static_cast<double>(history.success[static_cast<std::size_t>(i)]);
        }

        const double final_success_rate =
            (last_k > 0) ? (100.0 * final_success_sum / static_cast<double>(last_k)) : 0.0;

        std::cout << "Final success rate over last " << last_k
                  << " episodes of this run: " << final_success_rate << "%\n";

        std::cout << "Last completed episode overall: " << last_completed_episode << '\n';
        std::cout << "Run finished.\n";

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        print_usage(argv[0]);
        return 1;
    }
}