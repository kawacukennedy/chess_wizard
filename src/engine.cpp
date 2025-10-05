#include "engine.h"
#include "movegen.h"
#include "evaluate.h"
#include "tt.h"
#include "move.h"
#include "nnue.h"
#include "types.h"
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

// --- Search Globals ---
SearchLimits Limits;
Position RootPosition;
uint64_t NodeCount;
std::atomic<bool> StopSearch;

// --- PV Table ---
Move PV_TABLE[MAX_PLY][MAX_PLY];
int PV_LENGTH[MAX_PLY];

// --- Killer Moves ---
Move KILLER_MOVES[MAX_PLY][2];

// --- History Heuristic ---
int HISTORY_TABLE[12][64];

// --- Time Management ---
std::chrono::steady_clock::time_point start_time;

void check_time() {
    if ((NodeCount & 4095) == 0) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        if (elapsed >= Limits.movetime) {
            StopSearch = true;
        }
    }
}

void clear_search_globals() {
    NodeCount = 0;
    StopSearch = false;

    for (int i = 0; i < MAX_PLY; ++i) {
        PV_LENGTH[i] = 0;
        KILLER_MOVES[i][0] = Move(0);
        KILLER_MOVES[i][1] = Move(0);
    }

    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 64; ++j) {
            HISTORY_TABLE[i][j] = 0;
        }
    }
}

int score_move(Move move, int ply, Move tt_move) {
    if (move == tt_move) return 1 << 20;
    if (move.is_capture()) {
        return 100000 + (move.captured_piece() * 10) - move.moving_piece();
    }
    if (move == KILLER_MOVES[ply][0]) return 8000;
    if (move == KILLER_MOVES[ply][1]) return 7000;
    return HISTORY_TABLE[move.moving_piece()][move.to()];
}

void order_moves(MoveList& moves, int ply, Move tt_move) {
    std::sort(moves.begin(), moves.end(), [&](Move a, Move b) {
        return score_move(a, ply, tt_move) > score_move(b, ply, tt_move);
    });
}

