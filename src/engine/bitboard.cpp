#include "bitboard.h"
#include <iostream>

// Bitmasks for files and ranks
Bitboard FILE_MASKS[8];
Bitboard RANK_MASKS[8];

void init_bitboard_masks() {
    for (int file = 0; file < 8; ++file) {
        FILE_MASKS[file] = 0ULL;
        for (int rank = 0; rank < 8; ++rank) {
            set_bit(FILE_MASKS[file], (Square)(rank * 8 + file));
        }
    }

    for (int rank = 0; rank < 8; ++rank) {
        RANK_MASKS[rank] = 0ULL;
        for (int file = 0; file < 8; ++file) {
            set_bit(RANK_MASKS[rank], (Square)(rank * 8 + file));
        }
    }
}

void print_bitboard(Bitboard bb) {
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            Square sq = (Square)(rank * 8 + file);
            std::cout << (get_bit(bb, sq) ? "1 " : "0 ");
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
