#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include "board.h"
#include "movegen.h"
#include "engine.h"
#include "search.h"
#include "board.h"
#include "movegen.h"
#include "evaluate.h"
#include "tt.h"
#include "move.h"
#include "nnue.h"
#include "types.h"
#include "attack.h"
#include "book.h"
#include "tb_probe.h"
#include "zobrist.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <chrono>
#include <unordered_map>
#include <list>

#include "book.h"
#include "nnue.h"

extern ChessWizardOptions OPTIONS;
extern Book OPENING_BOOK;
extern std::string NNUE_PATH_BUFFER;
extern std::string BOOK_PATH_BUFFER;
extern void init_all();

// LRU Cache for session results
const size_t CACHE_SIZE = 4096;
std::unordered_map<std::string, std::list<std::pair<std::string, SearchResult>>::iterator> cache_map;
std::list<std::pair<std::string, SearchResult>> cache_list;

SearchResult* get_cached_result(const std::string& fen) {
    auto it = cache_map.find(fen);
    if (it == cache_map.end()) return nullptr;
    // Move to front
    cache_list.splice(cache_list.begin(), cache_list, it->second);
    return &it->second->second;
}

void put_cached_result(const std::string& fen, const SearchResult& result) {
    auto it = cache_map.find(fen);
    if (it != cache_map.end()) {
        // Update
        it->second->second = result;
        cache_list.splice(cache_list.begin(), cache_list, it->second);
    } else {
        // Insert
        if (cache_list.size() >= CACHE_SIZE) {
            auto last = cache_list.back();
            cache_map.erase(last.first);
            cache_list.pop_back();
        }
        cache_list.emplace_front(fen, result);
        cache_map[fen] = cache_list.begin();
    }
}

void clear_cache() {
    cache_map.clear();
    cache_list.clear();
}

bool is_game_over(const Position& pos, std::string& reason) {
    MoveList moves;
    generate_legal_moves(pos, moves);
    if (moves.empty()) {
        if (pos.is_check()) {
            reason = (pos.side_to_move == WHITE ? "Black wins (checkmate)" : "White wins (checkmate)");
            return true;
        } else {
            reason = "Draw (stalemate)";
            return true;
        }
    }
    if (pos.halfmove_clock >= 100) {
        reason = "Draw (50-move rule)";
        return true;
    }
    // Insufficient material: only kings, or kings + same color bishops/knights
    int white_pieces = 0, black_pieces = 0;
    bool white_bishop = false, black_bishop = false;
    bool white_knight = false, black_knight = false;
    for (int i = 0; i < 64; ++i) {
        PieceType p = pos.piece_on_square((Square)i);
        if (p == WK || p == BK) continue;
        if (p >= BP) {
            black_pieces++;
            if (p == BB) black_bishop = true;
            else if (p == BN) black_knight = true;
        } else {
            white_pieces++;
            if (p == WB) white_bishop = true;
            else if (p == WN) white_knight = true;
        }
    }
    if (white_pieces == 0 && black_pieces == 0) {
        reason = "Draw (insufficient material)";
        return true;
    }
    if (white_pieces == 1 && black_pieces == 0) {
        if (white_bishop || white_knight) {
            reason = "Draw (insufficient material)";
            return true;
        }
    }
    if (black_pieces == 1 && white_pieces == 0) {
        if (black_bishop || black_knight) {
            reason = "Draw (insufficient material)";
            return true;
        }
    }
    if (white_pieces == 1 && black_pieces == 1) {
        if ((white_bishop && black_bishop) || (white_knight && black_knight)) {
            // Same color bishops or both knights
            reason = "Draw (insufficient material)";
            return true;
        }
    }
    return false;
}

void print_result(const SearchResult& result, const Position& pos) {
    // JSON
    std::cout << "{\"best_move\":\"" << result.best_move_uci << "\",\"pv\":" << result.pv_json
              << ",\"score_cp\":" << result.score_cp << ",\"win_prob\":" << result.win_prob
              << ",\"win_prob_stddev\":" << result.win_prob_stddev
              << ",\"depth\":" << (int)result.depth << ",\"nodes\":" << result.nodes
              << ",\"time_ms\":" << result.time_ms << "}" << std::endl;

    // Human summary
    Move move = get_move_from_uci(std::string(result.best_move_uci), pos);
    std::string san = move.to_san_string(pos);
    std::string source = "ENGINE";
    if (result.info_flags & BOOK) source = "BOOK";
    else if (result.info_flags & TB) source = "TB";
    else if (result.info_flags & CACHE) source = "CACHE";
    else if (result.info_flags & MC_TIEBREAK) source = "MC_TIEBREAK";

    std::cout << "Recommended: " << san << " (" << result.best_move_uci << ")  Score: " << result.score_cp
              << "  WinProb: " << (result.win_prob * 100) << "%  Depth: " << (int)result.depth
              << "  Time: " << result.time_ms << "ms  Source: " << source << std::endl;
}