int quiescence(int alpha, int beta, int ply, Position& pos) {
    NodeCount++;
    if (StopSearch) return 0;
    if (ply >= MAX_PLY) return evaluate(pos);

    int stand_pat = evaluate(pos);
    if (stand_pat >= beta) return beta;
    alpha = std::max(alpha, stand_pat);

    MoveList captures;
    generate_moves(pos, captures, true);
    Move tt_move = Move(0);
    order_moves(captures, ply, tt_move);

    for (const auto& move : captures) {
        pos.make_move(move);
        NNUE::nnue_evaluator.update_make(pos, move);
        int score = -quiescence(-beta, -alpha, ply + 1, pos);
        NNUE::nnue_evaluator.update_unmake(pos, move);
        pos.unmake_move(move);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int search(int alpha, int beta, int depth, int ply, Position& pos, bool do_null = true) {
    NodeCount++;
    check_time();
    if (StopSearch) return 0;

    PV_LENGTH[ply] = ply;

    bool in_check = pos.is_check();
    if (in_check) {
        depth++;
    }

    if (depth <= 0) {
        return quiescence(alpha, beta, ply, pos);
    }

    if (ply >= MAX_PLY) {
        return evaluate(pos);
    }

    alpha = std::max(alpha, -(1000000 - ply));
    beta = std::min(beta, 1000000 - ply);
    if (alpha >= beta) {
        return alpha;
    }

    Move tt_move = Move(0);
    TTEntry* tt_entry = TT.probe(pos.hash_key);
    if (tt_entry) {
        tt_move = Move(tt_entry->move);
        if (tt_entry->depth >= depth) {
            int score = tt_entry->score;
            if (score > 900000) score -= ply;
            if (score < -900000) score += ply;

            if (tt_entry->flags == TT_EXACT) return score;
            if (tt_entry->flags == TT_LOWER && score >= beta) return score;
            if (tt_entry->flags == TT_UPPER && score <= alpha) return score;
        }
    }

    int static_eval = -1;

    if (depth == 1 && !in_check) {
        static_eval = evaluate(pos);
        if (static_eval + 300 < alpha) {
            return static_eval;
        }
    }

    if (!in_check && do_null && depth >= 3) {
        pos.make_null_move();
        NNUE::nnue_evaluator.update_make_null();
        int null_reduction = (depth >= 6) ? 3 : 2;
        int null_score = -search(-beta, -beta + 1, depth - 1 - null_reduction, ply + 1, pos, false);
        NNUE::nnue_evaluator.update_unmake_null();
        pos.unmake_null_move();
        if (null_score >= beta) {
            return beta;
        }
    }

    MoveList moves;
    generate_moves(pos, moves);
    order_moves(moves, ply, tt_move);

    int moves_searched = 0;
    int best_score = -1000000;
    Move best_move = Move(0);
    uint8_t tt_flag = TT_UPPER;

    for (const auto& move : moves) {
        if (!in_check && depth <= 2 && !move.is_capture() && !move.is_promotion()) {
            if (static_eval == -1) static_eval = evaluate(pos);
            int futility_margin = 100 + 40 * depth;
            if (static_eval + futility_margin <= alpha) {
                continue;
            }
        }

        pos.make_move(move);
        NNUE::nnue_evaluator.update_make(pos, move);
        moves_searched++;
        int score;
        bool gives_check = pos.is_check();

        if (moves_searched == 1) {
            score = -search(-beta, -alpha, depth - 1 + (gives_check ? 1 : 0), ply + 1, pos);
        } else {
            int reduction = 0;
            if (depth >= 3 && moves_searched > 3 && !move.is_capture() && !in_check && !gives_check) {
                reduction = 1 + static_cast<int>(log2(depth) * log2(moves_searched) * 0.66);
            }

            score = -search(-alpha - 1, -alpha, depth - 1 - reduction + (gives_check ? 1 : 0), ply + 1, pos);
            if (score > alpha && score < beta) {
                score = -search(-beta, -alpha, depth - 1 + (gives_check ? 1 : 0), ply + 1, pos);
            }
        }
        NNUE::nnue_evaluator.update_unmake(pos, move);
        pos.unmake_move(move);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        if (best_score >= beta) {
            int store_score = best_score;
            if (store_score > 900000) store_score += ply;
            TT.store(pos.hash_key, move.value, store_score, depth, TT_LOWER);

            if (!move.is_capture()) {
                KILLER_MOVES[ply][1] = KILLER_MOVES[ply][0];
                KILLER_MOVES[ply][0] = move;
                HISTORY_TABLE[move.moving_piece()][move.to()] += depth * depth;
            }
            return beta;
        }

        if (best_score > alpha) {
            alpha = best_score;
            tt_flag = TT_EXACT;
            PV_TABLE[ply][ply] = move;
            for (int next_ply = ply + 1; next_ply < PV_LENGTH[ply + 1]; ++next_ply) {
                PV_TABLE[ply][next_ply] = PV_TABLE[ply + 1][next_ply];
            }
            PV_LENGTH[ply] = PV_LENGTH[ply + 1];
        }
    }

    if (moves_searched == 0) {
        return in_check ? -(1000000 - ply) : 0;
    }

    int store_score = best_score;
    if (store_score > 900000) store_score += ply;
    TT.store(pos.hash_key, best_move.value, store_score, depth, tt_flag);

    return alpha;
}

#include "book.h"

#include "syzygy.h"

extern Book OPENING_BOOK;

SearchResult search_position(Position& pos, const SearchLimits& limits, const ChessWizardOptions* opts) {
    Move book_move = OPENING_BOOK.get_move(pos.hash_key);
    if (book_move.value != 0) {
        SearchResult result = {};
        strncpy(result.best_move_uci, book_move.to_uci_string().c_str(), 7);
        result.best_move_uci[7] = '\0';
        result.info_flags = 1 << 2; // Set BOOK_MOVE flag
        return result;
    }

    TBResult tb_result;
    if (probe_syzygy(pos, tb_result)) {
        SearchResult result = {};
        strncpy(result.best_move_uci, tb_result.best_move.to_uci_string().c_str(), 7);
        result.best_move_uci[7] = '\0';
        result.score_cp = tb_result.score;
        result.info_flags = 1 << 1; // Set TB_OVERRIDE flag
        return result;
    }

    RootPosition = pos;
    Limits = limits;
    clear_search_globals();
    start_time = std::chrono::steady_clock::now();

    if (opts && opts->use_nnue) {
        if (NNUE::nnue_evaluator.init(opts->nnue_path)) {
            set_use_nnue(true);
        }
    }

    if (NNUE::nnue_available) {
        NNUE::nnue_evaluator.reset(pos);
    }

    int alpha = -1000000;
    int beta = 1000000;
    int score = 0;
    int last_completed_depth = 0;

    for (int current_depth = 1; current_depth <= Limits.max_depth; ++current_depth) {
        score = search(alpha, beta, current_depth, 0, pos);

        if (StopSearch && current_depth > 1) {
            break;
        }

        if (score <= alpha || score >= beta) {
            alpha = -1000000;
            beta = 1000000;
            score = search(alpha, beta, current_depth, 0, pos);
        }

        alpha = score - 80;
        beta = score + 80;
        last_completed_depth = current_depth;

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "info depth " << current_depth << " score cp " << score
                  << " nodes " << NodeCount << " nps " << (NodeCount * 1000 / (elapsed_ms + 1))
                  << " time " << elapsed_ms << " pv ";
        for (int i = 0; i < PV_LENGTH[0]; ++i) {
            std::cout << PV_TABLE[0][i].to_uci_string() << " ";
        }
        std::cout << std::endl;
    }

    SearchResult result = {};
    if (PV_LENGTH[0] > 0) {
        strncpy(result.best_move_uci, PV_TABLE[0][0].to_uci_string().c_str(), 7);
        result.best_move_uci[7] = '\0';

        std::string pv_str;
        for (int i = 0; i < PV_LENGTH[0]; ++i) {
            pv_str += PV_TABLE[0][i].to_uci_string() + " ";
        }
        strncpy(result.pv_uci, pv_str.c_str(), sizeof(result.pv_uci) - 1);
        result.pv_uci[sizeof(result.pv_uci) - 1] = '\0';
    }

    result.score_cp = score;
    result.depth = last_completed_depth;
    result.nodes = NodeCount;
    auto final_time = std::chrono::steady_clock::now();
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(final_time - start_time).count();
    
    result.win_prob = 0.0;
    result.info_flags = 0;

    return result;
}

uint64_t perft(int depth, Position& pos) {
    if (depth == 0) {
        return 1;
    }

    MoveList moves;
    generate_legal_moves(pos, moves);
    uint64_t nodes = 0;

    if (depth == 1) {
        return moves.size();
    }

    for (const auto& move : moves) {
        pos.make_move(move);
        nodes += perft(depth - 1, pos);
        pos.unmake_move(move);
    }

    return nodes;
}
