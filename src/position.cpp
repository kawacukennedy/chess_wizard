#include "position.h"
#include "zobrist.h"
#include "attack.h"
#include "bitboard.h"
#include "move.h"
#include <iostream>
#include <sstream>
#include <cctype>



// Helper function to convert PieceType to char
char piece_to_char(PieceType pt) {
    switch (pt) {
        case WP: return 'P';
        case WN: return 'N';
        case WB: return 'B';
        case WR: return 'R';
        case WQ: return 'Q';
        case WK: return 'K';
        case BP: return 'p';
        case BN: return 'n';
        case BB: return 'b';
        case BR: return 'r';
        case BQ: return 'q';
        case BK: return 'k';
        default: return ' ';
    }
}

// Helper function to convert char to PieceType from FEN
PieceType char_to_piece_type_from_fen(char c) {
    switch (c) {
        case 'P': return WP;
        case 'N': return WN;
        case 'B': return WB;
        case 'R': return WR;
        case 'Q': return WQ;
        case 'K': return WK;
        case 'p': return BP;
        case 'n': return BN;
        case 'b': return BB;
        case 'r': return BR;
        case 'q': return BQ;
        case 'k': return BK;
        default: return NO_PIECE;
    }
}

// Function to get the king's square for a given color
Square get_king_square(const Position& pos, Color color) {
    Bitboard king_bb = pos.piece_bitboards[color == WHITE ? WK : BK];
    return static_cast<Square>(lsb_index(king_bb));
}

Position::Position() :
    piece_bitboards{},
    occupancy_bitboards{},
    side_to_move(WHITE),
    castling_rights(NO_CASTLING),
    en_passant_sq(NO_SQUARE),
    halfmove_clock(0),
    fullmove_number(1),
    hash_key(0),
    history()
{
}

void Position::set_from_fen(const std::string& fen) {
    // Reset board to initial state
    piece_bitboards = {};
    occupancy_bitboards[0] = 0;
    occupancy_bitboards[1] = 0;
    occupancy_bitboards[2] = 0;
    side_to_move = WHITE;
    castling_rights = NO_CASTLING;
    en_passant_sq = NO_SQUARE;
    halfmove_clock = 0;
    fullmove_number = 1;
    hash_key = 0;

    std::stringstream ss(fen);
    std::string board_str, side_str, castling_str, enpassant_str, halfmove_str, fullmove_str;

    ss >> board_str >> side_str >> castling_str >> enpassant_str >> halfmove_str >> fullmove_str;

    // Parse board_str
    int rank = 7; // Start from 8th rank
    int file = 0; // Start from 'a' file
    for (char c : board_str) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += (c - '0');
        } else {
            PieceType pt = char_to_piece_type_from_fen(c);
            Square sq = (Square)(rank * 8 + file);
            set_bit(piece_bitboards[pt], sq);
            set_bit(occupancy_bitboards[get_piece_color(pt)], sq);
            set_bit(occupancy_bitboards[BOTH], sq);
            hash_key ^= Zobrist.piece_keys[pt][sq];
            file++;
        }
    }

    // Parse side_str
    side_to_move = (side_str == "w") ? WHITE : BLACK;
    if (side_to_move == BLACK) {
        hash_key ^= Zobrist.side_to_move_key;
    }

    // Parse castling_str
    for (char c : castling_str) {
        switch (c) {
            case 'K': castling_rights = (CastlingRights)(castling_rights | WHITE_KINGSIDE); break;
            case 'Q': castling_rights = (CastlingRights)(castling_rights | WHITE_QUEENSIDE); break;
            case 'k': castling_rights = (CastlingRights)(castling_rights | BLACK_KINGSIDE); break;
            case 'q': castling_rights = (CastlingRights)(castling_rights | BLACK_QUEENSIDE); break;
            default: break;
        }
    }
    hash_key ^= Zobrist.castling_keys[castling_rights];

    // Parse enpassant_str
    if (enpassant_str != "-") {
        // Convert algebraic notation to square
        file = enpassant_str[0] - 'a';
        rank = enpassant_str[1] - '1';
        en_passant_sq = (Square)(rank * 8 + file);
        hash_key ^= Zobrist.en_passant_keys[get_file(en_passant_sq)];
    }

    // Parse halfmove_str
    halfmove_clock = std::stoi(halfmove_str);

    // Parse fullmove_str
    fullmove_number = std::stoi(fullmove_str);
}

