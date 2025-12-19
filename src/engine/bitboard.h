#pragma once

#include "types.h"

// Define a bitboard as a 64-bit unsigned integer
typedef uint64_t Bitboard;

// Bitboard manipulation functions

// Get bit from bitboard
inline bool get_bit(Bitboard bb, Square sq) {
    return (bb >> sq) & 1;
}

// Set bit on bitboard
inline void set_bit(Bitboard& bb, Square sq) {
    bb |= (1ULL << sq);
}

// Clear bit on bitboard
inline void clear_bit(Bitboard& bb, Square sq) {
    bb &= ~(1ULL << sq);
}

// Pop bit (clear and return square)
inline int lsb_index(Bitboard bb) {
    return __builtin_ctzll(bb);
}

// Count bits in bitboard
inline int popcnt(Bitboard bb) {
    return __builtin_popcountll(bb);
}

// Check if a number is a power of two
inline bool is_power_of_two(Bitboard bb) {
    return bb && !(bb & (bb - 1));
}

// Pop bit (clear and return square)
inline Square pop_bit(Bitboard& bb) {
    Square sq = (Square)lsb_index(bb); // Count trailing zeros
    clear_bit(bb, sq);
    return sq;
}

// Get file from square
inline File get_file(Square sq) {
    return (File)(sq % 8);
}

// Get rank from square
inline Rank get_rank(Square sq) {
    return (Rank)(sq / 8);
}

// Bitmasks for files and ranks
extern Bitboard FILE_MASKS[8];
extern Bitboard RANK_MASKS[8];

// Print bitboard (for debugging)
void print_bitboard(Bitboard bb);

// Function to initialize bitboard masks
void init_bitboard_masks();
