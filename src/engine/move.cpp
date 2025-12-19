#include "move.h"
#include "position.h"
#include "movegen.h"
#include <cctype>
#include <algorithm>

// Helper function to convert a Square to a string (e.g., A1 -> "a1")
std::string square_to_string(Square sq) {
    std::string s = "  ";
    s[0] = (char)('a' + (sq % 8));
    s[1] = (char)('1' + (sq / 8));
    return s;
}

// Convert PromotionType to char for promotion (e.g., QUEEN_PROMOTION -> 'q')
char promotion_type_to_char(Move::PromotionType pt) {
    switch (pt) {
        case Move::PROMOTION_N: return 'n';
        case Move::PROMOTION_B: return 'b';
        case Move::PROMOTION_R: return 'r';
        case Move::PROMOTION_Q: return 'q';
        default: return ' '; // Should not happen for non-promotions
    }
}

std::string Move::to_uci_string() const {
    std::string uci = square_to_string(from()) + square_to_string(to());
    if (is_promotion()) {
        uci += promotion_type_to_char(promotion());
    }
    return uci;
}

std::string Move::to_san_string(const Position& pos) const {
    PieceType pt = moving_piece();
    int generic = pt % 6;
    char piece_char = " PNBRQK"[generic];
    std::string san;
    if (generic != 0) san += piece_char; // not pawn

    // Disambiguation
    Position temp_pos = pos;
    MoveList legal_moves;
    generate_legal_moves(temp_pos, legal_moves);
    std::vector<Move> candidates;
    for (const auto& m : legal_moves) {
        if (m.to() == to() && (m.moving_piece() % 6) == generic) {
            candidates.push_back(m);
        }
    }
    if (candidates.size() > 1) {
        // Need disambiguation
        char from_file = 'a' + (from() % 8);
        char from_rank = '1' + (from() / 8);
        bool need_file = false, need_rank = false;
        for (const auto& m : candidates) {
            if (m.from() % 8 != from() % 8) need_file = true;
            if (m.from() / 8 != from() / 8) need_rank = true;
        }
        if (need_file) san += from_file;
        if (need_rank) san += from_rank;
    }

    if (is_capture()) san += 'x';
    san += square_to_string(to());
    if (is_promotion()) {
        san += '=';
        san += toupper(promotion_type_to_char(promotion()));
    }

    // Check if gives check or mate
    temp_pos.make_move(*this);
    if (temp_pos.is_check()) {
        MoveList after_moves;
        generate_legal_moves(temp_pos, after_moves);
        if (after_moves.empty()) {
            san += '#';
        } else {
            san += '+';
        }
    }

    return san;
}

// Function to get a move from a UCI string
Move get_move_from_uci(const std::string& uci_str, const Position& pos) {
    if (uci_str.length() < 4) {
        return Move(0); // Invalid UCI string length
    }

    // Parse from and to squares
    Square from_sq = (Square)((uci_str[1] - '1') * 8 + (uci_str[0] - 'a'));
    Square to_sq = (Square)((uci_str[3] - '1') * 8 + (uci_str[2] - 'a'));

    // Determine promoted piece (if any)
    Move::PromotionType promoted_type = Move::NO_PROMOTION;
    if (uci_str.length() == 5) {
        char promo_char = uci_str[4];
        switch (promo_char) {
            case 'n': promoted_type = Move::PROMOTION_N; break;
            case 'b': promoted_type = Move::PROMOTION_B; break;
            case 'r': promoted_type = Move::PROMOTION_R; break;
            case 'q': promoted_type = Move::PROMOTION_Q; break;
            default: return Move(0); // Invalid promotion character
        }
    }

    // Generate all legal moves for the current position
    // Note: This requires a non-const Position object for make_move/unmake_move
    // We'll make a copy to avoid modifying the original const Position
    Position temp_pos = pos;
    MoveList legal_moves;
    generate_legal_moves(temp_pos, legal_moves);

    // Find the matching legal move
    for (const auto& move : legal_moves) {
        if (move.from() == from_sq && move.to() == to_sq) {
            // For promotion moves, also check the promoted piece
            if (promoted_type != Move::NO_PROMOTION) {
                if (move.promotion() == promoted_type) {
                    return move;
                }
            } else { // Non-promotion move
                if (!move.is_promotion()) { // Ensure it's not a promotion move
                    return move;
                }
            }
        }
    }

    return Move(0); // No matching legal move found
}