#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include "position.h"
#include "movegen.h"
#include "engine.h"
#include "attack.h"
#include "zobrist.h"
#include "tt.h"
#include "types.h"
#include "move.h"
#include <chrono>

#include "book.h"
#include "nnue.h"

extern ChessWizardOptions OPTIONS;
extern Book OPENING_BOOK;
extern std::string NNUE_PATH_BUFFER;
extern std::string BOOK_PATH_BUFFER;
extern void init_all();



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
    if (NNUE::nnue_available) {
        pos.set_from_fen(START_FEN);
        NNUE::nnue_evaluator.reset(pos);
        int full_eval = NNUE::nnue_evaluator.evaluate(WHITE);

        generate_moves(pos, moves);
        if (!moves.empty()) {
            Move m = moves[0];
            pos.make_move(m);
            NNUE::nnue_evaluator.update_make(pos, m);
            int inc_eval = NNUE::nnue_evaluator.evaluate(WHITE);
            pos.unmake_move(m);
            NNUE::nnue_evaluator.update_unmake(pos, m);

            if (full_eval == inc_eval) {
                std::cout << "NNUE parity: PASS" << std::endl;
            } else {
                std::cout << "NNUE parity: FAIL (" << full_eval << " vs " << inc_eval << ")" << std::endl;
            }
        }
    } else {
        std::cout << "NNUE not loaded, skipping parity test" << std::endl;
    }

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

    std::cout << "Do you want the engine to make the first move? (y/n): ";
    std::string response;
    std::getline(std::cin, response);
    bool engine_first = (response == "y" || response == "Y");

    if (engine_first) {
        // Engine makes first move
        SearchLimits limits;
        limits.movetime = 5000;
        limits.max_depth = 64;

        SearchResult result = search_position(pos, limits, &OPTIONS);

        std::cout << "{\"best_move\":\"" << result.best_move_uci << "\",\"pv\":[";
        for (size_t i = 0; i < result.pv_uci.size(); ++i) {
            std::cout << "\"" << result.pv_uci[i] << "\"";
            if (i < result.pv_uci.size() - 1) std::cout << ",";
        }
        std::cout << "],\"score_cp\":" << result.score_cp << ",\"win_prob\":" << result.win_prob << ",\"depth\":" << result.depth << ",\"nodes\":" << result.nodes << ",\"time_ms\":" << result.time_ms << "}" << std::endl;

        std::cout << "Best: " << result.best_move_uci << "  PV: ";
        for (const auto& m : result.pv_uci) std::cout << m << " ";
        std::cout << "  Score: " << result.score_cp << "  WinProb: " << result.win_prob << std::endl;

        pos.make_move(get_move_from_uci(result.best_move_uci, pos));
    } else {
        // User makes first move
        std::cout << "Enter your first move (UCI): ";
        std::string user_move_str;
        std::getline(std::cin, user_move_str);
        Move user_move = get_move_from_uci(user_move_str, pos);
        if (user_move.value != 0) {
            pos.make_move(user_move);
            std::cout << "Applied your move: " << user_move_str << std::endl;

            // Engine's response
            SearchLimits limits;
            limits.movetime = 5000;
            limits.max_depth = 64;

            SearchResult result = search_position(pos, limits, &OPTIONS);

            std::cout << "{\"best_move\":\"" << result.best_move_uci << "\",\"pv\":[";
            for (size_t i = 0; i < result.pv_uci.size(); ++i) {
                std::cout << "\"" << result.pv_uci[i] << "\"";
                if (i < result.pv_uci.size() - 1) std::cout << ",";
            }
            std::cout << "],\"score_cp\":" << result.score_cp << ",\"win_prob\":" << result.win_prob << ",\"depth\":" << result.depth << ",\"nodes\":" << result.nodes << ",\"time_ms\":" << result.time_ms << "}" << std::endl;

            std::cout << "Best: " << result.best_move_uci << "  PV: ";
            for (const auto& m : result.pv_uci) std::cout << m << " ";
            std::cout << "  Score: " << result.score_cp << "  WinProb: " << result.win_prob << std::endl;

            pos.make_move(get_move_from_uci(result.best_move_uci, pos));
        } else {
            std::cout << "Invalid move. Exiting." << std::endl;
            return;
        }
    }

    std::cout << "Chess Wizard ready. Enter move or 'quit'." << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;

        // Try to parse as move
        Move move = get_move_from_uci(line, pos);
        if (move.value != 0) {
            pos.make_move(move);
            std::cout << "Applied move: " << line << std::endl;

            // Now suggest move
            SearchLimits limits;
            limits.movetime = 5000; // 5 seconds default
            limits.max_depth = 64;

            SearchResult result = search_position(pos, limits, &OPTIONS);

            // Print JSON
            std::cout << "{\"best_move\":\"" << result.best_move_uci << "\",\"pv\":[";
            for (size_t i = 0; i < result.pv_uci.size(); ++i) {
                std::cout << "\"" << result.pv_uci[i] << "\"";
                if (i < result.pv_uci.size() - 1) std::cout << ",";
            }
            std::cout << "],\"score_cp\":" << result.score_cp << ",\"win_prob\":" << result.win_prob << ",\"depth\":" << result.depth << ",\"nodes\":" << result.nodes << ",\"time_ms\":" << result.time_ms << "}" << std::endl;

            // Human-friendly line
            std::cout << "Best: " << result.best_move_uci << "  PV: ";
            for (const auto& m : result.pv_uci) std::cout << m << " ";
            std::cout << "  Score: " << result.score_cp << "  WinProb: " << result.win_prob << std::endl;

            // Apply engine's move for next input
            pos.make_move(get_move_from_uci(result.best_move_uci, pos));

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
            max_depth = std::max(max_depth, result.depth);

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