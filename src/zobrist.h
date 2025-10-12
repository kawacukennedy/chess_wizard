#pragma once

#include "types.h"
#include <array>
#include <random>

// Zobrist hashing keys
struct ZobristKeys {
    std::array<std::array<uint64_t, 64>, 12> piece_keys; // [piece][square]
    std::array<uint64_t, 16> castling_keys; // 16 possible castling rights combinations
    std::array<uint64_t, 8> en_passant_keys; // [file]
    uint64_t side_to_move_key; // XORed if black to move

    ZobristKeys(uint64_t seed = 0x9E3779B97F4A7C15ULL);

private:
    uint64_t generate_random_key();
};

extern ZobristKeys Zobrist;
extern uint64_t ZOBRIST_HISTORY[MAX_PLY];

void init_zobrist_keys(uint64_t seed = 0x9E3779B97F4A7C15ULL);