void Position::print_board() const {
    std::cout << "\n+---+---+---+---+---+---+---+---+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << "| ";
        for (int file = 0; file < 8; ++file) {
            Square sq = (Square)(rank * 8 + file);
            PieceType pt = piece_on_square(sq);
            std::cout << piece_to_char(pt) << " | ";
        }
        std::cout << rank + 1 << "\n+---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "  a   b   c   d   e   f   g   h\n";
    std::cout << "Side to move: " << (side_to_move == WHITE ? "White" : "Black") << std::endl;
    std::cout << "Castling rights: ";
    if (castling_rights & WHITE_KINGSIDE) std::cout << "K";
    if (castling_rights & WHITE_QUEENSIDE) std::cout << "Q";
    if (castling_rights & BLACK_KINGSIDE) std::cout << "k";
    if (castling_rights & BLACK_QUEENSIDE) std::cout << "q";
    if (castling_rights == NO_CASTLING) std::cout << "-";
    std::cout << std::endl;
    if (en_passant_sq != NO_SQUARE) {
        std::cout << (char)('a' + (en_passant_sq % 8)) << (char)('1' + (en_passant_sq / 8));
    } else {
        std::cout << "-";
    }
    std::cout << std::endl;
    std::cout << "Halfmove clock: " << halfmove_clock << std::endl;
    std::cout << "Fullmove number: " << fullmove_number << std::endl;
    std::cout << "Hash key: " << std::hex << hash_key << std::dec << std::endl;
}

PieceType Position::piece_on_square(Square sq) const {
    for (int pt = WP; pt <= BK; ++pt) {
        if (get_bit(piece_bitboards[pt], sq)) {
            return (PieceType)pt;
        }
    }
    return NO_PIECE;
}

bool Position::is_square_attacked(Square sq, Color by_color) const {
    // Check for pawn attacks
    if (by_color == WHITE) {
        if (get_rank(sq) > RANK_1) {
            if (get_file(sq) > FILE_A && get_bit(piece_bitboards[WP], (Square)(sq - 9))) return true;
            if (get_file(sq) < FILE_H && get_bit(piece_bitboards[WP], (Square)(sq - 7))) return true;
        }
    } else { // by_color == BLACK
        if (get_rank(sq) < RANK_8) {
            if (get_file(sq) > FILE_A && (sq + 7) < 64 && get_bit(piece_bitboards[BP], (Square)(sq + 7))) return true;
            if (get_file(sq) < FILE_H && (sq + 9) < 64 && get_bit(piece_bitboards[BP], (Square)(sq + 9))) return true;
        }
    }

    // Check for knight attacks
    if (KNIGHT_ATTACKS[sq] & (by_color == WHITE ? piece_bitboards[WN] : piece_bitboards[BN])) return true;

    // Check for king attacks
    if (KING_ATTACKS[sq] & (by_color == WHITE ? piece_bitboards[WK] : piece_bitboards[BK])) return true;

    // Blockers exclude the piece on sq
    Bitboard occ = occupancy_bitboards[BOTH] & ~(1ULL << sq);

    // Check for bishop/queen attacks
    Bitboard bishop_queen_attackers = (by_color == WHITE ? (piece_bitboards[WB] | piece_bitboards[WQ]) : (piece_bitboards[BB] | piece_bitboards[BQ]));
    while (bishop_queen_attackers) {
        Square attacker_sq = pop_bit(bishop_queen_attackers);
        if (get_bishop_attacks(attacker_sq, occ) & (1ULL << sq)) return true;
    }

    // Check for rook/queen attacks
    Bitboard rook_queen_attackers = (by_color == WHITE ? (piece_bitboards[WR] | piece_bitboards[WQ]) : (piece_bitboards[BR] | piece_bitboards[BQ]));
    while (rook_queen_attackers) {
        Square attacker_sq = pop_bit(rook_queen_attackers);
        if (get_rook_attacks(attacker_sq, occ) & (1ULL << sq)) return true;
    }

    return false;
}

