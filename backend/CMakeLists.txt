cmake_minimum_required(VERSION 3.20)
project(pathfinder_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Foundation library
add_library(pathfinder_foundation STATIC
    src/foundation/error.cpp
    src/foundation/id.cpp
    src/foundation/version.cpp
    src/foundation/hash.cpp
    src/foundation/time.cpp
    src/foundation/random.cpp
)

target_include_directories(pathfinder_foundation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Config library (P1)
add_library(pathfinder_config STATIC
    src/config/validation.cpp
    src/config/package.cpp
    src/config/loader.cpp
)

target_include_directories(pathfinder_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_config PUBLIC
    pathfinder_foundation
)

# Command library (P1)
add_library(pathfinder_command STATIC
    src/command/types.cpp
    src/command/target.cpp
    src/command/action_command.cpp
    src/command/envelope.cpp
    src/command/validation.cpp
)

target_include_directories(pathfinder_command PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pathfinder_command PUBLIC
    pathfinder_foundation
)

# Tests
add_subdirectory(tests)
