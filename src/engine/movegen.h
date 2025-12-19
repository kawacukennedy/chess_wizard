#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"

// Forward declaration
class Position;

// Use fixed-size arrays for move lists as per specification.
// Do NOT allocate on heap during search.
const int MAX_MOVES_PER_PLY = 256; // Max pseudo-legal moves
const int MAX_CAPTURES_PER_PLY = 128; // Max captures
const int MAX_QUIETS_PER_PLY = 128; // Max quiet moves

// Main move generation function
// Fills the provided arrays with pseudo-legal moves for the given position.
// if `captures_only` is true, only generates captures and promotions.
void generate_moves(const Position& pos, Move* captures, int& num_captures, Move* quiets, int& num_quiets, bool captures_only = false);
void generate_moves(const Position& pos, Move* moves, int& num_moves, bool captures_only = false);
void generate_moves(const Position& pos, MoveList& moves, bool captures_only = false);
void generate_legal_moves(const Position& pos, MoveList& legal_moves);

#endif // MOVEGEN_H