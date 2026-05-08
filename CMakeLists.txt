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
# Compiler warnings
# --------------------------------------------------
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# Optional stronger warnings:
# add_compile_options(
#     -Wall
#     -Wextra
#     -Wpedantic
#     -Wconversion
#     -Wshadow
# )

# --------------------------------------------------
# Includes
# --------------------------------------------------
include_directories(include)

# --------------------------------------------------
# Source files
# --------------------------------------------------
set(SOURCES
    src/main.cpp
    src/types.cpp
    src/gridworld.cpp
    src/mc_agent.cpp
    src/training.cpp
    src/logging.cpp
    src/checkpointing.cpp
    src/config.cpp
)

# --------------------------------------------------
# Executable
# --------------------------------------------------
add_executable(rl_main ${SOURCES})

find_package(nlohmann_json REQUIRED)
target_link_libraries(rl_main PRIVATE nlohmann_json::nlohmann_json)

target_include_directories(rl_main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# --------------------------------------------------
# Optimization / debug flags
# --------------------------------------------------
target_compile_options(rl_main PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)

# --------------------------------------------------
# Output directory
# --------------------------------------------------
set_target_properties(rl_main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

