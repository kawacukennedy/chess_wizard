#include "nnue.h"
#include "move.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace NNUE {

Evaluator nnue_evaluator;
bool nnue_available = false;

// --- Feature Transformer ---
// Simple halfkp: 12 pieces * 64 squares = 768 features
int Evaluator::get_feature_index(PieceType pt, Square sq) {
    return static_cast<int>(pt) * 64 + sq;
}

// --- Evaluator Class Implementation ---

Evaluator::Evaluator() : initialized(false) {}

bool Evaluator::init(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "info string NNUE: file not found: " << path << std::endl;
        initialized = false;
        nnue_available = false;
        return false;
    }

    // Read header as per spec
    char magic[9] = {0};
    file.read(magic, 8);
    if (std::string(magic) != "CWNNUEv1") {
        std::cout << "info string NNUE: invalid magic header." << std::endl;
        initialized = false;
        nnue_available = false;
        return false;
    }

    int32_t input_size, hidden_size, output_size;
    file.read(reinterpret_cast<char*>(&input_size), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&hidden_size), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&output_size), sizeof(int32_t));

    if (input_size != INPUT_SIZE || hidden_size != HIDDEN_SIZE || output_size != 1) {
        std::cout << "info string NNUE: network size mismatch." << std::endl;
        initialized = false;
        nnue_available = false;
        return false;
    }

    // Skip quant_params for now (assume int16)
    int32_t quant_params;
    file.read(reinterpret_cast<char*>(&quant_params), sizeof(int32_t));

    // Read weights as int16_t, convert to int32_t
    std::vector<int16_t> temp_feature_weights(INPUT_SIZE * HIDDEN_SIZE);
    std::vector<int16_t> temp_feature_bias(HIDDEN_SIZE);
    std::vector<int16_t> temp_output_weights(HIDDEN_SIZE);
    int16_t temp_output_bias;

    file.read(reinterpret_cast<char*>(temp_feature_weights.data()), INPUT_SIZE * HIDDEN_SIZE * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(temp_feature_bias.data()), HIDDEN_SIZE * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(temp_output_weights.data()), HIDDEN_SIZE * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(&temp_output_bias), sizeof(int16_t));

    net.feature_weights.assign(temp_feature_weights.begin(), temp_feature_weights.end());
    net.feature_bias.assign(temp_feature_bias.begin(), temp_feature_bias.end());
    net.output_weights.assign(temp_output_weights.begin(), temp_output_weights.end());
    net.output_bias = temp_output_bias;

    // Skip checksum for now
    uint32_t checksum;
    file.read(reinterpret_cast<char*>(&checksum), sizeof(uint32_t));

    if (!file) {
        std::cout << "info string NNUE: error reading file." << std::endl;
        initialized = false;
        nnue_available = false;
        return false;
    }

    initialized = true;
    nnue_available = true;
    std::cout << "info string NNUE: network loaded successfully." << std::endl;
    return true;
}

void Evaluator::reset(const Position& pos) {
    if (!initialized) return;

    acc.hidden.assign(net.feature_bias.begin(), net.feature_bias.end());

    for (int pt_idx = 0; pt_idx < 12; ++pt_idx) {
        Bitboard bb = pos.piece_bitboards[pt_idx];
        while (bb) {
            Square sq = pop_bit(bb);
            update_feature(get_feature_index(static_cast<PieceType>(pt_idx), sq), true);
        }
    }
}

void Evaluator::update_feature(int feature_index, bool add) {
    if (add) {
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.hidden[i] += net.feature_weights[feature_index * HIDDEN_SIZE + i];
        }
    } else {
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.hidden[i] -= net.feature_weights[feature_index * HIDDEN_SIZE + i];
        }
    }
}

void Evaluator::update_make(const Position& pos, Move move) {
    if (!initialized) return;

    acc.deltas.clear();
    std::vector<int> toggled;

    PieceType moved_piece = move.moving_piece();
    PieceType captured_piece = move.captured_piece();
    Square from = move.from();
    Square to = move.to();

    // Collect toggled features
    // Remove moved piece from original square
    toggled.push_back(get_feature_index(moved_piece, from));

    // Remove captured piece
    if (captured_piece != NO_PIECE) {
        Square captured_sq = to;
        if (move.is_en_passant()) {
            captured_sq = (get_piece_color(moved_piece) == WHITE) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
        }
        toggled.push_back(get_feature_index(captured_piece, captured_sq));
    }

    // Add moved piece to new square
    if (move.is_promotion()) {
        PieceType promoted_piece = promotion_val_to_piece_type(move.promotion(), get_piece_color(moved_piece));
        toggled.push_back(get_feature_index(promoted_piece, to));
    } else {
        toggled.push_back(get_feature_index(moved_piece, to));
    }

    // Update accumulator
    for (int f : toggled) {
        update_feature(f, true);
    }

    acc.deltas = toggled;
}

void Evaluator::update_unmake(const Position& pos, Move move) {
    if (!initialized) return;

    // Undo the updates
    for (int f : acc.deltas) {
        update_feature(f, false);
    }
    acc.deltas.clear();
}

void Evaluator::update_make_null() {
    if (!initialized) return;
    // Stub: A real implementation would need to handle the side-to-move change efficiently.
}

void Evaluator::update_unmake_null() {
    if (!initialized) return;
    // Stub: A real implementation would need to handle the side-to-move change efficiently.
}

// ReLU activation function
inline int32_t relu(int32_t x) {
    return std::max(0, x);
}

int32_t Evaluator::evaluate(Color us) {
    if (!initialized) return 0;

    int32_t score = net.output_bias;
    for (int i = 0; i < HIDDEN_SIZE; ++i) {
        int32_t hidden = std::max(0, acc.hidden[i]);
        score += hidden * net.output_weights[i];
    }

    // Scale by 16 as per common NNUE implementations
    score /= 16;

    return (us == WHITE) ? score : -score;
}

} // namespace NNUE