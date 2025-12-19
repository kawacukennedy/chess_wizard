#pragma once

#include "position.h"

struct TBResult {
    Move best_move;
    int score;
    int dtz;
};

bool probe_syzygy(Position& pos, TBResult& result);
