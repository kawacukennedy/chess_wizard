#include "syzygy.h"
#include "attack.h"
#include "movegen.h"

int get_material(const Position& pos, Color c) {
    int mat = 0;
    for (int p = WP; p <= BK; ++p) {
        if (get_piece_color((PieceType)p) == c) {
            mat += __builtin_popcountll(pos.piece_bitboards[p]) * SEE_VALUES[p];
        }
    }
    return mat;
}

bool probe_syzygy(Position& pos, TBResult& result) {
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