bool Position::is_check() const {
    Square king_sq = get_king_square(*this, side_to_move);
    Color opp_color = (side_to_move == WHITE) ? BLACK : WHITE;
    return is_square_attacked(king_sq, opp_color);
}

std::string Position::to_fen_string() const {
    std::string fen = "";

    // Piece placement
    for (int rank = 7; rank >= 0; --rank) {
        int empty_squares = 0;
        for (int file = 0; file < 8; ++file) {
            Square sq = (Square)(rank * 8 + file);
            PieceType pt = piece_on_square(sq);
            if (pt == NO_PIECE) {
                empty_squares++;
            } else {
                if (empty_squares > 0) {
                    fen += std::to_string(empty_squares);
                    empty_squares = 0;
                }
                fen += piece_to_char(pt);
            }
        }
        if (empty_squares > 0) {
            fen += std::to_string(empty_squares);
        }
        if (rank > 0) {
            fen += "/";
        }
    }

    // Side to move
    fen += (side_to_move == WHITE) ? " w" : " b";

    // Castling rights
    std::string castling_str = "";
    if (castling_rights & WHITE_KINGSIDE) castling_str += "K";
    if (castling_rights & WHITE_QUEENSIDE) castling_str += "Q";
    if (castling_rights & BLACK_KINGSIDE) castling_str += "k";
    if (castling_rights & BLACK_QUEENSIDE) castling_str += "q";
    if (castling_str == "") castling_str = "-";
    fen += " " + castling_str;

    if (en_passant_sq != NO_SQUARE) {
        fen += " " + square_to_string(en_passant_sq);
    } else {
        fen += " -";
    }

    // Halfmove clock
    fen += " " + std::to_string(halfmove_clock);

    // Fullmove number
    fen += " " + std::to_string(fullmove_number);

    return fen;
}