void run_perft_tests() {
    std::cout << "Running perft tests..." << std::endl;
    Position pos;
    pos.set_from_fen(START_FEN);

    // Perft targets from spec
    std::vector<uint64_t> targets = {20, 400, 8902, 197281, 4865609, 119060324};
    for (int depth = 1; depth <= 6; ++depth) {
        uint64_t nodes = perft(depth, pos);
        std::cout << "Perft " << depth << ": " << nodes << " (expected " << targets[depth-1] << ")";
        if (nodes == targets[depth-1]) {
            std::cout << " PASS" << std::endl;
        } else {
            std::cout << " FAIL" << std::endl;
        }
    }

    std::cout << "Perft tests completed." << std::endl;
}



void run_tests() {
    std::cout << "Running unit tests..." << std::endl;

    // Zobrist invariance test
    Position pos;
    pos.set_from_fen(START_FEN);
    uint64_t original_key = pos.hash_key;

    // Generate moves
    MoveList moves;
    generate_moves(pos, moves);

    if (!moves.empty()) {
        Move m = moves[0];
        pos.make_move(m);
        pos.unmake_move(m);
        if (pos.hash_key == original_key) {
            std::cout << "Zobrist invariance: PASS" << std::endl;
        } else {
            std::cout << "Zobrist invariance: FAIL" << std::endl;
        }
    }

    // NNUE parity test (if NNUE loaded)
    // TODO: fix with si and ply
    std::cout << "NNUE parity test skipped" << std::endl;

    // Evaluate start position
    std::cout << "Evaluate start position: " << evaluate(pos) << std::endl;

    std::cout << "Tests completed." << std::endl;
}

void run_integration_tests() {
    std::cout << "Running integration tests..." << std::endl;

    Position pos;
    pos.set_from_fen(START_FEN);
    std::vector<uint64_t> targets = {20, 400, 8902, 197281, 4865609, 119060324};

    for (int d = 1; d <= 6; ++d) {
        uint64_t nodes = perft(d, pos);
        if (nodes == targets[d-1]) {
            std::cout << "Perft " << d << ": PASS (" << nodes << ")" << std::endl;
        } else {
            std::cout << "Perft " << d << ": FAIL (" << nodes << " vs " << targets[d-1] << ")" << std::endl;
        }
    }

    std::cout << "Integration tests completed." << std::endl;
}



