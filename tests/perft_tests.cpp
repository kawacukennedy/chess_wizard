#include "../src/engine/board.h"
#include "../src/engine/movegen.h"
#include <iostream>

uint64_t perft(Position& pos, int depth) {
    if (depth == 0) return 1;
    MoveList moves;
    generate_legal_moves(pos, moves);
    uint64_t nodes = 0;
    for (const auto& move : moves) {
        pos.make_move(move);
        nodes += perft(pos, depth - 1);
        pos.unmake_move(move);
    }
    return nodes;
}

int main() {
    Position pos;
    pos.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << "Perft 1: " << perft(pos, 1) << std::endl;
    // Add more depths
    return 0;
}