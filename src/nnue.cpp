#include "nnue.h"
#include "move.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace NNUE {

Evaluator nnue_evaluator;
bool nnue_available = false;

// --- Feature Transformer ---
// Maps piece and square to a feature index as per Stockfish convention
// King presence is used to determine which 384-feature half to use.
int Evaluator::get_feature_index(PieceType pt, Square sq) {
    const int piece_offsets[] = {0, 0, 64, 128, 192, 256, 320}; // None, P, N, B, R, Q, K
    const int color_offset = (get_piece_color(pt) == BLACK) ? 384 : 0;
    GenericPieceType gpt = (pt == NO_PIECE) ? NO_GENERIC_PIECE : static_cast<GenericPieceType>(pt % 6);
    return color_offset + piece_offsets[gpt + 1] + sq;
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

    // Simplified loader based on spec
    // Real implementation would need more robust error checking
    net.feature_weights.resize(INPUT_SIZE * HIDDEN_SIZE);
    net.feature_bias.resize(HIDDEN_SIZE);
    net.output_weights.resize(HIDDEN_SIZE);

    file.read(reinterpret_cast<char*>(net.feature_weights.data()), INPUT_SIZE * HIDDEN_SIZE * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(net.feature_bias.data()), HIDDEN_SIZE * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(net.output_weights.data()), HIDDEN_SIZE * sizeof(int16_t));
    file.read(reinterpret_cast<char*>(&net.output_bias), sizeof(int16_t));

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
    acc.active[WHITE] = false;
    acc.active[BLACK] = false;

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

    PieceType moved_piece = move.moving_piece();
    PieceType captured_piece = move.captured_piece();
    Square from = move.from();
    Square to = move.to();

    // Remove moved piece from original square
    update_feature(get_feature_index(moved_piece, from), false);

    // Add moved piece to new square
    if (move.is_promotion()) {
        PieceType promoted_piece = promotion_val_to_piece_type(move.promotion(), get_piece_color(moved_piece));
        update_feature(get_feature_index(promoted_piece, to), true);
    } else {
        update_feature(get_feature_index(moved_piece, to), true);
    }

    // Remove captured piece
    if (captured_piece != NO_PIECE) {
        Square captured_sq = to;
        if (move.is_en_passant()) {
            captured_sq = (get_piece_color(moved_piece) == WHITE) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
        }
        update_feature(get_feature_index(captured_piece, captured_sq), false);
    }
}

void Evaluator::update_unmake(const Position& pos, Move move) {
    // This is just the reverse of update_make
    if (!initialized) return;

    PieceType moved_piece = move.moving_piece();
    PieceType captured_piece = move.captured_piece();
    Square from = move.from();
    Square to = move.to();

    // Add moved piece back to original square
    update_feature(get_feature_index(moved_piece, from), true);

    // Remove moved piece from new square
    if (move.is_promotion()) {
        PieceType promoted_piece = promotion_val_to_piece_type(move.promotion(), get_piece_color(moved_piece));
        update_feature(get_feature_index(promoted_piece, to), false);
    } else {
        update_feature(get_feature_index(moved_piece, to), false);
    }

    // Add captured piece back
    if (captured_piece != NO_PIECE) {
        Square captured_sq = to;
        if (move.is_en_passant()) {
            captured_sq = (get_piece_color(moved_piece) == WHITE) ? static_cast<Square>(to - 8) : static_cast<Square>(to + 8);
        }
        update_feature(get_feature_index(captured_piece, captured_sq), true);
    }
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
inline int16_t relu(int16_t x) {
    return std::max((int16_t)0, x);
}

int32_t Evaluator::evaluate(Color us) {
    if (!initialized) return 0;

    int32_t output = 0;
    for (int i = 0; i < HIDDEN_SIZE; ++i) {
        output += relu(acc.hidden[i]) * net.output_weights[i];
    }

    // The final score needs to be scaled and centered.
    // This is a simplified version. A real implementation would use constants from the model.
    output = (output / 16) + net.output_bias;

    return (us == WHITE) ? output : -output;
}

} // namespace NNUE