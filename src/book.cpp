#include "book.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// Polyglot books are big-endian, so we need a byte-swapping function
uint64_t swap_uint64(uint64_t n) {
    n = ((n & 0x00000000FFFFFFFF) << 32) | ((n & 0xFFFFFFFF00000000) >> 32);
    n = ((n & 0x0000FFFF0000FFFF) << 16) | ((n & 0xFFFF0000FFFF0000) >> 16);
    n = ((n & 0x00FF00FF00FF00FF) << 8) | ((n & 0xFF00FF00FF00FF00) >> 8);
    return n;
}

uint16_t swap_uint16(uint16_t n) {
    return (n << 8) | (n >> 8);
}

bool Book::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "info string Book: file not found: " << path << std::endl;
        return false;
    }

    BookEntry entry;
    while (file.read(reinterpret_cast<char*>(&entry), sizeof(BookEntry))) {
        entry.key = swap_uint64(entry.key);
        entry.move = swap_uint16(entry.move);
        entry.weight = swap_uint16(entry.weight);
        entries.push_back(entry);
    }

    std::cout << "info string Book: loaded " << entries.size() << " entries." << std::endl;
    return true;
}

Move Book::get_move(uint64_t hash) {
    const BookEntry* best_entry = nullptr;
    uint16_t max_weight = 0;

    // Find all moves for the given hash and select the one with the highest weight
    for (const auto& entry : entries) {
        if (entry.key == hash && entry.weight > max_weight) {
            max_weight = entry.weight;
            best_entry = &entry;
        }
    }

    if (!best_entry) {
        return Move(0);
    }

    // Decode Polyglot move format
    uint16_t poly_move = best_entry->move;
    int from_file = (poly_move >> 6) & 7;
    int from_rank = (poly_move >> 9) & 7;
    int to_file = (poly_move >> 0) & 7;
    int to_rank = (poly_move >> 3) & 7;
    int promotion_piece = (poly_move >> 12) & 7;

    Square from_sq = static_cast<Square>(from_rank * 8 + from_file);
    Square to_sq = static_cast<Square>(to_rank * 8 + to_file);

    Move::PromotionType promo_type = Move::NO_PROMOTION;
    if (promotion_piece != 0) {
        switch (promotion_piece) {
            case 1: promo_type = Move::KNIGHT_PROMOTION; break;
            case 2: promo_type = Move::BISHOP_PROMOTION; break;
            case 3: promo_type = Move::ROOK_PROMOTION; break;
            case 4: promo_type = Move::QUEEN_PROMOTION; break;
        }
    }

    // We need the full position to create a valid move with all flags.
    // The book does not store this information. For now, we create a simple move.
    // This will fail for castling, en-passant, and captures.
    // A full implementation would need to generate legal moves and match.
    Move m;
    m.set_from(from_sq);
    m.set_to(to_sq);
    m.set_promotion(promo_type);

    return m;
}
