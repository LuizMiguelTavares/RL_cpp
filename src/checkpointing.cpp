#include "checkpointing.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace rl {

namespace {

template <typename T>
void save_vector_binary(const std::filesystem::path& file_path, const std::vector<T>& data) {
    std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("save_vector_binary: failed to open file: " + file_path.string());
    }

    const std::uint64_t size = static_cast<std::uint64_t>(data.size());
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));

    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(sizeof(T) * data.size()));
    }

    if (!out) {
        throw std::runtime_error("save_vector_binary: failed to write file: " + file_path.string());
    }
}

template <typename T>
std::vector<T> load_vector_binary(const std::filesystem::path& file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("load_vector_binary: failed to open file: " + file_path.string());
    }

    std::uint64_t size = 0;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    if (!in) {
        throw std::runtime_error("load_vector_binary: failed to read vector size from: " + file_path.string());
    }

    std::vector<T> data(static_cast<std::size_t>(size));
    if (size > 0) {
        in.read(reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(sizeof(T) * data.size()));
    }

    if (!in) {
        throw std::runtime_error("load_vector_binary: failed to read vector data from: " + file_path.string());
    }

    return data;
}

struct Metadata {
    int checkpoint_version{1};
    int last_completed_episode{0};

    int width{0};
    int height{0};

    int start_row{0};
    int start_col{0};
    int goal_row{0};
    int goal_col{0};

    int allow_diagonal{0};
    int max_steps{0};

    double diagonal_cost{1.0};
    double reward_goal{100.0};
    double reward_obstacle{-10.0};
    double reward_step{-0.001};

    std::string shaping{"euclidean"};

    double gamma{0.99};
    double epsilon_behavior{0.2};
    std::string visit_mode{"first_visit"};

    std::string agent_rng_state;
};

void save_metadata(const std::filesystem::path& file_path, const Metadata& meta) {
    std::ofstream out(file_path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("save_metadata: failed to open file: " + file_path.string());
    }

    out << "checkpoint_version " << meta.checkpoint_version << '\n';
    out << "last_completed_episode " << meta.last_completed_episode << '\n';

    out << "width " << meta.width << '\n';
    out << "height " << meta.height << '\n';

    out << "start_row " << meta.start_row << '\n';
    out << "start_col " << meta.start_col << '\n';
    out << "goal_row " << meta.goal_row << '\n';
    out << "goal_col " << meta.goal_col << '\n';

    out << "allow_diagonal " << meta.allow_diagonal << '\n';
    out << "max_steps " << meta.max_steps << '\n';

    out << "diagonal_cost " << meta.diagonal_cost << '\n';
    out << "reward_goal " << meta.reward_goal << '\n';
    out << "reward_obstacle " << meta.reward_obstacle << '\n';
    out << "reward_step " << meta.reward_step << '\n';

    out << "shaping " << meta.shaping << '\n';

    out << "gamma " << meta.gamma << '\n';
    out << "epsilon_behavior " << meta.epsilon_behavior << '\n';
    out << "visit_mode " << meta.visit_mode << '\n';

    out << "agent_rng_state " << std::quoted(meta.agent_rng_state) << '\n';

    if (!out) {
        throw std::runtime_error("save_metadata: failed to write file: " + file_path.string());
    }
}

Metadata load_metadata(const std::filesystem::path& file_path) {
    std::ifstream in(file_path);
    if (!in.is_open()) {
        throw std::runtime_error("load_metadata: failed to open file: " + file_path.string());
    }

    Metadata meta;
    std::string key;

    while (in >> key) {
        if (key == "checkpoint_version") {
            in >> meta.checkpoint_version;
        } else if (key == "last_completed_episode") {
            in >> meta.last_completed_episode;
        } else if (key == "width") {
            in >> meta.width;
        } else if (key == "height") {
            in >> meta.height;
        } else if (key == "start_row") {
            in >> meta.start_row;
        } else if (key == "start_col") {
            in >> meta.start_col;
        } else if (key == "goal_row") {
            in >> meta.goal_row;
        } else if (key == "goal_col") {
            in >> meta.goal_col;
        } else if (key == "allow_diagonal") {
            in >> meta.allow_diagonal;
        } else if (key == "max_steps") {
            in >> meta.max_steps;
        } else if (key == "diagonal_cost") {
            in >> meta.diagonal_cost;
        } else if (key == "reward_goal") {
            in >> meta.reward_goal;
        } else if (key == "reward_obstacle") {
            in >> meta.reward_obstacle;
        } else if (key == "reward_step") {
            in >> meta.reward_step;
        } else if (key == "shaping") {
            in >> meta.shaping;
        } else if (key == "gamma") {
            in >> meta.gamma;
        } else if (key == "epsilon_behavior") {
            in >> meta.epsilon_behavior;
        } else if (key == "visit_mode") {
            in >> meta.visit_mode;
        } else if (key == "agent_rng_state") {
            in >> std::quoted(meta.agent_rng_state);
        } else {
            throw std::runtime_error("load_metadata: unknown key in metadata file: " + key);
        }
    }

    return meta;
}

}  // namespace

