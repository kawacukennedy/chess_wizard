#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations
class Position;

// Square representation (0-63)
enum Square : int8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE
};

// Piece type representation (combines piece and color)
// Spec requires captured_piece = 15 for none.
enum PieceType : uint8_t {
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK,
    // 12, 13, 14 are unused
    NO_PIECE = 15
};

// Move representation (32-bit) as per specification
class Move {
public:
    uint32_t value;

    enum MoveFlag : uint8_t {
        NONE = 0,
        EN_PASSANT = 1 << 0,
        CASTLING = 1 << 1,
        DOUBLE_PAWN_PUSH = 1 << 2
    };

    enum PromotionType : uint8_t {
        NO_PROMOTION = 0,
        PROMOTION_N = 1,
        PROMOTION_B = 2,
        PROMOTION_R = 3,
        PROMOTION_Q = 4
    };

    Move() : value(0) {}
    Move(uint32_t val) : value(val) {}

    // Getters from spec
    Square from() const { return static_cast<Square>((value >> 0) & 0x3F); }
    Square to() const { return static_cast<Square>((value >> 6) & 0x3F); }
    PieceType moving_piece() const { return static_cast<PieceType>((value >> 12) & 0xF); }
    PieceType captured_piece() const { return static_cast<PieceType>((value >> 16) & 0xF); }
    PromotionType promotion() const { return static_cast<PromotionType>((value >> 20) & 0x7); }
    uint8_t flags() const { return static_cast<uint8_t>((value >> 23) & 0x7); }
    uint8_t ordering_hint() const { return static_cast<uint8_t>((value >> 26) & 0x3F); }

    // Setters
    void set_from(Square s) { value = (value & ~0x3F) | (static_cast<uint32_t>(s) << 0); }
    void set_to(Square s) { value = (value & ~(0x3F << 6)) | (static_cast<uint32_t>(s) << 6); }
    void set_moving_piece(PieceType p) { value = (value & ~(0xF << 12)) | (static_cast<uint32_t>(p) << 12); }
    void set_captured_piece(PieceType p) { value = (value & ~(0xF << 16)) | (static_cast<uint32_t>(p) << 16); }
    void set_promotion(PromotionType p) { value = (value & ~(0x7 << 20)) | (static_cast<uint32_t>(p) << 20); }
    void set_flags(uint8_t f) { value = (value & ~(0x7 << 23)) | (static_cast<uint32_t>(f) << 23); }
    void set_ordering_hint(uint8_t h) { value = (value & ~(0x3F << 26)) | (static_cast<uint32_t>(h) << 26); }

    // Helpers
    bool is_en_passant() const { return (flags() & EN_PASSANT); }
    bool is_castling() const { return (flags() & CASTLING); }
    bool is_double_push() const { return (flags() & DOUBLE_PAWN_PUSH); }
    bool is_capture() const { return captured_piece() != NO_PIECE; }
    bool is_promotion() const { return promotion() != NO_PROMOTION; }

    // Comparison
    bool operator==(const Move& other) const { return value == other.value; }
    bool operator!=(const Move& other) const { return value != other.value; }

    std::string to_uci_string() const;
    std::string to_san_string(const Position& pos) const;
};

using MoveList = std::vector<Move>;

// Max ply in search
const int MAX_PLY = 128;

// Search result info flags enum
enum InfoFlags : uint32_t {
    NO_INFO_FLAG = 0,
    BOOK = 1 << 0,
    TB = 1 << 1,
    CACHE = 1 << 2,
    MC_TIEBREAK = 1 << 3,
    RESIGN = 1 << 4,
    ERROR = 1 << 5
};

struct SearchResult {
    char best_move_uci[8];
    char* pv_json; // malloced JSON array of UCI strings, caller must free
    int32_t score_cp;
    double win_prob;
    double win_prob_stddev;
    uint8_t depth;
    uint64_t nodes;
    uint32_t time_ms;
    uint32_t info_flags; // bitmask using InfoFlags enum
    char* error_message; // optional error message, caller must free
};

struct ChessWizardOptions {
    bool use_nnue;
    const char *nnue_path;
    bool use_syzygy;
    const char **tb_paths;
    const char *book_path;
    uint32_t tt_size_mb;
    uint8_t multi_pv;
    double resign_threshold;
    uint64_t seed;
};

struct SearchLimits {
    int movetime; // max time in ms
    int max_depth; // max search depth
};

// File and Rank representation
enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, NO_FILE
};

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, NO_RANK
};

enum Color {
    WHITE,
    BLACK,
    BOTH,
    NO_COLOR
};

// Helper function to get color of a piece
inline Color get_piece_color(PieceType pt) {
    return (pt >= BP) ? BLACK : WHITE;
}

// Helper functions for square
inline int file_of(Square s) { return s % 8; }
inline int rank_of(Square s) { return s / 8; }

enum GenericPieceType {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    NO_GENERIC_PIECE
};

// Starting FEN string
const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Transposition Table flags
enum TTFlag {
    TT_EXACT = 0,  // Exact score
    TT_LOWER = 1,  // Alpha cutoff, true value is >= value
    TT_UPPER = 2   // Beta cutoff, true value is <= value
};

#endif // TYPES_H
