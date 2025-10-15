#ifndef EVALUATE_H
#define EVALUATE_H

// Forward declaration
class Position;

// NNUE flag
extern bool USE_NNUE;

// Call this to enable/disable NNUE based on options
void set_use_nnue(bool use_nnue);

// Main evaluation function
int evaluate(const Position& pos);

#endif // EVALUATE_H