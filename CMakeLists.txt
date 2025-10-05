cmake_minimum_required(VERSION 3.20)
project(ChessWizard CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Compiler Flags as per Specification ---

# General flags for all build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Release flags for performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -funroll-loops -fomit-frame-pointer -fno-exceptions -fno-rtti -DNDEBUG")

# Debug flags for sanity checking
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fsanitize=address,undefined -DDEBUG")

# --- Source Files ---

# Use glob to automatically find all .cpp files in src
file(GLOB SOURCE_FILES "src/*.cpp")

# --- Targets as per Specification ---

# 1. Binary Target: chess_wizard
add_executable(chess_wizard ${SOURCE_FILES})

# Public include directory for the executable
target_include_directories(chess_wizard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 2. Static Library Target: libchesswizard.a
add_library(libchesswizard STATIC ${SOURCE_FILES})

# Public include directory for the library
target_include_directories(libchesswizard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Build Information ---

# Print configuration for user feedback
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")