#include "engine.h"
#include "search.h"
#include "position.h"
#include "movegen.h"
#include "evaluate.h"
#include "tt.h"
#include "move.h"
#include "nnue.h"
#include "types.h"
#include "attack.h"
#include "book.h"
#include "syzygy.h"
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

// --- Global Options ---
ChessWizardOptions OPTIONS = {
    .use_nnue = false,
    .nnue_path = nullptr,
    .use_syzygy = false,
    .tb_paths = nullptr,
    .book_path = nullptr,
    .tt_size_mb = 32,
    .multi_pv = 1,
    .resign_threshold = 0.01,
    .seed = 0
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
extern "C" SearchResult chess_wizard_suggest_move(const char* fen_or_moves, uint32_t max_time_ms, uint8_t max_depth, const ChessWizardOptions* opts) {
    Position pos;
    pos.set_from_fen(std::string(fen_or_moves));

    SearchLimits limits;
    limits.movetime = max_time_ms;
    limits.max_depth = max_depth;

    return search_position(pos, limits, opts);
}
