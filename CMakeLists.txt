cmake_minimum_required(VERSION 3.16)

project(projeto_rl_cpp VERSION 0.1.0 LANGUAGES CXX)

# --------------------------------------------------
# C++ standard
# --------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Build type default
# --------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# --------------------------------------------------
# Dependencies
# --------------------------------------------------
find_package(nlohmann_json REQUIRED)

# --------------------------------------------------
# Compiler warnings
# --------------------------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# --------------------------------------------------
# Core source files
# --------------------------------------------------
set(RL_CORE_SOURCES
    src/types.cpp
    src/gridworld.cpp
    src/mc_agent.cpp
    src/training.cpp
    src/logging.cpp
    src/checkpointing.cpp
    src/config.cpp
    src/plot_export.cpp
)

# --------------------------------------------------
# Core library
# --------------------------------------------------
add_library(rl_core STATIC ${RL_CORE_SOURCES})

target_include_directories(rl_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(rl_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

target_compile_options(rl_core
    PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0 -g>
)

# --------------------------------------------------
# Main executable
# --------------------------------------------------
add_executable(rl_main
    src/main.cpp
)

target_link_libraries(rl_main
    PRIVATE
        rl_core
)

target_compile_options(rl_main
    PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0 -g>
)

# --------------------------------------------------
# Offline snapshot exporter
# --------------------------------------------------
add_executable(rl_export_snapshots
    tools/export_snapshots.cpp
)

target_link_libraries(rl_export_snapshots
    PRIVATE
        rl_core
)

target_compile_options(rl_export_snapshots
    PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0 -g>
)

# --------------------------------------------------
# Output directory
# --------------------------------------------------
set_target_properties(
    rl_main
    rl_export_snapshots
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)