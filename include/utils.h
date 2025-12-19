#ifndef UTILS_H
#define UTILS_H

#include "common.h"

inline int lsb_index(Bitboard b) { return __builtin_ctzll(b); }
inline int popcount(Bitboard b) { return __builtin_popcountll(b); }
inline void set_bit(Bitboard& b, Square sq) { b |= (1ULL << sq); }
inline void clear_bit(Bitboard& b, Square sq) { b &= ~(1ULL << sq); }
inline bool get_bit(Bitboard b, Square sq) { return b & (1ULL << sq); }

#endif