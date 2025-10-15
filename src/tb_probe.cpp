#include "tb_probe.h"
#include "attack.h"
#include "movegen.h"

// Include Fathom library (assuming tbprobe.h and tbprobe.c are added to the project)
// extern "C" {
// #include "tbprobe.h"
// }

int get_material(const Position& pos, Color c) {
    int mat = 0;
    for (int p = WP; p <= BK; ++p) {
        if (get_piece_color((PieceType)p) == c) {
            mat += __builtin_popcountll(pos.piece_bitboards[p]) * SEE_VALUES[p];
        }
    }
    return mat;
}

// Convert Position to Fathom board structure
// Assuming Fathom uses a specific board representation
// This is a placeholder; actual conversion needed based on Fathom API

bool probe_syzygy(Position& pos, TBResult& result) {
    // Check if tablebases are available
    // if (!tb_init("/path/to/syzygy")) return false; // Set path from options

    // Convert position to Fathom format
    // unsigned int wdl = tb_probe_wdl(...);
    // if (wdl != TB_RESULT_FAILED) {
    //     result.score = (wdl == TB_WIN) ? 1000000 : (wdl == TB_LOSS) ? -1000000 : 0;
    //     // Get best move, dtz, etc.
    //     return true;
    // }

    // Fallback to simple cases
    int white_mat = get_material(pos, WHITE);
    int black_mat = get_material(pos, BLACK);

    // K vs K
    if (white_mat == 0 && black_mat == 0) {
        result.score = 0;
        result.dtz = 0;
        MoveList moves;
        generate_moves(pos, moves);
        if (!moves.empty()) result.best_move = moves[0];
        return true;
    }

    // KQ vs K
    if (white_mat == SEE_VALUES[WQ] && black_mat == 0) {
        result.score = 1000000;
        result.dtz = 1;
        MoveList moves;
        generate_moves(pos, moves);
        for (auto m : moves) {
            if (m.captured_piece() == BK) {
                result.best_move = m;
                return true;
            }
        }
        if (!moves.empty()) result.best_move = moves[0];
        return true;
    }

    // K vs KQ
    if (black_mat == SEE_VALUES[BQ] && white_mat == 0) {
        result.score = -1000000;
        result.dtz = 1;
        MoveList moves;
        generate_moves(pos, moves);
        for (auto m : moves) {
            if (m.captured_piece() == WK) {
                result.best_move = m;
                return true;
            }
        }
        if (!moves.empty()) result.best_move = moves[0];
        return true;
    }

    // KR vs K
    if (white_mat == SEE_VALUES[WR] && black_mat == 0) {
        result.score = 1000000;
        result.dtz = 1;
        MoveList moves;
        generate_moves(pos, moves);
        if (!moves.empty()) result.best_move = moves[0];
        return true;
    }

    // K vs KR
    if (black_mat == SEE_VALUES[BR] && white_mat == 0) {
        result.score = -1000000;
        result.dtz = 1;
        MoveList moves;
        generate_moves(pos, moves);
        if (!moves.empty()) result.best_move = moves[0];
        return true;
    }

    return false;
}