// --- UCI Loop ---
void uci_loop() {
    Position pos;
    pos.set_from_fen(START_FEN);

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "uci") {
            std::cout << "id name Chess Wizard" << std::endl;
            std::cout << "id author Gemini" << std::endl;
            std::cout << "option name TT Size type spin default 32 min 1 max 1024" << std::endl;
            std::cout << "option name Use NNUE type check default false" << std::endl;
            std::cout << "option name NNUE_File type string default" << std::endl;
            std::cout << "option name Book type string default" << std::endl;
            std::cout << "option name SyzygyPath type string default" << std::endl;
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "setoption") {
            std::string name_token, value_token, name, value;
            iss >> name_token >> name;
            if (name == "TT") {
                iss >> name; // "Size"
                iss >> value_token >> value;
                OPTIONS.tt_size_mb = std::stoi(value);
                TT.resize(OPTIONS.tt_size_mb);
            } else if (name == "Use") {
                iss >> name; // "NNUE"
                iss >> value_token >> value;
                OPTIONS.use_nnue = (value == "true");
            } else if (name == "NNUE_File") {
                iss >> value_token >> value;
                NNUE_PATH_BUFFER = value;
                OPTIONS.nnue_path = NNUE_PATH_BUFFER.c_str();
            } else if (name == "Book") {
                iss >> value_token >> value;
                BOOK_PATH_BUFFER = value;
                OPTIONS.book_path = BOOK_PATH_BUFFER.c_str();
                OPENING_BOOK.load(OPTIONS.book_path);
            } else if (name == "SyzygyPath") {
                iss >> value_token >> value;
                // A real implementation would initialize the tablebase here.
                std::cout << "info string SyzygyPath set to " << value << std::endl;
            }
        } else if (token == "ucinewgame") {
            pos.set_from_fen(START_FEN);
        } else if (token == "position") {
            std::string sub_token;
            iss >> sub_token;
            if (sub_token == "startpos") {
                pos.set_from_fen(START_FEN);
                iss >> sub_token; // "moves"
            } else if (sub_token == "fen") {
                std::string fen;
                while (iss >> sub_token && sub_token != "moves") {
                    fen += sub_token + " ";
                }
                pos.set_from_fen(fen);
            }
            
            while (iss >> sub_token) {
                Move move = get_move_from_uci(sub_token, pos);
                if (move.value != 0) {
                    pos.make_move(move);
                }
            }
        } else if (token == "go") {
            SearchLimits limits;
            std::string sub_token;
            while (iss >> sub_token) {
                if (sub_token == "wtime") iss >> limits.movetime; // Simplified
                if (sub_token == "btime") iss >> limits.movetime;
                if (sub_token == "movetime") iss >> limits.movetime;
                if (sub_token == "depth") iss >> limits.max_depth;
            }
            SearchResult result = search_position(pos, limits, &OPTIONS);
            std::cout << "bestmove " << result.best_move_uci << std::endl;
        } else if (token == "perft") {
            int depth;
            iss >> depth;
            uint64_t nodes = perft_debug(depth, pos, depth);
            std::cout << "Perft " << depth << ": " << nodes << std::endl;
        } else if (token == "quit") {
            break;
        }
    }
}

