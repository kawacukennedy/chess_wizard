#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include "tt.h"
#include "move.h"
#include "nnue.h"
#include "types.h"
#include "attack.h"
#include "board.h"
#include "book.h"
#include "tb_probe.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <vector>
#include <atomic>

extern bool USE_NNUE;

extern Book OPENING_BOOK;

// --- Search Globals ---
SearchLimits Limits;
Position RootPosition;
uint64_t NodeCount;
std::atomic<bool> StopSearch;

// --- Win Probability Calibration ---
const double WIN_PROB_K = 0.0045;
const double WIN_PROB_OFFSET = 0.0;

// --- Mate Score ---
const int MATE_VALUE = 1000000;

// --- Move Ordering Constants ---
const int TT_HINT = 1000000;
const int CAP_BASE = 100000;
const int MVV_MULT = 10;
const int SEE_WIN_BONUS = 500;
const int SEE_LOSE_PENALTY = -1200;
const int PROM_BASE = 90000;
const int KILLER1 = 8000;
const int KILLER2 = 7000;
const int HISTORY_MAX = 1 << 28;

// --- PV Table ---
Move PV_TABLE[MAX_PLY + 1][MAX_PLY + 1];
int PV_LENGTH[MAX_PLY + 1];

// --- Killer Moves ---
Move KILLER_MOVES[MAX_PLY + 1][2];

// --- History Heuristic ---
int HISTORY_TABLE[12][64];

// --- Move Lists per Ply ---
Move CAPTURES[MAX_PLY][128];
Move QUIETS[MAX_PLY][128];
int NUM_CAPTURES[MAX_PLY];
int NUM_QUIETS[MAX_PLY];

// --- Time Management ---
std::chrono::steady_clock::time_point start_time;

// --- Monte Carlo Rollout ---
std::tuple<int, int, int> rollout(Position pos, int max_depth) {
    int ply = 0;
    Move moves[256];
    int num_moves = 0;
    while (ply < max_depth) {
        num_moves = 0;
        generate_moves(pos, moves, num_moves);
        if (num_moves == 0) {
            if (pos.is_check()) {
                return {pos.side_to_move == BLACK ? 1 : 0, pos.side_to_move == WHITE ? 1 : 0, 0};
            } else {
                return {0, 0, 1};
            }
        }
        // NNUE-based policy: softmax over evaluation scores
        int evals[256];
        for (int i = 0; i < num_moves; ++i) {
            Move m = moves[i];
            if (pos.make_move(m)) {
                int eval = evaluate(pos);
                pos.unmake_move(m);
                evals[i] = eval;
            } else {
                evals[i] = -MATE_VALUE;
            }
        }
        int max_eval = evals[0];
        for (int i = 1; i < num_moves; ++i) {
            if (evals[i] > max_eval) max_eval = evals[i];
        }
        double probs[256];
        double sum = 0.0;
        for (int i = 0; i < num_moves; ++i) {
            double prob = exp((evals[i] - max_eval) / 100.0); // temperature 100 cp
            probs[i] = prob;
            sum += prob;
        }
        for (int i = 0; i < num_moves; ++i) probs[i] /= sum;
        double r = (double)rand() / RAND_MAX;
        double cum = 0.0;
        int idx = 0;
        for (int i = 0; i < num_moves; ++i) {
            cum += probs[i];
            if (r <= cum) {
                idx = i;
                break;
            }
        }
        pos.make_move(moves[idx]);
        ply++;
    }
    int eval = evaluate(pos);
    if (eval > 0) return {1, 0, 0};
    else if (eval < 0) return {0, 1, 0};
    else return {0, 0, 1};
}

// --- Draw Detection ---
bool is_draw(const Position& pos) {
    // 50-move rule
    if (pos.halfmove_clock >= 100) return true;

    // Insufficient material
    int piece_count[12] = {0};
    for (int p = WP; p <= BK; ++p) {
        piece_count[p] = __builtin_popcountll(pos.piece_bitboards[p]);
    }
    int total_pieces = 0;
    for (int p = 0; p < 12; ++p) total_pieces += piece_count[p];

    // King vs king
    if (total_pieces == 2) return true;

    // King + minor vs king
    if (total_pieces == 3) {
        if (piece_count[WN] || piece_count[WB] || piece_count[BN] || piece_count[BB]) return true;
    }

    // King + two minors vs king (but not same color bishops)
    if (total_pieces == 4) {
        int minors = piece_count[WN] + piece_count[WB] + piece_count[BN] + piece_count[BB];
        if (minors == 2) {
            // Check if bishops are same color
            if (piece_count[WB] == 2 || piece_count[BB] == 2) return false; // Different colors
            return true;
        }
    }

    return false;
}

