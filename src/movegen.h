#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"
#include <vector>

// Forward declaration
class Position;

// Use a std::vector for the move list for simplicity.
// For performance, a fixed-size array or custom allocator could be used.
using MoveList = std::vector<Move>;

// Main move generation function
// Fills the provided MoveList with pseudo-legal moves for the given position.
// if `captures_only` is true, only generates captures and promotions.
void generate_moves(const Position& pos, MoveList& moves, bool captures_only = false);
void generate_legal_moves(const Position& pos, MoveList& legal_moves);

#endif // MOVEGEN_H