void cli_loop() {
    Position pos;
    pos.set_from_fen(START_FEN);
    TT.resize(32);

    TT.clear();
    int time_ms = 2000; // default

    std::cout << "Chess Wizard ready. Who started the match? Type 'me' if you started (White) or 'opponent' if they started (Black)." << std::endl;
    std::string response;
    while (std::getline(std::cin, response)) {
        if (response == "me" || response == "white" || response == "ME" || response == "WHITE" || response == "newgame") {
            // Case me: set board to startpos
            pos.set_from_fen(START_FEN);
            // Check book
            bool book_used = false;
            if (OPTIONS.book_path && strlen(OPTIONS.book_path) > 0) {
                if (!OPENING_BOOK.is_loaded()) {
                    OPENING_BOOK.load(OPTIONS.book_path);
                }
                Move book_move = OPENING_BOOK.get_move(pos.hash_key);
                if (book_move.value != 0) {
                    // Assume threshold met for simplicity
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
                    print_result(result, pos);
                    pos.make_move(book_move);
                    book_used = true;
                }
            }
            if (!book_used) {
                SearchLimits limits;
                limits.movetime = time_ms;
                limits.max_depth = 64;
                SearchResult result = search_position(pos, limits, &OPTIONS);
                print_result(result, pos);
                pos.make_move(get_move_from_uci(result.best_move_uci, pos));
            }
            break;
        } else if (response == "opponent" || response == "black" || response == "OPPONENT" || response == "BLACK") {
            // Case opponent: prompt for opponent move
            std::cout << "Enter opponent move (UCI or SAN):" << std::endl;
            std::string opponent_move_str;
            if (!std::getline(std::cin, opponent_move_str)) return;
            Move opponent_move = get_move_from_uci(opponent_move_str, pos);
            if (opponent_move.value == 0) {
                std::cout << "Illegal move" << std::endl;
                continue;
            }
            pos.make_move(opponent_move);
            std::cout << "Applied opponent move: " << opponent_move_str << std::endl;
            break;
        } else if (response == "quit") {
            return;
        } else {
            std::cout << "Invalid response — type \"me\" or \"opponent\"" << std::endl;
        }
    }

    // Per-move loop
    std::string line;
    while (std::getline(std::cin, line)) {
        std::cout << "Enter opponent move (UCI or SAN) or command (newgame/undo/quit): " << std::endl;
        if (line == "quit") break;
        if (line == "help") {
            std::cout << "Commands: newgame, undo, fen <FEN>, set time <ms>, set tt <MB>, quit, help" << std::endl;
            continue;
        }
        if (line == "newgame") {
            pos.set_from_fen(START_FEN);
            clear_cache();
            std::cout << "New game started." << std::endl;
            continue;
        }
        if (line == "undo") {
            if (pos.history_size >= 2) {
                // Reconstruct last two moves from history
                StateInfo si1 = pos.history[pos.history_size - 1];
                StateInfo si2 = pos.history[pos.history_size - 2];
                Move m1 = Move(0);
                m1.set_from((Square)si1.from);
                m1.set_to((Square)si1.to);
                PieceType moving1 = pos.piece_on_square((Square)si1.to);
                m1.set_moving_piece(moving1);
                m1.set_captured_piece((PieceType)si1.captured_piece);
                if (si1.promoted_piece != NO_PIECE) {
                    Move::PromotionType prom = Move::NO_PROMOTION;
                    switch (si1.promoted_piece) {
                        case WN: case BN: prom = Move::PROMOTION_N; break;
                        case WB: case BB: prom = Move::PROMOTION_B; break;
                        case WR: case BR: prom = Move::PROMOTION_R; break;
                        case WQ: case BQ: prom = Move::PROMOTION_Q; break;
                    }
                    m1.set_promotion(prom);
                }
                uint8_t flags1 = 0;
                if (si1.captured_piece != NO_PIECE && abs(si1.to - si1.from) != 8 && abs(si1.to - si1.from) != 16) flags1 |= Move::EN_PASSANT;
                if (abs(si1.to - si1.from) == 16 && (moving1 == WP || moving1 == BP)) flags1 |= Move::DOUBLE_PAWN_PUSH;
                if ((si1.from == 4 && si1.to == 6) || (si1.from == 60 && si1.to == 62) || (si1.from == 4 && si1.to == 2) || (si1.from == 60 && si1.to == 58)) flags1 |= Move::CASTLING;
                m1.set_flags(flags1);

                pos.unmake_move(m1);

                Move m2 = Move(0);
                m2.set_from((Square)si2.from);
                m2.set_to((Square)si2.to);
                PieceType moving2 = pos.piece_on_square((Square)si2.to);
                m2.set_moving_piece(moving2);
                m2.set_captured_piece((PieceType)si2.captured_piece);
                if (si2.promoted_piece != NO_PIECE) {
                    Move::PromotionType prom = Move::NO_PROMOTION;
                    switch (si2.promoted_piece) {
                        case WN: case BN: prom = Move::PROMOTION_N; break;
                        case WB: case BB: prom = Move::PROMOTION_B; break;
                        case WR: case BR: prom = Move::PROMOTION_R; break;
                        case WQ: case BQ: prom = Move::PROMOTION_Q; break;
                    }
                    m2.set_promotion(prom);
                }
                uint8_t flags2 = 0;
                if (si2.captured_piece != NO_PIECE && abs(si2.to - si2.from) != 8 && abs(si2.to - si2.from) != 16) flags2 |= Move::EN_PASSANT;
                if (abs(si2.to - si2.from) == 16 && (moving2 == WP || moving2 == BP)) flags2 |= Move::DOUBLE_PAWN_PUSH;
                if ((si2.from == 4 && si2.to == 6) || (si2.from == 60 && si2.to == 62) || (si2.from == 4 && si2.to == 2) || (si2.from == 60 && si2.to == 58)) flags2 |= Move::CASTLING;
                m2.set_flags(flags2);

                pos.unmake_move(m2);
                std::cout << "Undid last two moves." << std::endl;
            } else {
                std::cout << "Cannot undo." << std::endl;
            }
            continue;
        }
        if (line.substr(0, 4) == "fen ") {
            std::string fen = line.substr(4);
            pos.set_from_fen(fen);
            std::cout << "Board set to FEN: " << fen << std::endl;
            continue;
        }
        if (line.substr(0, 9) == "set time ") {
            time_ms = std::atoi(line.substr(9).c_str());
            std::cout << "Time set to " << time_ms << "ms." << std::endl;
            continue;
        }
        if (line.substr(0, 7) == "set tt ") {
            int tt_mb = std::atoi(line.substr(7).c_str());
            OPTIONS.tt_size_mb = tt_mb;
            TT.resize(tt_mb);
            std::cout << "TT size set to " << tt_mb << "MB." << std::endl;
            continue;
        }

        // Parse move
        Move move = get_move_from_uci(line, pos);
        if (move.value != 0) {
            pos.make_move(move);
            std::cout << "Applied move: " << line << std::endl;

            // Check terminal
            std::string reason;
            if (is_game_over(pos, reason)) {
                std::cout << "Game over — " << reason << std::endl;
                std::cout << "New game? (yes/no)" << std::endl;
                std::string answer;
                if (std::getline(std::cin, answer) && (answer == "yes" || answer == "y")) {
                    pos.set_from_fen(START_FEN);
                    clear_cache();
                    std::cout << "New game started." << std::endl;
                    continue;
                } else {
                    break;
                }
            }

            // Suggest move
            SearchLimits limits;
            limits.movetime = time_ms;
            limits.max_depth = 64;
            SearchResult result = search_position(pos, limits, &OPTIONS);
            print_result(result, pos);
            pos.make_move(get_move_from_uci(result.best_move_uci, pos));

            // Check terminal after engine move
            if (is_game_over(pos, reason)) {
                std::cout << "Game over — " << reason << std::endl;
                std::cout << "New game? (yes/no)" << std::endl;
                std::string answer;
                if (std::getline(std::cin, answer) && (answer == "yes" || answer == "y")) {
                    pos.set_from_fen(START_FEN);
                    clear_cache();
                    std::cout << "New game started." << std::endl;
                    continue;
                } else {
                    break;
                }
            }
        } else {
            std::cout << "Invalid move or command." << std::endl;
        }
    }
}