void Position::make_move(Move move) {
    // Update Zobrist hash for side to move
    Square from_sq = move.from();
    Square to_sq = move.to();
    PieceType piece_moved = move.moving_piece();
    PieceType captured_piece = move.captured_piece();
    Move::PromotionType promoted_type = move.promotion();
    uint8_t flags = move.flags();



    // Save current state for unmake_move
    StateInfo si = {};
    si.from = from_sq;
    si.to = to_sq;
    si.moving_piece = piece_moved;
    si.captured_piece = captured_piece;
    si.promoted_piece = promoted_type == Move::NO_PROMOTION ? NO_PIECE : promotion_val_to_piece_type(promoted_type, side_to_move);
    si.flags = flags;
    si.prev_castle = castling_rights;
    si.prev_ep_sq = en_passant_sq;
    si.prev_halfmove = halfmove_clock;
    si.prev_zobrist = hash_key;
    si.eval_delta = 0; // TODO: compute incremental eval delta
    si.nnue_delta_count = 0; // TODO: nnue deltas
    history.push_back(si);

    // Update halfmove clock (reset on pawn move or capture)
    if (piece_moved == WP || piece_moved == BP || captured_piece != NO_PIECE || (flags & Move::EN_PASSANT)) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }

    // Remove piece from 'from' square
    clear_bit(piece_bitboards[piece_moved], from_sq);
    clear_bit(occupancy_bitboards[side_to_move], from_sq);
    clear_bit(occupancy_bitboards[BOTH], from_sq);
    hash_key ^= Zobrist.piece_keys[piece_moved][from_sq];

    if (captured_piece != NO_PIECE && !(flags & Move::EN_PASSANT)) {
        clear_bit(piece_bitboards[captured_piece], to_sq);
        clear_bit(occupancy_bitboards[(side_to_move == WHITE ? BLACK : WHITE)], to_sq);
        clear_bit(occupancy_bitboards[BOTH], to_sq);
        hash_key ^= Zobrist.piece_keys[captured_piece][to_sq];
    }

    // Place piece on 'to' square
    set_bit(piece_bitboards[piece_moved], to_sq);
    set_bit(occupancy_bitboards[side_to_move], to_sq);
    set_bit(occupancy_bitboards[BOTH], to_sq);
    hash_key ^= Zobrist.piece_keys[piece_moved][to_sq];

    // Handle promotion
    if (move.is_promotion()) {
        clear_bit(piece_bitboards[piece_moved], to_sq); // Remove pawn
        hash_key ^= Zobrist.piece_keys[piece_moved][to_sq];

        set_bit(piece_bitboards[promotion_val_to_piece_type(promoted_type, side_to_move)], to_sq); // Add promoted piece
        hash_key ^= Zobrist.piece_keys[promotion_val_to_piece_type(promoted_type, side_to_move)][to_sq];
    }

    // Handle castling
    if (flags & Move::CASTLING) {
        if (side_to_move == WHITE) {
            if (to_sq == G1) { // White kingside castling
                clear_bit(piece_bitboards[WR], H1);
                clear_bit(occupancy_bitboards[WHITE], H1);
                clear_bit(occupancy_bitboards[BOTH], H1);
                hash_key ^= Zobrist.piece_keys[WR][H1];

                set_bit(piece_bitboards[WR], F1);
                set_bit(occupancy_bitboards[WHITE], F1);
                set_bit(occupancy_bitboards[BOTH], F1);
                hash_key ^= Zobrist.piece_keys[WR][F1];
            } else if (to_sq == C1) { // White queenside castling
                clear_bit(piece_bitboards[WR], A1);
                clear_bit(occupancy_bitboards[WHITE], A1);
                clear_bit(occupancy_bitboards[BOTH], A1);
                hash_key ^= Zobrist.piece_keys[WR][A1];

                set_bit(piece_bitboards[WR], D1);
                set_bit(occupancy_bitboards[WHITE], D1);
                set_bit(occupancy_bitboards[BOTH], D1);
                hash_key ^= Zobrist.piece_keys[WR][D1];
            }
        } else { // Black
            if (to_sq == G8) { // Black kingside castling
                clear_bit(piece_bitboards[BR], H8);
                clear_bit(occupancy_bitboards[BLACK], H8);
                clear_bit(occupancy_bitboards[BOTH], H8);
                hash_key ^= Zobrist.piece_keys[BR][H8];

                set_bit(piece_bitboards[BR], F8);
                set_bit(occupancy_bitboards[BLACK], F8);
                set_bit(occupancy_bitboards[BOTH], F8);
                hash_key ^= Zobrist.piece_keys[BR][F8];
            } else if (to_sq == C8) { // Black queenside castling
                clear_bit(piece_bitboards[BR], A8);
                clear_bit(occupancy_bitboards[BLACK], A8);
                clear_bit(occupancy_bitboards[BOTH], A8);
                hash_key ^= Zobrist.piece_keys[BR][A8];

                set_bit(piece_bitboards[BR], D8);
                set_bit(occupancy_bitboards[BLACK], D8);
                set_bit(occupancy_bitboards[BOTH], D8);
                hash_key ^= Zobrist.piece_keys[BR][D8];
            }
        }
    }

    // Handle en passant capture
    if (flags & Move::EN_PASSANT) {
        Square captured_pawn_sq = (Square)((side_to_move == WHITE) ? (to_sq - 8) : (to_sq + 8));
        PieceType pawn_type = (side_to_move == WHITE) ? BP : WP;

        clear_bit(piece_bitboards[pawn_type], captured_pawn_sq);
        clear_bit(occupancy_bitboards[(side_to_move == WHITE ? BLACK : WHITE)], captured_pawn_sq);
        clear_bit(occupancy_bitboards[BOTH], captured_pawn_sq);
        hash_key ^= Zobrist.piece_keys[pawn_type][captured_pawn_sq];
    }

    // Update en passant square
    if (en_passant_sq != NO_SQUARE) {
        hash_key ^= Zobrist.en_passant_keys[get_file(en_passant_sq)];
    }
    if (flags & Move::DOUBLE_PAWN_PUSH) {
        en_passant_sq = (Square)((side_to_move == WHITE) ? (to_sq - 8) : (to_sq + 8));
        hash_key ^= Zobrist.en_passant_keys[get_file(en_passant_sq)];
    } else {
        en_passant_sq = NO_SQUARE;
    }

    // Update castling rights
    hash_key ^= Zobrist.castling_keys[castling_rights]; // Remove old castling rights from hash

    if (piece_moved == WK || from_sq == E1) {
        castling_rights = (CastlingRights)(castling_rights & ~(WHITE_KINGSIDE | WHITE_QUEENSIDE));
    }
    if (piece_moved == BK || from_sq == E8) {
        castling_rights = (CastlingRights)(castling_rights & ~(BLACK_KINGSIDE | BLACK_QUEENSIDE));
    }
    if (piece_moved == WR) {
        if (from_sq == H1) castling_rights = (CastlingRights)(castling_rights & ~WHITE_KINGSIDE);
        if (from_sq == A1) castling_rights = (CastlingRights)(castling_rights & ~WHITE_QUEENSIDE);
    }
    if (piece_moved == BR) {
        if (from_sq == H8) castling_rights = (CastlingRights)(castling_rights & ~BLACK_KINGSIDE);
        if (from_sq == A8) castling_rights = (CastlingRights)(castling_rights & ~BLACK_QUEENSIDE);
    }
    if (captured_piece == WR) {
        if (to_sq == H1) castling_rights = (CastlingRights)(castling_rights & ~WHITE_KINGSIDE);
        if (to_sq == A1) castling_rights = (CastlingRights)(castling_rights & ~WHITE_QUEENSIDE);
    }
    if (captured_piece == BR) {
        if (to_sq == H8) castling_rights = (CastlingRights)(castling_rights & ~BLACK_KINGSIDE);
        if (to_sq == A8) castling_rights = (CastlingRights)(castling_rights & ~BLACK_QUEENSIDE);
    }

    hash_key ^= Zobrist.castling_keys[castling_rights]; // Add new castling rights to hash

    if (side_to_move == BLACK) {
        fullmove_number++;
    }

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;
}

