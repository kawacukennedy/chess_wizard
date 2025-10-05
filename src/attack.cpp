#include "attack.h"

// --- Precomputed Attack Tables ---
Bitboard PAWN_ATTACKS[2][64];
Bitboard KNIGHT_ATTACKS[64];
Bitboard KING_ATTACKS[64];
Bitboard BISHOP_ATTACKS[64];
Bitboard ROOK_ATTACKS[64];

// --- Magic Bitboard Tables (placeholder) ---

Bitboard mask_pawn_attacks(Color side, Square sq) {
    Bitboard attacks = 0;
    Bitboard b = 1ULL << sq;
    if (side == WHITE) {
        if ((b >> 7) & ~FILE_A_BB) attacks |= (b >> 7);
        if ((b >> 9) & ~FILE_H_BB) attacks |= (b >> 9);
    } else {
        if ((b << 7) & ~FILE_H_BB) attacks |= (b << 7);
        if ((b << 9) & ~FILE_A_BB) attacks |= (b << 9);
    }
    return attacks;
}

Bitboard mask_knight_attacks(Square sq) {
    Bitboard attacks = 0;
    Bitboard b = 1ULL << sq;
    if ((b >> 17) & ~FILE_H_BB) attacks |= (b >> 17);
    if ((b >> 15) & ~FILE_A_BB) attacks |= (b >> 15);
    if ((b >> 10) & ~FILE_GH_BB) attacks |= (b >> 10);
    if ((b >> 6) & ~FILE_AB_BB) attacks |= (b >> 6);
    if ((b << 17) & ~FILE_A_BB) attacks |= (b << 17);
    if ((b << 15) & ~FILE_H_BB) attacks |= (b << 15);
    if ((b << 10) & ~FILE_AB_BB) attacks |= (b << 10);
    if ((b << 6) & ~FILE_GH_BB) attacks |= (b << 6);
    return attacks;
}

Bitboard mask_king_attacks(Square sq) {
    Bitboard attacks = 0;
    Bitboard b = 1ULL << sq;
    if ((b >> 9) & ~FILE_H_BB) attacks |= (b >> 9);
    if ((b >> 8)) attacks |= (b >> 8);
    if ((b >> 7) & ~FILE_A_BB) attacks |= (b >> 7);
    if ((b >> 1) & ~FILE_H_BB) attacks |= (b >> 1);
    if ((b << 9) & ~FILE_A_BB) attacks |= (b << 9);
    if ((b << 8)) attacks |= (b << 8);
    if ((b << 7) & ~FILE_H_BB) attacks |= (b << 7);
    if ((b << 1) & ~FILE_A_BB) attacks |= (b << 1);
    return attacks;
}

// --- Blocker-based sliding piece attack generation ---

Bitboard get_bishop_attacks(Square sq, Bitboard blockers) {
    Bitboard attacks = 0;
    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    // Northeast
    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; ++r, ++f) {
        attacks |= (1ULL << (r * 8 + f));
        if (get_bit(blockers, (Square)(r * 8 + f))) break;
    }
    // Northwest
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; ++r, --f) {
        attacks |= (1ULL << (r * 8 + f));
        if (get_bit(blockers, (Square)(r * 8 + f))) break;
    }
    // Southeast
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; --r, ++f) {
        attacks |= (1ULL << (r * 8 + f));
        if (get_bit(blockers, (Square)(r * 8 + f))) break;
    }
    // Southwest
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; --r, --f) {
        attacks |= (1ULL << (r * 8 + f));
        if (get_bit(blockers, (Square)(r * 8 + f))) break;
    }
    return attacks;
}

Bitboard get_rook_attacks(Square sq, Bitboard blockers) {
    Bitboard attacks = 0;
    int r, f;
    int tr = sq / 8;
    int tf = sq % 8;

    // North
    for (r = tr + 1; r <= 7; ++r) {
        attacks |= (1ULL << (r * 8 + tf));
        if (get_bit(blockers, (Square)(r * 8 + tf))) break;
    }
    // South
    for (r = tr - 1; r >= 0; --r) {
        attacks |= (1ULL << (r * 8 + tf));
        if (get_bit(blockers, (Square)(r * 8 + tf))) break;
    }
    // East
    for (f = tf + 1; f <= 7; ++f) {
        attacks |= (1ULL << (tr * 8 + f));
        if (get_bit(blockers, (Square)(tr * 8 + f))) break;
    }
    // West
    for (f = tf - 1; f >= 0; --f) {
        attacks |= (1ULL << (tr * 8 + f));
        if (get_bit(blockers, (Square)(tr * 8 + f))) break;
    }
    return attacks;
}

Bitboard get_piece_attacks(PieceType pt, Square sq, Bitboard blockers) {
    switch (pt) {
        case WP: return PAWN_ATTACKS[WHITE][sq];
        case BP: return PAWN_ATTACKS[BLACK][sq];
        case WN: case BN: return KNIGHT_ATTACKS[sq];
        case WB: case BB: return get_bishop_attacks(sq, blockers);
        case WR: case BR: return get_rook_attacks(sq, blockers);
        case WQ: case BQ: return get_bishop_attacks(sq, blockers) | get_rook_attacks(sq, blockers);
        case WK: case BK: return KING_ATTACKS[sq];
        default: return 0;
    }
}


void init_attacks() {
    for (int sq = 0; sq < 64; ++sq) {
        PAWN_ATTACKS[WHITE][sq] = mask_pawn_attacks(WHITE, (Square)sq);
        PAWN_ATTACKS[BLACK][sq] = mask_pawn_attacks(BLACK, (Square)sq);
        KNIGHT_ATTACKS[sq] = mask_knight_attacks((Square)sq);
        KING_ATTACKS[sq] = mask_king_attacks((Square)sq);
    }
}