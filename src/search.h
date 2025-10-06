#ifndef SEARCH_H
#define SEARCH_H

#include "position.h"
#include "types.h"
#include <vector>
#include <tuple>

// Main search function
SearchResult search_position(Position& pos, const SearchLimits& limits, const ChessWizardOptions* opts);

// Perft test function
uint64_t perft(int depth, Position& pos);
uint64_t perft_debug(int depth, Position& pos, int original_depth);

// Helper functions
bool is_draw(const Position& pos);
double sigmoid_win_prob(int cp_score);
void check_time();
void clear_search_globals();

// Search functions
int search(int alpha, int beta, int depth, int ply, Position& pos, bool do_null = true);
int quiescence(int alpha, int beta, int ply, Position& pos);

// Monte Carlo rollout
std::tuple<int, int, int> rollout(Position pos, int max_depth);

// Move ordering
int score_move(Move move, int ply, Move tt_move, const Position& pos);
void order_moves(MoveList& moves, int ply, Move tt_move, const Position& pos);

// Search globals (to be initialized per search)
extern SearchLimits Limits;
extern Position RootPosition;
extern uint64_t NodeCount;
extern std::atomic<bool> StopSearch;

// PV table
extern Move PV_TABLE[MAX_PLY + 1][MAX_PLY + 1];
extern int PV_LENGTH[MAX_PLY + 1];
extern Move KILLER_MOVES[MAX_PLY + 1][2];

// History heuristic
extern int HISTORY_TABLE[12][64];

#endif // SEARCH_H