void Position::make_null_move() {
    StateInfo si = {};
    si.from = NO_SQUARE;
    si.to = NO_SQUARE;
    si.moving_piece = NO_PIECE;
    si.captured_piece = NO_PIECE;
    si.promoted_piece = NO_PIECE;
    si.flags = 0;
    si.prev_castle = castling_rights;
    si.prev_ep_sq = en_passant_sq;
    si.prev_halfmove = halfmove_clock;
    si.prev_zobrist = hash_key;
    si.eval_delta = 0;
    si.nnue_delta_count = 0;
    history.push_back(si);

    hash_key ^= Zobrist.side_to_move_key;

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;

    halfmove_clock++;

    if (en_passant_sq != NO_SQUARE) {
        hash_key ^= Zobrist.en_passant_keys[get_file(en_passant_sq)];
        en_passant_sq = NO_SQUARE;
    }
}

void Position::unmake_null_move() {
    StateInfo si = history.back();
    history.pop_back();

    castling_rights = (CastlingRights)si.prev_castle;
    en_passant_sq = (Square)si.prev_ep_sq;
    halfmove_clock = si.prev_halfmove;
    hash_key = si.prev_zobrist;

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;
}

void Position::unmake_move(Move move) {
    StateInfo si = history.back();
    history.pop_back();

    castling_rights = (CastlingRights)si.prev_castle;
    en_passant_sq = (Square)si.prev_ep_sq;
    halfmove_clock = si.prev_halfmove;
    hash_key = si.prev_zobrist;

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;

    if (side_to_move == WHITE) {
        fullmove_number--;
    }

    Square from_sq = move.from();
    Square to_sq = move.to();
    PieceType piece_moved = move.moving_piece();
    PieceType captured_piece = move.captured_piece();
    Move::PromotionType promoted_type = move.promotion();
    uint8_t flags = move.flags();

    clear_bit(piece_bitboards[piece_moved], to_sq);
    clear_bit(occupancy_bitboards[side_to_move], to_sq);
    clear_bit(occupancy_bitboards[BOTH], to_sq);

    set_bit(piece_bitboards[piece_moved], from_sq);
    set_bit(occupancy_bitboards[side_to_move], from_sq);
    set_bit(occupancy_bitboards[BOTH], from_sq);

    if (captured_piece != NO_PIECE && !(flags & Move::EN_PASSANT)) {
        set_bit(piece_bitboards[captured_piece], to_sq);
        set_bit(occupancy_bitboards[(side_to_move == WHITE ? BLACK : WHITE)], to_sq);
        set_bit(occupancy_bitboards[BOTH], to_sq);
    }

    if (move.is_promotion()) {
        clear_bit(piece_bitboards[promotion_val_to_piece_type(promoted_type, side_to_move)], to_sq);
        set_bit(piece_bitboards[piece_moved], to_sq);
    }

    if (flags & Move::CASTLING) {
        if (side_to_move == WHITE) {
            if (to_sq == G1) {
                clear_bit(piece_bitboards[WR], F1);
                clear_bit(occupancy_bitboards[WHITE], F1);
                clear_bit(occupancy_bitboards[BOTH], F1);

                set_bit(piece_bitboards[WR], H1);
                set_bit(occupancy_bitboards[WHITE], H1);
                set_bit(occupancy_bitboards[BOTH], H1);
            } else if (to_sq == C1) {
                clear_bit(piece_bitboards[WR], D1);
                clear_bit(occupancy_bitboards[WHITE], D1);
                clear_bit(occupancy_bitboards[BOTH], D1);

                set_bit(piece_bitboards[WR], A1);
                set_bit(occupancy_bitboards[WHITE], A1);
                set_bit(occupancy_bitboards[BOTH], A1);
            }
        } else { // Black
            if (to_sq == G8) {
                clear_bit(piece_bitboards[BR], F8);
                clear_bit(occupancy_bitboards[BLACK], F8);
                clear_bit(occupancy_bitboards[BOTH], F8);

                set_bit(piece_bitboards[BR], H8);
                set_bit(occupancy_bitboards[BLACK], H8);
                set_bit(occupancy_bitboards[BOTH], H8);
            } else if (to_sq == C8) {
                clear_bit(piece_bitboards[BR], D8);
                clear_bit(occupancy_bitboards[BLACK], D8);
                clear_bit(occupancy_bitboards[BOTH], D8);

                set_bit(piece_bitboards[BR], A8);
                set_bit(occupancy_bitboards[BLACK], A8);
                set_bit(occupancy_bitboards[BOTH], A8);
            }
        }
    }

    if (flags & Move::EN_PASSANT) {
        Square captured_pawn_sq = (Square)((side_to_move == WHITE) ? (to_sq - 8) : (to_sq + 8));
        PieceType pawn_type = (side_to_move == WHITE) ? BP : WP;

        set_bit(piece_bitboards[pawn_type], captured_pawn_sq);
        set_bit(occupancy_bitboards[(side_to_move == WHITE ? BLACK : WHITE)], captured_pawn_sq);
        set_bit(occupancy_bitboards[BOTH], captured_pawn_sq);
    }
