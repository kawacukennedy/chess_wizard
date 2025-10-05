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

#include "book.h"

// --- Global Options ---
ChessWizardOptions OPTIONS = {
    .use_nnue = false,
    .nnue_path = "",
    .use_tb = false,
    .tb_paths = nullptr,
    .book_path = "",
    .tt_size_mb = 32,
    .multi_pv = 1,
    .resign_threshold = 0.01
};

Book OPENING_BOOK;
std::string NNUE_PATH_BUFFER;
std::string BOOK_PATH_BUFFER;

void init_all() {
    init_attacks();
    init_zobrist_keys();
    TT.resize(OPTIONS.tt_size_mb);
}

// --- C-API Implementation ---
extern "C" SearchResult chess_wizard_suggest_move(const char* fen_or_moves, int max_time_ms, int max_depth, const ChessWizardOptions* opts) {
    Position pos;
    pos.set_from_fen(std::string(fen_or_moves));

    SearchLimits limits;
    limits.movetime = max_time_ms;
    limits.max_depth = max_depth;

    return search_position(pos, limits, opts);
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
            uint64_t nodes = perft(depth, pos);
            std::cout << "Perft " << depth << ": " << nodes << std::endl;
        } else if (token == "quit") {
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    init_all();
    
    // Simple CLI mode check
    if (argc > 1 && std::string(argv[1]) == "bench") {
        // TODO: Implement benchmark from spec
        std::cout << "Benchmark mode not implemented yet." << std::endl;
        return 0;
    }

    uci_loop();
    return 0;
}