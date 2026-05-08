#include "logging.hpp"

#include <stdexcept>

namespace rl {

CSVLogger::CSVLogger(const std::filesystem::path& file_path)
    : file_path_(file_path) {
    const auto parent = file_path_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    file_.open(file_path_, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        throw std::runtime_error("CSVLogger: failed to open file: " + file_path_.string());
    }
}

void CSVLogger::write_header() {
    if (header_written_) {
        return;
    }

    file_
        << "episode,"
        << "episode_return,"
        << "episode_length,"
        << "success,"
        << "updates_applied,"
        << "break_happened,"
        << "episode_time_sec,"
        << "generation_time_sec,"
        << "update_time_sec\n";

    header_written_ = true;
}

void CSVLogger::append_row(const EpisodeLogRow& row) {
    if (!header_written_) {
        write_header();
    }

    file_
        << row.episode << ','
        << row.episode_return << ','
        << row.episode_length << ','
        << row.success << ','
        << row.updates_applied << ','
        << row.break_happened << ','
        << row.episode_time_sec << ','
        << row.generation_time_sec << ','
        << row.update_time_sec << '\n';
}

void CSVLogger::flush() {
    file_.flush();
}

const std::filesystem::path& CSVLogger::file_path() const noexcept {
    return file_path_;
}

}  // namespace rl