#pragma once

#include <cstddef>
#include <ostream>

namespace rl {

struct State {
    int row{0};
    int col{0};

    bool operator==(const State& other) const noexcept;
    bool operator!=(const State& other) const noexcept;
};

struct StepResult {
    State next_state{};
    double reward{0.0};
    bool terminated{false};
    bool truncated{false};
    bool invalid_move{false};
};

using Action = int;
using Index = std::size_t;

std::ostream& operator<<(std::ostream& os, const State& state);
std::ostream& operator<<(std::ostream& os, const StepResult& result);

}  // namespace rl