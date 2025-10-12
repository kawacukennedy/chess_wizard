#pragma once

#include "types.h"
#include "board.h"
#include <vector>

namespace NNUE {

// As per spec: 768 inputs, 256 neurons in hidden layer
const int INPUT_SIZE = 768;
const int HIDDEN_SIZE = 256;

// NNUE Network Weights and Biases
struct Network {
    // Input to hidden layer
    std::vector<int32_t> feature_weights; // size = INPUT_SIZE * HIDDEN_SIZE
    std::vector<int32_t> feature_bias;    // size = HIDDEN_SIZE

    // Hidden to output layer
    std::vector<int32_t> output_weights;  // size = HIDDEN_SIZE
    int32_t output_bias;
};

// Accumulator for incremental updates
struct Accumulator {
    std::vector<int32_t> hidden; // size = HIDDEN_SIZE
    std::vector<int> deltas; // for nnue_delta_indices
};

// Main NNUE evaluation class
class Evaluator {
public:
    Evaluator();
    bool init(const char* path);
    void reset(const Position& pos);
    void update_make(Position& pos, Move move, StateInfo& si, int ply);
    void update_unmake(Position& pos, Move move, StateInfo& si, int ply);
    void update_make_null();
    void update_unmake_null();
    int32_t evaluate(Color us);

private:
    Network net;
    Accumulator acc;
    bool initialized = false;

    void update_feature(int feature_index, bool add);
    int get_feature_index(PieceType pt, Square sq);
};

// Global NNUE evaluator instance
extern Evaluator nnue_evaluator;
extern bool nnue_available;

} // namespace NNUE