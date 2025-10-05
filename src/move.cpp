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
        case Move::KNIGHT_PROMOTION: return 'n';
        case Move::BISHOP_PROMOTION: return 'b';
        case Move::ROOK_PROMOTION: return 'r';
        case Move::QUEEN_PROMOTION: return 'q';
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
            case 'n': promoted_type = Move::KNIGHT_PROMOTION; break;
            case 'b': promoted_type = Move::BISHOP_PROMOTION; break;
            case 'r': promoted_type = Move::ROOK_PROMOTION; break;
            case 'q': promoted_type = Move::QUEEN_PROMOTION; break;
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