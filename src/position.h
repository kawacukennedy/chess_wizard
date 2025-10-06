#pragma once

#include "types.h"
#include "bitboard.h"
#include <array>
#include <vector>

// Forward declarations
class Move;

// Helper function to get the color of a piece type
Color get_piece_color(PieceType pt);

// Function to get the king's square for a given color
Square get_king_square(const class Position& pos, Color color);

// Castling rights
enum CastlingRights : uint8_t {
    NO_CASTLING = 0,
    WHITE_KINGSIDE = 1 << 0,
    WHITE_QUEENSIDE = 1 << 1,
    BLACK_KINGSIDE = 1 << 2,
    BLACK_QUEENSIDE = 1 << 3,
    ALL_CASTLING = WHITE_KINGSIDE | WHITE_QUEENSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE
};

struct StateInfo {
    uint8_t from;
    uint8_t to;
    uint8_t moving_piece;
    uint8_t captured_piece;
    uint8_t promoted_piece;
    uint8_t flags;
    uint8_t prev_castle;
    uint8_t prev_ep_sq;
    uint16_t prev_halfmove;
    uint64_t prev_zobrist;
    int32_t eval_delta;
    uint8_t nnue_delta_count;
    // Padding to <=32 bytes
    uint8_t padding[1];
};

class Position {
public:
    std::array<Bitboard, 12> piece_bitboards; // 12 bitboards for each piece type
    Bitboard occupancy_bitboards[3]; // [WHITE, BLACK, ALL]
    Color side_to_move;
    CastlingRights castling_rights;
    Square en_passant_sq;
    int halfmove_clock;
    int fullmove_number;

    // Zobrist hash key for transposition table
    uint64_t hash_key;

    // History stack for unmake_move
    std::vector<StateInfo> history;

    Position();

    // Set up position from FEN string
    void set_from_fen(const std::string& fen);

    // Make and unmake moves
    void make_move(Move move);
    void unmake_move(Move move);

    // Make and unmake null moves
    void make_null_move();
    void unmake_null_move();

    // Check if square is attacked by a given color
    bool is_square_attacked(Square sq, Color by_color) const;

    // Check if the current side to move is in check
    bool is_check() const;

    // Get piece on a square
    PieceType piece_on_square(Square sq) const;

    // Print board (for debugging)
    void print_board() const;

    // Convert position to FEN string
    std::string to_fen_string() const;
};
