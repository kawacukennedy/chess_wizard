cmake_minimum_required(VERSION 3.22)
project(ChessWizard CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Compiler and Feature Checks ---

# Check for C++20 support
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "GCC 10.0 or later is required for C++20 support.")
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.0 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "Clang 12.0 or later is required for C++20 support.")
endif()

# --- Compiler Flags as per Specification ---

# Option to disable -march=native
option(USE_MARCH_NATIVE "Enable -march=native for performance" ON)

# General flags for all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Release flags for performance
set(RELEASE_FLAGS "-O3 -flto -funroll-loops -fomit-frame-pointer -fno-exceptions -fno-rtti -DNDEBUG")
if(USE_MARCH_NATIVE)
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -march=native")
endif()
set(CMAKE_CXX_FLAGS_RELEASE ${RELEASE_FLAGS})

# Debug flags for sanity checking
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fsanitize=address,undefined -DDEBUG")

# --- Source Files ---

# Use glob to automatically find all .cpp files in src
file(GLOB SOURCE_FILES "src/*.cpp")
# Explicitly list to ensure order
set(SOURCE_FILES
    src/api.cpp
    src/attack.cpp
    src/bitboard.cpp
    src/book.cpp
    src/engine.cpp
    src/evaluate.cpp
    src/main.cpp
    src/move.cpp
    src/movegen.cpp
    src/nnue.cpp
    src/board.cpp
    src/search.cpp
    src/tb_probe.cpp
    src/tt.cpp
    src/zobrist.cpp
)

# --- Targets as per Specification ---

# 1. Binary Target: chess_wizard
add_executable(chess_wizard ${SOURCE_FILES})

# Public include directory for the executable
target_include_directories(chess_wizard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 2. Static Library Target: libchesswizard.a
# add_library(libchesswizard STATIC ${SOURCE_FILES})

# Public include directory for the library
# target_include_directories(libchesswizard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 3. Shared Library Target: libchesswizard.so (for Python wrapper)
# add_library(libchesswizard_shared SHARED ${SOURCE_FILES})

# Public include directory for the shared library
# target_include_directories(libchesswizard_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Build Information ---

# Print configuration for user feedback
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")