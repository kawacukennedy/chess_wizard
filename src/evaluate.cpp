#include "evaluate.h"
#include "board.h"
#include "bitboard.h"
#include "nnue.h"
#include "attack.h"

// --- Global flag for NNUE ---
static bool USE_NNUE = false;

void set_use_nnue(bool use_nnue) {
    USE_NNUE = use_nnue && NNUE::nnue_available;
}

// --- Helper functions ---
inline Bitboard file_mask(int file) { return FILE_MASKS[file]; }

Bitboard rank_mask_above(int rank) {
    Bitboard mask = 0;
    for (int r = rank + 1; r < 8; ++r) mask |= RANK_MASKS[r];
    return mask;
}

Bitboard rank_mask_below(int rank) {
    Bitboard mask = 0;
    for (int r = 0; r < rank; ++r) mask |= RANK_MASKS[r];
    return mask;
}

// --- Material Values (centipawns) ---
const int MATERIAL[12] = {
    100, 320, 330, 500, 900, 20000, // White
   -100,-320,-330,-500,-900,-20000  // Black
};

// --- Piece-Square Tables (Middlegame & Endgame) ---
// Mirrored for black pieces

// Pawns
const int PST_PAWN_MG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int PST_PAWN_EG[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    10, 10,  0,-10,-10,  0, 10, 10,
     5,  0,-10,  0,  0,-10,  0,  5,
     0,  0, 10, 20, 20, 10,  0,  0,
     5,  5,  5, 15, 15,  5,  5,  5,
    20, 20, 20, 30, 30, 20, 20, 20,
    80, 80, 80, 80, 80, 80, 80, 80,
     0,  0,  0,  0,  0,  0,  0,  0
};