int main(int argc, char* argv[]) {
    init_all();
    
    // Parse args
    bool is_cli = false;
    bool is_bench = false;
    bool is_test = false;
    bool is_perft = false;
    bool is_integration_test = false;
    int bench_tt_size = 32;
    int bench_time_ms = 10000;
    std::string bench_position = START_FEN;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--cli") {
            is_cli = true;
        } else if (arg == "--test") {
            is_test = true;
        } else if (arg == "--perft") {
            is_perft = true;
        } else if (arg == "--integration-test") {
            is_integration_test = true;
        } else if (arg == "--bench") {
            is_bench = true;
            // Parse bench options
            for (int j = i + 1; j < argc; ++j) {
                if (std::string(argv[j]) == "--tt-size" && j + 1 < argc) {
                    bench_tt_size = std::stoi(argv[++j]);
                } else if (std::string(argv[j]) == "--time" && j + 1 < argc) {
                    std::string time_str = argv[++j];
                    if (time_str.back() == 's') {
                        bench_time_ms = std::stoi(time_str.substr(0, time_str.size() - 1)) * 1000;
                    } else {
                        bench_time_ms = std::stoi(time_str);
                    }
                } else if (std::string(argv[j]) == "--position" && j + 1 < argc) {
                    bench_position = argv[++j];
                }
            }
        }
    }

    if (is_test) {
        run_tests();
        return 0;
    }

    if (is_perft) {
        run_perft_tests();
        return 0;
    }

    if (is_integration_test) {
        run_integration_tests();
        return 0;
    }

    if (is_bench) {
        TT.resize(bench_tt_size);
        Position pos;
        pos.set_from_fen(bench_position);

        auto start = std::chrono::steady_clock::now();
        uint64_t total_nodes = 0;
        int max_depth = 0;

        while (true) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= bench_time_ms) break;

            SearchLimits limits;
            limits.movetime = bench_time_ms - elapsed;
            limits.max_depth = 64;

            SearchResult result = search_position(pos, limits, &OPTIONS);
            total_nodes += result.nodes;
            max_depth = std::max(max_depth, (int)result.depth);

            if (result.nodes == 0) break;
        }

        auto end = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        double nps = total_nodes * 1000.0 / total_time;

        std::cout << "Benchmark results:" << std::endl;
        std::cout << "Total nodes: " << total_nodes << std::endl;
        std::cout << "Max depth: " << max_depth << std::endl;
        std::cout << "Time: " << total_time << " ms" << std::endl;
        std::cout << "Nodes/sec: " << (uint64_t)nps << std::endl;

        return 0;
    }

    if (is_cli) {
        cli_loop();
    } else {
        uci_loop();
    }
    return 0;
}