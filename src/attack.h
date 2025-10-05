#ifndef ATTACK_H
#define ATTACK_H

#include "bitboard.h"

// Forward declaration
class Position;

// --- Attack Table Declarations ---
extern Bitboard PAWN_ATTACKS[2][64];
extern Bitboard KNIGHT_ATTACKS[64];
extern Bitboard KING_ATTACKS[64];

// --- File & Rank Masks (for attack generation) ---
const Bitboard FILE_A_BB = 0x0101010101010101ULL;
const Bitboard FILE_H_BB = 0x8080808080808080ULL;
const Bitboard FILE_AB_BB = FILE_A_BB | (FILE_A_BB << 1);
const Bitboard FILE_GH_BB = FILE_H_BB | (FILE_H_BB >> 1);

// --- Initialization Function ---
void init_attacks();

// --- On-the-fly Attack Getters ---
Bitboard get_piece_attacks(PieceType pt, Square sq, Bitboard blockers);

// Sliding piece attack getters
Bitboard get_bishop_attacks(Square sq, Bitboard blockers);
Bitboard get_rook_attacks(Square sq, Bitboard blockers);
// --- Static Exchange Evaluation ---

// Piece values for SEE (centipawns)
extern const int SEE_VALUES[12];

int see(const Position& pos, Move move);

#endif // ATTACK_H