// Knights
const int PST_KNIGHT_MG[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
const int PST_KNIGHT_EG[64] = {
    -20,-10, -5, -5, -5, -5,-10,-20,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  5, 10, 10, 10, 10,  5, -5,
     -5,  5, 10, 15, 15, 10,  5, -5,
     -5,  5, 10, 15, 15, 10,  5, -5,
     -5,  5, 10, 10, 10, 10,  5, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -20,-10, -5, -5, -5, -5,-10,-20
};

// Bishops
const int PST_BISHOP_MG[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
const int PST_BISHOP_EG[64] = {
    -10, -5, -5, -5, -5, -5, -5,-10,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  5,  5,  5,  5,  5,  5, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
     -5,  5,  5,  5,  5,  5,  5, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
    -10, -5, -5, -5, -5, -5, -5,-10
};

// Rooks
const int PST_ROOK_MG[64] = {
      0,  0,  0,  5,  5,  0,  0,  0,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      5, 10, 10, 10, 10, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};
const int PST_ROOK_EG[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0
};

// Queens
const int PST_QUEEN_MG[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
const int PST_QUEEN_EG[64] = {
    -10, -5, -5, -5, -5, -5, -5,-10,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
    -10, -5, -5, -5, -5, -5, -5,-10
};

// King
const int PST_KING_MG[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};
const int PST_KING_EG[64] = {
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

// --- PST Access ---
const int* PST_MG[] = { PST_PAWN_MG, PST_KNIGHT_MG, PST_BISHOP_MG, PST_ROOK_MG, PST_QUEEN_MG, PST_KING_MG };
const int* PST_EG[] = { PST_PAWN_EG, PST_KNIGHT_EG, PST_BISHOP_EG, PST_ROOK_EG, PST_QUEEN_EG, PST_KING_EG };

// --- Game Phase ---
const int PHASE_VALUES[] = {0, 1, 1, 2, 4, 0}; // Pawn, Knight, Bishop, Rook, Queen, King
const int TOTAL_PHASE = 24; // Sum of phase values for all pieces at start

int get_game_phase(const Position& pos) {
    int phase = TOTAL_PHASE;
    for (int pt = WP; pt <= BK; ++pt) {
        Bitboard bb = pos.piece_bitboards[pt];
        phase -= popcnt(bb) * PHASE_VALUES[pt % 6];
    }
    return (phase * 256 + (TOTAL_PHASE / 2)) / TOTAL_PHASE;
}

// --- Pawn Structure Evaluation ---
int evaluate_pawn_structure(const Position& pos, Color color) {
    int score = 0;
    Bitboard pawns = pos.piece_bitboards[color == WHITE ? WP : BP];
    Bitboard enemy_pawns = pos.piece_bitboards[color == WHITE ? BP : WP];
    Bitboard our_pawns = pawns;

    while (pawns) {
        Square sq = pop_bit(pawns);
        int file = sq % 8;
        int rank = sq / 8;
        int relative_rank = color == WHITE ? rank : 7 - rank;

        // Isolated pawn
        bool isolated = true;
        if (file > 0 && (our_pawns & file_mask(file - 1))) isolated = false;
        if (file < 7 && (our_pawns & file_mask(file + 1))) isolated = false;
        if (isolated) score -= 15;

        // Doubled pawn
        Bitboard file_bb = file_mask(file);
        int pawns_on_file = popcnt(our_pawns & file_bb);
        if (pawns_on_file > 1) score -= 25;

        // Passed pawn
        bool passed = true;
        Bitboard blocking_squares = (color == WHITE ? rank_mask_above(rank) : rank_mask_below(rank)) & (file_mask(std::max(0, file-1)) | file_mask(file) | file_mask(std::min(7, file+1)));
        if (enemy_pawns & blocking_squares) passed = false;
        if (passed) {
            score += 20 + 10 * relative_rank;
        }
    }
    return score;
}

// --- King Safety Evaluation ---
int evaluate_king_safety(const Position& pos, Color color) {
    Square king_sq = get_king_square(pos, color);
    int score = 0;

    // Pawn shield
    Bitboard pawn_shield = 0ULL;
    int king_file = king_sq % 8;
    int king_rank = king_sq / 8;
    if (color == WHITE) {
        if (king_rank < 7) {
            for (int f = std::max(0, king_file - 1); f <= std::min(7, king_file + 1); ++f) {
                pawn_shield |= (1ULL << ((king_rank + 1) * 8 + f));
            }
        }
    } else {
        if (king_rank > 0) {
            for (int f = std::max(0, king_file - 1); f <= std::min(7, king_file + 1); ++f) {
                pawn_shield |= (1ULL << ((king_rank - 1) * 8 + f));
            }
        }
    }
    Bitboard our_pawns = pos.piece_bitboards[color == WHITE ? WP : BP];
    int shield_pawns = popcnt(pawn_shield & our_pawns);
    score += shield_pawns * -20; // More pawns better (less negative)

    // Attacker count
    int attackers = 0;
    Color enemy = color == WHITE ? BLACK : WHITE;
    for (int pt = WN; pt <= WQ; ++pt) {
        Bitboard pieces = pos.piece_bitboards[enemy * 6 + pt - WP];
        while (pieces) {
            Square sq = pop_bit(pieces);
            if (pos.is_square_attacked(king_sq, enemy)) {
                attackers++;
                break; // Just count if any attack, but wait, no, count number of attackers.
                // Actually, to count distinct attackers, but for simplicity, count pieces that attack.
            }
        }
    }
    // Since is_square_attacked is bool, I need to count how many pieces attack.
    // Better to loop and check for each piece if it attacks king_sq.
    attackers = 0;
    for (int pt = WN; pt <= WQ; ++pt) {
        Bitboard pieces = pos.piece_bitboards[enemy * 6 + pt - WP];
        while (pieces) {
            Square sq = pop_bit(pieces);
            // Check if this piece attacks king_sq
            Bitboard attacks = 0ULL;
            switch (pt) {
                case WN: attacks = KNIGHT_ATTACKS[sq]; break;
                case WB: attacks = get_bishop_attacks(sq, pos.occAll); break;
                case WR: attacks = get_rook_attacks(sq, pos.occAll); break;
                case WQ: attacks = (get_bishop_attacks(sq, pos.occAll) | get_rook_attacks(sq, pos.occAll)); break;
            }
            if (attacks & (1ULL << king_sq)) attackers++;
        }
    }
    // Buckets
    const int attacker_penalty[] = {0, -10, -25, -60, -120};
    int bucket = std::min(attackers, 4);
    score += attacker_penalty[bucket];

    return score;
}

// --- Mobility Evaluation ---
int evaluate_mobility(const Position& pos, Color color) {
    int score = 0;
    const int mobility_weights[] = {0, 4, 3, 2, 1}; // P, N, B, R, Q

    for (int pt = WN; pt <= WQ; ++pt) {
        Bitboard pieces = pos.piece_bitboards[color * 6 + pt - WP];
        while (pieces) {
            Square sq = pop_bit(pieces);
            int moves = 0;
            // Count pseudo-legal moves (simplified)
            Bitboard attacks = 0ULL;
            switch (pt) {
                case WN: attacks = KNIGHT_ATTACKS[sq]; break;
                case WB: attacks = get_bishop_attacks(sq, pos.occAll); break;
                case WR: attacks = get_rook_attacks(sq, pos.occAll); break;
                case WQ: attacks = (get_bishop_attacks(sq, pos.occAll) | get_rook_attacks(sq, pos.occAll)); break;
            }
            attacks &= ~(color == WHITE ? pos.occWhite : pos.occBlack);
            moves = popcnt(attacks);
            score += moves * mobility_weights[pt - WN + 1];
        }
    }
    return score;
}

// --- Evaluation Function ---
int evaluate(const Position& pos) {
    if (USE_NNUE) {
        return NNUE::nnue_evaluator.evaluate(pos.side_to_move);
    }

    // --- Classical Evaluation Fallback ---
    int score_mg = 0;
    int score_eg = 0;
    int phase = get_game_phase(pos);

    // --- Material and PST ---
    for (int pt_idx = 0; pt_idx < 12; ++pt_idx) {
        Bitboard bb = pos.piece_bitboards[pt_idx];
        int material_value = (pt_idx < 6) ? MATERIAL[pt_idx] : -MATERIAL[pt_idx % 6];
        
        while (bb) {
            Square sq = pop_bit(bb);
            score_mg += material_value;
            score_eg += material_value;

            if (pt_idx < 6) { // White
                score_mg += PST_MG[pt_idx][sq];
                score_eg += PST_EG[pt_idx][sq];
            } else { // Black
                score_mg -= PST_MG[pt_idx % 6][sq ^ 56]; // Flip square for black
                score_eg -= PST_EG[pt_idx % 6][sq ^ 56];
            }
        }
    }

    // --- Pawn Structure ---
    score_mg += evaluate_pawn_structure(pos, WHITE);
    score_eg += evaluate_pawn_structure(pos, WHITE);
    score_mg -= evaluate_pawn_structure(pos, BLACK);
    score_eg -= evaluate_pawn_structure(pos, BLACK);

    // --- King Safety ---
    score_mg += evaluate_king_safety(pos, WHITE);
    score_eg += evaluate_king_safety(pos, WHITE);
    score_mg -= evaluate_king_safety(pos, BLACK);
    score_eg -= evaluate_king_safety(pos, BLACK);

    // --- Mobility ---
    score_mg += evaluate_mobility(pos, WHITE);
    score_eg += evaluate_mobility(pos, WHITE);
    score_mg -= evaluate_mobility(pos, BLACK);
    score_eg -= evaluate_mobility(pos, BLACK);

    // --- Interpolate based on game phase ---
    int final_score = ((score_mg * (256 - phase)) + (score_eg * phase)) / 256;

    // --- Tempo Bonus ---
    final_score += (pos.side_to_move == WHITE) ? 10 : -10;

    return (pos.side_to_move == WHITE) ? final_score : -final_score;
}