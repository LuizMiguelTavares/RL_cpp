#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace rl {

struct EpisodeLogRow {
    int episode{0};
    double episode_return{0.0};
    int episode_length{0};
    int success{0};
    int updates_applied{0};
    int break_happened{0};

    double episode_time_sec{0.0};
    double generation_time_sec{0.0};
    double update_time_sec{0.0};
};

class CSVLogger {
public:
    explicit CSVLogger(const std::filesystem::path& file_path);

    void write_header();
    void append_row(const EpisodeLogRow& row);
    void flush();

    [[nodiscard]] const std::filesystem::path& file_path() const noexcept;

private:
    std::filesystem::path file_path_;
    std::ofstream file_;
    bool header_written_{false};
};

}  // namespace rl