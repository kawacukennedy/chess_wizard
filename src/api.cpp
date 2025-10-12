#include "types.h"
#include "board.h"
#include "search.h"
#include "tt.h"
#include "nnue.h"
#include "book.h"
#include "tb_probe.h"
#include "zobrist.h"
#include "movegen.h"
#include <cstring>
#include <cstdlib>
#include <vector>

extern ChessWizardOptions OPTIONS;
extern Book OPENING_BOOK;
extern std::string NNUE_PATH_BUFFER;
extern std::string BOOK_PATH_BUFFER;
extern void init_all();

void free_api_memory(void* ptr) {
    free(ptr);
}

extern "C" SearchResult chess_wizard_suggest_move(const char* fen, uint32_t max_time_ms, uint8_t max_depth, const ChessWizardOptions* opts) {
    // Initialize if not done
    static bool initialized = false;
    if (!initialized) {
        init_all();
        initialized = true;
    }

    // Copy options
    ChessWizardOptions local_opts = *opts;
    OPTIONS = local_opts;

    // Set NNUE and book paths
    if (local_opts.nnue_path) {
        NNUE_PATH_BUFFER = local_opts.nnue_path;
        NNUE::nnue_available = NNUE::nnue_evaluator.init(NNUE_PATH_BUFFER.c_str());
    }
    if (local_opts.book_path) {
        BOOK_PATH_BUFFER = local_opts.book_path;
        OPENING_BOOK.load(BOOK_PATH_BUFFER);
    }

    // Resize TT
    TT.resize(local_opts.tt_size_mb);

    // Set up position
    Position pos;
    pos.set_from_fen(fen);

    // Check cache (but since no global cache here, skip for now)

    // Check book
    SearchResult result = {};
    if (local_opts.book_path && OPENING_BOOK.is_loaded()) {
        Move book_move = OPENING_BOOK.get_move(pos.hash_key);
        if (book_move.value != 0) {
            // Assume qualifies
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

    // Check TB
    // TODO: implement TB probe

    // Search
    SearchLimits limits;
    limits.movetime = max_time_ms;
    limits.max_depth = max_depth;
    result = search_position(pos, limits, &local_opts);

    // Ensure move is legal
    MoveList legal_moves;
    generate_legal_moves(pos, legal_moves);
    bool is_legal = false;
    for (const auto& m : legal_moves) {
        if (m.to_uci_string() == std::string(result.best_move_uci)) {
            is_legal = true;
            break;
        }
    }
    if (!is_legal && !legal_moves.empty()) {
        // Fallback to highest scored legal
        // For simplicity, take first legal
        std::string fallback_uci = legal_moves[0].to_uci_string();
        strncpy(result.best_move_uci, fallback_uci.c_str(), 7);
        // Update pv_json
        free(result.pv_json);
        std::string json = "[\"" + fallback_uci + "\"]";
        result.pv_json = (char*)malloc(json.size() + 1);
        strcpy(result.pv_json, json.c_str());
    }

    return result;
}