void save_checkpoint(
    const std::filesystem::path& checkpoint_dir,
    const GridWorld& env,
    const MonteCarloOffPolicyAgent& agent,
    int last_completed_episode
) {
    std::filesystem::create_directories(checkpoint_dir);

    Metadata meta;
    meta.last_completed_episode = last_completed_episode;

    meta.width = env.width();
    meta.height = env.height();

    meta.start_row = env.start().row;
    meta.start_col = env.start().col;
    meta.goal_row = env.goal().row;
    meta.goal_col = env.goal().col;

    meta.allow_diagonal = env.allow_diagonal() ? 1 : 0;
    meta.max_steps = env.max_steps();

    meta.diagonal_cost = env.diagonal_cost();
    meta.reward_goal = env.reward_goal();
    meta.reward_obstacle = env.reward_obstacle();
    meta.reward_step = env.reward_step();
    meta.shaping = env.shaping();

    meta.gamma = agent.gamma();
    meta.epsilon_behavior = agent.epsilon_behavior();
    meta.visit_mode = agent.visit_mode();
    meta.agent_rng_state = agent.rng_state_string();

    save_metadata(checkpoint_dir / "meta.txt", meta);
    save_vector_binary(checkpoint_dir / "grid.bin", env.grid_data());
    save_vector_binary(checkpoint_dir / "Q.bin", agent.q_data());
    save_vector_binary(checkpoint_dir / "C.bin", agent.c_data());
    save_vector_binary(checkpoint_dir / "tie_noise.bin", agent.tie_noise_data());
    save_vector_binary(checkpoint_dir / "state_visits.bin", agent.state_visit_data());
    save_vector_binary(checkpoint_dir / "state_update_counts.bin", agent.state_update_count_data());
}

LoadedCheckpoint load_checkpoint(
    const std::filesystem::path& checkpoint_dir
) {
    const Metadata meta = load_metadata(checkpoint_dir / "meta.txt");

    std::vector<std::uint8_t> grid = load_vector_binary<std::uint8_t>(checkpoint_dir / "grid.bin");
    std::vector<double> Q = load_vector_binary<double>(checkpoint_dir / "Q.bin");
    std::vector<double> C = load_vector_binary<double>(checkpoint_dir / "C.bin");
    std::vector<double> tie_noise = load_vector_binary<double>(checkpoint_dir / "tie_noise.bin");
    std::vector<std::uint64_t> state_visits;
    std::vector<std::uint64_t> state_update_counts;

    const std::filesystem::path state_visits_path =
        checkpoint_dir / "state_visits.bin";

    const std::filesystem::path state_update_counts_path =
        checkpoint_dir / "state_update_counts.bin";

    if (std::filesystem::exists(state_visits_path)) {
        state_visits = load_vector_binary<std::uint64_t>(state_visits_path);
    }

    if (std::filesystem::exists(state_update_counts_path)) {
        state_update_counts = load_vector_binary<std::uint64_t>(state_update_counts_path);
    }

    GridWorld env(
        meta.width,
        meta.height,
        0.0,                                  // obstacle_density dummy
        "random",                             // obstacle_mode dummy
        0,                                    // seed dummy
        1,                                    // cluster_size dummy
        State{meta.start_row, meta.start_col},
        State{meta.goal_row, meta.goal_col},
        meta.allow_diagonal != 0,
        meta.diagonal_cost,
        meta.reward_goal,
        meta.reward_obstacle,
        meta.reward_step,
        meta.shaping,
        meta.max_steps,
        false,                                // ensure_solvable not needed here
        1
    );

    env.set_grid(grid);

    MonteCarloOffPolicyAgent agent(
        env,
        meta.gamma,
        meta.epsilon_behavior,
        meta.visit_mode,
        0
    );

    agent.set_q_data(Q);
    agent.set_c_data(C);
    agent.set_tie_noise_data(tie_noise);

    if (!state_visits.empty()) {
        agent.set_state_visit_data(state_visits);
    }

    if (!state_update_counts.empty()) {
        agent.set_state_update_count_data(state_update_counts);
    }

    agent.set_rng_state_string(meta.agent_rng_state);

    return LoadedCheckpoint{
        std::move(env),
        std::move(agent),
        meta.last_completed_episode
    };
}

}  // namespace rl