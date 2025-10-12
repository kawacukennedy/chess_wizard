#include "nnue.h"
#include "move.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <immintrin.h>

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

void Evaluator::update_make(Position& pos, Move move, StateInfo& si, int ply) {
    if (!initialized) return;

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

    // Handle castling rook moves
    if (move.is_castling()) {
        if (get_piece_color(moved_piece) == WHITE) {
            if (to == G1) {
                toggled.push_back(get_feature_index(WR, H1));
                toggled.push_back(get_feature_index(WR, F1));
            } else if (to == C1) {
                toggled.push_back(get_feature_index(WR, A1));
                toggled.push_back(get_feature_index(WR, D1));
            }
        } else {
            if (to == G8) {
                toggled.push_back(get_feature_index(BR, H8));
                toggled.push_back(get_feature_index(BR, F8));
            } else if (to == C8) {
                toggled.push_back(get_feature_index(BR, A8));
                toggled.push_back(get_feature_index(BR, D8));
            }
        }
    }

    // Update accumulator
    for (int f : toggled) {
        update_feature(f, true);
    }

    // Store in position's delta buffer
    int base = ply * 8;
    for (size_t i = 0; i < toggled.size(); ++i) {
        pos.nnue_delta_buffer[base + i] = toggled[i];
    }
    si.nnue_delta_count = toggled.size();
}

void Evaluator::update_unmake(Position& pos, Move move, StateInfo& si, int ply) {
    if (!initialized) return;

    // Undo the updates
    int base = ply * 8;
    for (uint8_t i = 0; i < si.nnue_delta_count; ++i) {
        update_feature(pos.nnue_delta_buffer[base + i], false);
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
inline int32_t relu(int32_t x) {
    return std::max(0, x);
}

int32_t Evaluator::evaluate(Color us) {
    if (!initialized) return 0;

    int32_t score = net.output_bias;
#ifdef __AVX2__
    __m256i sum_vec = _mm256_setzero_si256();
    for (int i = 0; i < HIDDEN_SIZE; i += 8) {
        __m256i hidden_vec = _mm256_load_si256((__m256i*)&acc.hidden[i]);
        // Apply ReLU: max(0, hidden)
        __m256i zero = _mm256_setzero_si256();
        hidden_vec = _mm256_max_epi32(hidden_vec, zero);
        __m256i weight_vec = _mm256_load_si256((__m256i*)&net.output_weights[i]);
        __m256i prod = _mm256_mullo_epi32(hidden_vec, weight_vec);
        sum_vec = _mm256_add_epi32(sum_vec, prod);
    }
    // Horizontal sum
    __m128i sum128 = _mm_add_epi32(_mm256_castsi256_si128(sum_vec), _mm256_extracti128_si256(sum_vec, 1));
    sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(2,3,0,1)));
    sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1,0,3,2)));
    score += _mm_cvtsi128_si32(sum128);
#else
    for (int i = 0; i < HIDDEN_SIZE; ++i) {
        int32_t hidden = std::max(0, acc.hidden[i]);
        score += hidden * net.output_weights[i];
    }
#endif

    // Scale by 16 as per common NNUE implementations
    score /= 16;

    return (us == WHITE) ? score : -score;
}

} // namespace NNUE