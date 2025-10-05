#ifndef ENGINE_H
#define ENGINE_H

#include "position.h"
#include <atomic>

struct SearchLimits {
    int max_depth = 64;
    long movetime = 5000; // Default 5 seconds
    // Add other limits like nodes, wtime, btime etc. as needed
};

#include "types.h" // For SearchResult

struct ChessWizardOptions; // Forward declaration

// Main search function
SearchResult search_position(Position& pos, const SearchLimits& limits, const ChessWizardOptions* opts);

// Perft test function
uint64_t perft(int depth, Position& pos);

#endif // ENGINE_H
