#pragma once

#include "types.h"
#include <string>

// Forward declaration
class Position;

// Helper function to convert a Square to a string (e.g., A1 -> "a1")
std::string square_to_string(Square sq);

// Helper function to convert PieceType to promotion value (1=N,2=B,3=R,4=Q)
inline uint8_t piece_type_to_promotion_val(PieceType pt) {
    switch (pt) {
        case WN: case BN: return Move::PROMOTION_N;
        case WB: case BB: return Move::PROMOTION_B;
        case WR: case BR: return Move::PROMOTION_R;
        case WQ: case BQ: return Move::PROMOTION_Q;
        default: return Move::NO_PROMOTION; // No promotion
    }
}

// Helper function to convert promotion value to PieceType
// This function needs the color to determine the correct piece type (e.g., WN vs BN)
inline PieceType promotion_val_to_piece_type(uint8_t val, Color color) {
    if (color == WHITE) {
        switch (val) {
            case Move::PROMOTION_N: return WN;
            case Move::PROMOTION_B: return WB;
            case Move::PROMOTION_R: return WR;
            case Move::PROMOTION_Q: return WQ;
            default: return NO_PIECE;
        }
    } else { // BLACK
        switch (val) {
            case Move::PROMOTION_N: return BN;
            case Move::PROMOTION_B: return BB;
            case Move::PROMOTION_R: return BR;
            case Move::PROMOTION_Q: return BQ;
            default: return NO_PIECE;
        }
    }
}

// Helper function to create a move
inline Move create_move(Square from, Square to, PieceType piece, PieceType captured = NO_PIECE, uint8_t promotion_val = Move::NO_PROMOTION, uint8_t flags = Move::NONE) {
    Move m;
    m.set_from(from);
    m.set_to(to);
    m.set_moving_piece(piece);
    m.set_captured_piece(captured);
    m.set_promotion(static_cast<Move::PromotionType>(promotion_val));
    m.set_flags(flags);
    return m;
}

// Function to get a move from a UCI string
Move get_move_from_uci(const std::string& uci_str, const Position& pos);