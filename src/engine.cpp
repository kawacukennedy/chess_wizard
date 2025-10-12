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
    // init_zobrist_keys called separately with seed
    TT.resize(32); // default
}


