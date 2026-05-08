#include "types.hpp"

#include <ostream>

namespace rl {

bool State::operator==(const State& other) const noexcept {
    return row == other.row && col == other.col;
}

bool State::operator!=(const State& other) const noexcept {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const State& state) {
    os << "(" << state.row << ", " << state.col << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const StepResult& result) {
    os << "{ next_state: " << result.next_state
       << ", reward: " << result.reward
       << ", terminated: " << result.terminated
       << ", truncated: " << result.truncated
       << ", invalid_move: " << result.invalid_move
       << " }";
    return os;
}

}  // namespace rl