// --- Win Probability ---
double sigmoid_win_prob(int cp_score) {
    if (cp_score >= MATE_VALUE - MAX_PLY) {
        return 1.0 - 1e-12; // Side to move mates
    } else if (cp_score <= -MATE_VALUE + MAX_PLY) {
        return 1e-12; // Opponent mates
    }
    double x = (cp_score + WIN_PROB_OFFSET) / 100.0;
    return 1.0 / (1.0 + std::exp(-WIN_PROB_K * x));
}

// --- Time Check ---
void check_time() {
    if ((NodeCount & 4095) == 0) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
        if (elapsed >= Limits.movetime) {
            StopSearch = true;
        }
    }
}

// --- Clear Search Globals ---
void clear_search_globals() {
    NodeCount = 0;
    StopSearch = false;

    for (int i = 0; i < MAX_PLY + 1; ++i) {
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

// --- Search Function ---
int search(int alpha, int beta, int depth, int ply, Position& pos, bool do_null) {
    NodeCount++;
    if (StopSearch) return 0;

    if (ply > MAX_PLY) return evaluate(pos);

    // Draw detection
    if (is_draw(pos)) return 0;

    bool in_check = pos.is_check();

    // Check extension
    if (in_check) depth++;

    // Quiescence at leaf nodes
    if (depth <= 0) return quiescence(alpha, beta, ply, pos);

    // TT probe
    TTEntry* tt_entry_ptr = TT.probe(pos.hash_key);
    Move tt_move = Move(0);
    if (tt_entry_ptr) {
        tt_move = Move(tt_entry_ptr->move);
        if (tt_entry_ptr->depth >= depth) {
            int tt_score = tt_entry_ptr->score;
            if (abs(tt_score) > MATE_VALUE - 1000) {
                if (tt_score > 0) tt_score -= ply;
                else tt_score += ply;
            }
            if (tt_entry_ptr->flags == TT_EXACT) return tt_score;
            if (tt_entry_ptr->flags == TT_LOWER && tt_score >= beta) return tt_score;
            if (tt_entry_ptr->flags == TT_UPPER && tt_score <= alpha) return tt_score;
        }
    }

    int static_eval = evaluate(pos);
    bool improving = false; // TODO: implement improving

    // Null move pruning
    bool has_non_pawn = false;
    for (int pt = WN; pt <= BQ; ++pt) {
        if (pos.piece_bitboards[pt] != 0) { has_non_pawn = true; break; }
    }
    if (do_null && !in_check && depth >= 3 && static_eval >= beta && has_non_pawn) {
        int R = 3 + depth / 6;
        pos.make_null_move();
        int null_score = -search(-beta, -beta + 1, depth - R, ply + 1, pos, false);
        pos.unmake_null_move();
        if (null_score >= beta) return beta;
    }

    // Internal iterative deepening
    if (depth >= 4 && tt_move.value == 0) {
        search(alpha, beta, depth - 2, ply, pos, do_null);
        TTEntry* tt_entry_ptr2 = TT.probe(pos.hash_key);
        if (tt_entry_ptr2) {
            tt_move = Move(tt_entry_ptr2->move);
        }
    }

    Move* captures = CAPTURES[ply];
    int& num_captures = NUM_CAPTURES[ply];
    Move* quiets = QUIETS[ply];
    int& num_quiets = NUM_QUIETS[ply];
    generate_moves(pos, captures, num_captures, quiets, num_quiets, false);
    order_moves(captures, num_captures, quiets, num_quiets, ply, tt_move, pos);

    int best_score = -MATE_VALUE;
    Move best_move = Move(0);
    int moves_searched = 0;
    int tt_flag = TT_UPPER;
    int second_best = -MATE_VALUE;

    for (int i = 0; i < num_captures + num_quiets; ++i) {
        Move move = (i < num_captures) ? captures[i] : quiets[i - num_captures];
        if (move == tt_move) continue; // Already searched TT move first

        bool gives_check = false; // TODO: implement gives_check
        int reduction = 0;

        // Late move reduction
        if (moves_searched >= 4 && depth >= 3 && !move.is_capture() && !gives_check && !in_check) {
            reduction = 1 + (moves_searched >= 8 ? 1 : 0);
            if (!improving) reduction++;
        }

        if (!pos.make_move(move)) continue;
        // if (USE_NNUE) NNUE::nnue_evaluator.update_make(pos, move, pos.history[pos.history_size-1], ply);

        int score;
        if (moves_searched == 0) {
            score = -search(-beta, -alpha, depth - 1 + (gives_check ? 1 : 0), ply + 1, pos, true);
        } else {
            // PVS
            score = -search(-alpha - 1, -alpha, depth - 1 - reduction + (gives_check ? 1 : 0), ply + 1, pos, true);
            if (score > alpha && score < beta) {
                score = -search(-beta, -alpha, depth - 1 + (gives_check ? 1 : 0), ply + 1, pos, true);
            }
        }
        if (score > alpha) {
            alpha = score;
            PV_TABLE[ply][ply] = move;
            for (int i = 0; i < PV_LENGTH[ply + 1]; ++i) {
                PV_TABLE[ply][ply + 1 + i] = PV_TABLE[ply + 1][ply + 1 + i];
            }
            PV_LENGTH[ply] = 1 + PV_LENGTH[ply + 1];
        }
        // if (USE_NNUE) NNUE::nnue_evaluator.update_unmake(pos, move, pos.history[pos.history_size-1], ply);
        pos.unmake_move(move);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (score > second_best && score < best_score) second_best = score;

        moves_searched++;

        if (best_score >= beta) {
            if (!move.is_capture()) {
                KILLER_MOVES[ply][1] = KILLER_MOVES[ply][0];
                KILLER_MOVES[ply][0] = move;
                HISTORY_TABLE[move.moving_piece()][move.to()] = std::min(HISTORY_TABLE[move.moving_piece()][move.to()] + depth * depth * 8, HISTORY_MAX);
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
        return in_check ? -(MATE_VALUE - ply) : 0;
    }

    // Singular extension
    if (best_score - second_best >= 60 * depth && depth < 60) {
        int extended_score = search(alpha, beta, depth + 1, ply, pos, do_null);
        if (abs(extended_score) < MATE_VALUE) {
            best_score = extended_score;
            if (best_score > alpha) alpha = best_score;
        }
    }

    return alpha;
}

// --- Policy Score ---
int policy_score(Move m);

// --- Move Scoring ---
int score_move(Move move, int ply, Move tt_move, const Position& pos) {
    if (move == tt_move) return 1 << 20;
    if (move.is_capture()) {
        int see_score = see(pos, move);
        if (see_score < 0) return 100000 + see_score; // Demote losing captures
        return 100000 + (move.captured_piece() * 10) - move.moving_piece();
    }
    if (move == KILLER_MOVES[ply][0]) return 8000;
    if (move == KILLER_MOVES[ply][1]) return 7000;
    return HISTORY_TABLE[move.moving_piece()][move.to()] + policy_score(move);
}

// --- Policy Score ---
int policy_score(Move m) {
    if (m.is_capture()) return 0;
    if (m.is_promotion()) return 50;
    Square to = m.to();
    int file = get_file(to);
    int rank = get_rank(to);
    int center = 4 - std::abs(file - 3) - std::abs(rank - 3);
    return center;
}

// --- Move Ordering ---
void order_moves(MoveList& moves, int ply, Move tt_move, const Position& pos) {
    for (auto& move : moves) {
        move.set_ordering_hint(score_move(move, ply, tt_move, pos));
    }
    std::sort(moves.begin(), moves.end(), [&](Move a, Move b) {
        return a.ordering_hint() > b.ordering_hint();
    });
}

void order_moves(Move* captures, int num_captures, Move* quiets, int num_quiets, int ply, Move tt_move, const Position& pos) {
    // Sort captures
    for (int i = 0; i < num_captures; ++i) {
        captures[i].set_ordering_hint(score_move(captures[i], ply, tt_move, pos));
    }
    // Simple bubble sort for captures
    for (int i = 0; i < num_captures - 1; ++i) {
        for (int j = 0; j < num_captures - i - 1; ++j) {
            if (captures[j].ordering_hint() < captures[j+1].ordering_hint()) {
                std::swap(captures[j], captures[j+1]);
            }
        }
    }
    // Sort quiets
    for (int i = 0; i < num_quiets; ++i) {
        quiets[i].set_ordering_hint(score_move(quiets[i], ply, tt_move, pos));
    }
    // Simple bubble sort for quiets
    for (int i = 0; i < num_quiets - 1; ++i) {
        for (int j = 0; j < num_quiets - i - 1; ++j) {
            if (quiets[j].ordering_hint() < quiets[j+1].ordering_hint()) {
                std::swap(quiets[j], quiets[j+1]);
            }
        }
    }
}

// --- Quiescence Search ---
int quiescence(int alpha, int beta, int ply, Position& pos) {
    NodeCount++;
    if (StopSearch) return 0;

    if (ply > 10) return evaluate(pos);

    // Draw detection
    if (is_draw(pos)) return 0;

    if (ply >= MAX_PLY) return evaluate(pos);

    bool in_check = pos.is_check();
    int stand_pat = evaluate(pos);
    if (!in_check && stand_pat >= beta) return beta;
    if (!in_check) alpha = std::max(alpha, stand_pat);

    Move* captures = CAPTURES[ply];
    int& num_captures = NUM_CAPTURES[ply];
    Move* quiets = QUIETS[ply];
    int& num_quiets = NUM_QUIETS[ply];
    bool only_captures = !in_check;
    generate_moves(pos, captures, num_captures, quiets, num_quiets, only_captures);
    order_moves(captures, num_captures, quiets, num_quiets, ply, Move(0), pos);

    if (num_captures + num_quiets == 0) {
        if (in_check) return -(MATE_VALUE - ply);
        else return stand_pat;
    }

    for (int i = 0; i < num_captures + num_quiets; ++i) {
        Move move = (i < num_captures) ? captures[i] : quiets[i - num_captures];
        if (!pos.make_move(move)) continue;
        int score = -quiescence(-beta, -alpha, ply + 1, pos);
        pos.unmake_move(move);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}







// --- Get root moves scores ---
int get_root_moves_scores(Position& pos, int depth, std::pair<Move, int>* scores, int max_scores) {
    MoveList moves;
    generate_moves(pos, moves);
    order_moves(moves, 0, Move(0), pos); // No TT move for simplicity

    int count = 0;
    for (const auto& move : moves) {
        if (count >= max_scores) break;
        if (pos.make_move(move)) {
            int score = -search(-MATE_VALUE, MATE_VALUE, depth - 1, 1, pos, true);
            pos.unmake_move(move);
            scores[count++] = {move, score};
        }
    }
    return count;
}



SearchResult search_position(Position& pos, const SearchLimits& limits, const ChessWizardOptions* opts) {
    RootPosition = pos;
    Limits = limits;
    // Short time mode
    if (Limits.movetime < 100) {
        Limits.max_depth = std::min(Limits.max_depth, 6);
    }
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

    // Syzygy tablebase
    if (opts && opts->use_syzygy) {
        TBResult tb_result;
        if (probe_syzygy(pos, tb_result)) {
            SearchResult result = {};
            std::string uci = tb_result.best_move.to_uci_string();
            strncpy(result.best_move_uci, uci.c_str(), 7);
            std::string json = "[\"" + uci + "\"]";
            result.pv_json = (char*)malloc(json.size() + 1);
            strcpy(result.pv_json, json.c_str());
            result.score_cp = tb_result.score;
            result.win_prob = sigmoid_win_prob(tb_result.score);
            result.depth = 0;
            result.nodes = 0;
            result.time_ms = 0;
            result.info_flags = TB;
            return result;
        }
    }

    // Opening book
    if (opts && opts->book_path && strlen(opts->book_path) > 0) {
        if (!OPENING_BOOK.is_loaded()) {
            OPENING_BOOK.load(opts->book_path);
        }
        Move book_move = OPENING_BOOK.get_move(pos.hash_key);
        if (book_move.value != 0) {
            SearchResult result = {};
            std::string uci = book_move.to_uci_string();
            strncpy(result.best_move_uci, uci.c_str(), 7);
            std::string json = "[\"" + uci + "\"]";
            result.pv_json = (char*)malloc(json.size() + 1);
            strcpy(result.pv_json, json.c_str());
            result.score_cp = 0;
            result.win_prob = 0.5;
            result.depth = 0;
            result.nodes = 0;
            result.time_ms = 0;
            result.info_flags = BOOK;
            return result;
        }
    }

    int last_score = 0;
    int score = 0;
    int last_completed_depth = 0;
    int depth_scores[64];
    double win_probs[64];
    int depth_idx = 0;
    std::pair<Move, int> last_moves_scores[256]; // Assuming max moves
    int last_moves_count = 0;

    for (int current_depth = 1; current_depth <= Limits.max_depth; ++current_depth) {
        // Disable aspiration for now
        int alpha = -MATE_VALUE;
        int beta = MATE_VALUE;

        score = search(alpha, beta, current_depth, 0, pos, true);

        if (StopSearch && current_depth > 1) {
            break;
        }

        last_moves_count = get_root_moves_scores(pos, current_depth, last_moves_scores, 256);

        if (StopSearch && current_depth > 1) {
            break;
        }

        last_score = score;
        last_completed_depth = current_depth;
        depth_scores[depth_idx] = score;
        win_probs[depth_idx] = sigmoid_win_prob(score);
        depth_idx++;

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

    // Monte Carlo tie-break disabled for now
    if (PV_LENGTH[0] > 0) {
        strncpy(result.best_move_uci, PV_TABLE[0][0].to_uci_string().c_str(), 7);
        result.best_move_uci[7] = '\0';

        std::string json = "[";
        for (int i = 0; i < PV_LENGTH[0]; ++i) {
            json += "\"" + PV_TABLE[0][i].to_uci_string() + "\"";
            if (i < PV_LENGTH[0] - 1) json += ",";
        }
        json += "]";
        result.pv_json = (char*)malloc(json.size() + 1);
        strcpy(result.pv_json, json.c_str());
    } else {
        result.pv_json = (char*)malloc(3);
        strcpy(result.pv_json, "[]");
    }

    result.score_cp = score;
    result.depth = last_completed_depth;
    result.nodes = NodeCount;
    auto final_time = std::chrono::steady_clock::now();
    result.time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(final_time - start_time).count();

    result.win_prob = sigmoid_win_prob(score);

    // Calculate uncertainty as stddev of depth scores
    double stddev_cp = 0.0;
    if (depth_idx > 1) {
        double mean = 0.0;
        for (int i = 0; i < depth_idx; ++i) mean += depth_scores[i];
        mean /= depth_idx;
        double stddev_cp = 0.0;
        for (int i = 0; i < depth_idx; ++i) stddev_cp += (depth_scores[i] - mean) * (depth_scores[i] - mean);
        stddev_cp = sqrt(stddev_cp / (depth_idx - 1));
        result.win_prob_stddev = sigmoid_win_prob(score + stddev_cp) - sigmoid_win_prob(score - stddev_cp);
    }

    // Monte Carlo tie-break
    std::sort(last_moves_scores, last_moves_scores + last_moves_count, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    if (last_moves_count >= 2 && abs(last_moves_scores[0].second - last_moves_scores[1].second) <= 20) {
        // Run rollouts for top 2 moves
        Position temp_pos = pos;
        temp_pos.make_move(last_moves_scores[0].first);
        auto [w1, l1, d1] = rollout(temp_pos, 40);
        double wr1 = (w1 + 0.5 * d1) / (w1 + l1 + d1 + 1e-9); // avoid div0

        temp_pos = pos;
        temp_pos.make_move(last_moves_scores[1].first);
        auto [w2, l2, d2] = rollout(temp_pos, 40);
        double wr2 = (w2 + 0.5 * d2) / (w2 + l2 + d2 + 1e-9);

        if (wr2 > wr1) {
            // Choose the second move
            std::swap(last_moves_scores[0], last_moves_scores[1]);
            std::string uci = last_moves_scores[0].first.to_uci_string();
            strncpy(result.best_move_uci, uci.c_str(), 7);
            std::string json = "[\"" + uci + "\"]";
            free(result.pv_json); // free previous
            result.pv_json = (char*)malloc(json.size() + 1);
            strcpy(result.pv_json, json.c_str());
            result.score_cp = last_moves_scores[0].second;
            result.win_prob = sigmoid_win_prob(result.score_cp);
        }
        result.info_flags |= MC_TIEBREAK;
    }

    return result;
}

// --- Perft ---
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
        if (pos.make_move(move)) {
            nodes += perft(depth - 1, pos);
            pos.unmake_move(move);
        }
    }

    return nodes;
}

// Debug perft to print moves at depth 1
uint64_t perft_debug(int depth, Position& pos, int original_depth) {
    if (depth == 0) {
        return 1;
    }

    MoveList moves;
    generate_legal_moves(pos, moves);
    uint64_t nodes = 0;

    if (depth == 1) {
        std::cerr << "Moves at depth 1: " << moves.size() << std::endl;
        return moves.size();
    }

    for (const auto& move : moves) {
        pos.make_move(move);
        uint64_t sub_nodes = perft_debug(depth - 1, pos, original_depth);
        if (depth == 3) {
            std::cout << move.to_uci_string() << ": " << sub_nodes << std::endl;
        }
        nodes += sub_nodes;
        pos.unmake_move(move);
    }

    return nodes;
}