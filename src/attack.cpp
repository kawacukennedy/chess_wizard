#include "attack.h"
#include "position.h"
#include "types.h"

// --- Precomputed Attack Tables ---
Bitboard PAWN_ATTACKS[2][64];
Bitboard KNIGHT_ATTACKS[64];
Bitboard KING_ATTACKS[64];
Bitboard BISHOP_ATTACKS[64];
Bitboard ROOK_ATTACKS[64];

// --- Magic Bitboard Tables (placeholder) ---

Bitboard mask_pawn_attacks(Color side, Square sq) {
    Bitboard attacks = 0;
    if (side == WHITE) {
        if ((sq % 8) != 0) attacks |= (1ULL << (sq + 7));
        if ((sq % 8) != 7) attacks |= (1ULL << (sq + 9));
    } else {
        if ((sq % 8) != 0) attacks |= (1ULL << (sq - 9));
        if ((sq % 8) != 7) attacks |= (1ULL << (sq - 7));
    }
    return attacks;
}

Bitboard mask_knight_attacks(Square sq) {
    Bitboard attacks = 0;
    Bitboard b = 1ULL << sq;
    if (sq >= 17 && (b >> 17) & ~FILE_H_BB) attacks |= (b >> 17);
    if (sq >= 15 && (b >> 15) & ~FILE_A_BB) attacks |= (b >> 15);
    if (sq >= 10 && (b >> 10) & ~FILE_GH_BB) attacks |= (b >> 10);
    if (sq >= 6 && (b >> 6) & ~FILE_AB_BB) attacks |= (b >> 6);
    if (sq <= 46 && (b << 17) & ~FILE_A_BB) attacks |= (b << 17);
    if (sq <= 48 && (b << 15) & ~FILE_H_BB) attacks |= (b << 15);
    if (sq <= 53 && (b << 10) & ~FILE_AB_BB) attacks |= (b << 10);
    if (sq <= 57 && (b << 6) & ~FILE_GH_BB) attacks |= (b << 6);
    return attacks;
}

Bitboard mask_king_attacks(Square sq) {
    Bitboard attacks = 0;
    Bitboard b = 1ULL << sq;
    if (sq >= 9 && (b >> 9) & ~FILE_H_BB) attacks |= (b >> 9);
    if (sq >= 8 && (b >> 8)) attacks |= (b >> 8);
    if (sq >= 7 && (b >> 7) & ~FILE_A_BB) attacks |= (b >> 7);
    if (sq >= 1 && (b >> 1) & ~FILE_H_BB) attacks |= (b >> 1);
    if (sq <= 54 && (b << 9) & ~FILE_A_BB) attacks |= (b << 9);
    if (sq <= 55 && (b << 8)) attacks |= (b << 8);
    if (sq <= 56 && (b << 7) & ~FILE_H_BB) attacks |= (b << 7);
    if (sq <= 62 && (b << 1) & ~FILE_A_BB) attacks |= (b << 1);
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
        Square s = (Square)(r * 8 + f);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
    }
    // Northwest
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; ++r, --f) {
        Square s = (Square)(r * 8 + f);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
    }
    // Southeast
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; --r, ++f) {
        Square s = (Square)(r * 8 + f);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
    }
    // Southwest
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; --r, --f) {
        Square s = (Square)(r * 8 + f);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
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
        Square s = (Square)(r * 8 + tf);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
    }
    // South
    for (r = tr - 1; r >= 0; --r) {
        Square s = (Square)(r * 8 + tf);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
    }
    // East
    for (f = tf + 1; f <= 7; ++f) {
        Square s = (Square)(tr * 8 + f);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
    }
    // West
    for (f = tf - 1; f >= 0; --f) {
        Square s = (Square)(tr * 8 + f);
        attacks |= (1ULL << s);
        if (get_bit(blockers, s)) break;
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

// Piece values for SEE (centipawns)
const int SEE_VALUES[12] = {100, 320, 330, 500, 900, 0, 100, 320, 330, 500, 900, 0};

int see(const Position& pos, Move move) {
    Square from = move.from();
    Square to = move.to();
    PieceType moving = move.moving_piece();
    PieceType captured = move.captured_piece();

    int gain[32];
    int d = 0;
    Bitboard may_xray = pos.piece_bitboards[WB] | pos.piece_bitboards[BB] | pos.piece_bitboards[WR] | pos.piece_bitboards[BR] | pos.piece_bitboards[WQ] | pos.piece_bitboards[BQ];
    Bitboard from_bb = 1ULL << from;
    Bitboard occ = pos.occupancy_bitboards[BOTH];
    Bitboard attackers = 0;

    // Initial capture
    gain[d] = SEE_VALUES[captured];
    d++;

    // Find all attackers
    attackers |= (PAWN_ATTACKS[WHITE][to] & pos.piece_bitboards[BP]) | (PAWN_ATTACKS[BLACK][to] & pos.piece_bitboards[WP]);
    attackers |= (KNIGHT_ATTACKS[to] & (pos.piece_bitboards[WN] | pos.piece_bitboards[BN]));
    attackers |= (KING_ATTACKS[to] & (pos.piece_bitboards[WK] | pos.piece_bitboards[BK]));

    Bitboard bishops = pos.piece_bitboards[WB] | pos.piece_bitboards[BB];
    Bitboard rooks = pos.piece_bitboards[WR] | pos.piece_bitboards[BR];
    Bitboard queens = pos.piece_bitboards[WQ] | pos.piece_bitboards[BQ];

    if (bishops | queens) attackers |= get_bishop_attacks(to, occ) & (bishops | queens);
    if (rooks | queens) attackers |= get_rook_attacks(to, occ) & (rooks | queens);

    // Remove the moving piece if it's still on from (but since we're simulating, assume it's moved)
    attackers &= ~from_bb;

    Color side = (moving >= BP) ? BLACK : WHITE;

    occ ^= from_bb; // Remove moving piece
    if (captured != NO_PIECE) occ ^= (1ULL << to); // Remove captured piece

    while (attackers) {
        // Find least valuable attacker
        PieceType pt = NO_PIECE;
        Bitboard attacker_bb = 0;
        int min_value = 10000;

        for (int p = WP; p <= BK; ++p) {
            Bitboard bb = attackers & pos.piece_bitboards[p];
            if (bb) {
                int val = SEE_VALUES[p];
                if (val < min_value) {
                    min_value = val;
                    pt = (PieceType)p;
                    attacker_bb = bb & -bb; // LSB
                }
            }
        }

        if (pt == NO_PIECE) break;

        // Add capture value
        gain[d] = SEE_VALUES[pt] - gain[d - 1];
        d++;

        // Update occupancy
        occ ^= attacker_bb;

        // Add discovered attacks
        if (may_xray & attacker_bb) {
            Bitboard xray_attackers = 0;
            if (bishops | queens) xray_attackers |= get_bishop_attacks(to, occ) & (bishops | queens);
            if (rooks | queens) xray_attackers |= get_rook_attacks(to, occ) & (rooks | queens);
            attackers |= xray_attackers;
        }

        attackers &= ~attacker_bb;
        attackers &= occ; // Only pieces still on board

        side = (side == WHITE) ? BLACK : WHITE;
    }

    // Negamax the gains
    while (--d) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }

    return gain[0];
}