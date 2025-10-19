#include "board.h"
#include "zobrist.h"
#include "attack.h"
#include "bitboard.h"
#include "move.h"
#include "nnue.h"
#include <iostream>
#include <sstream>
#include <cctype>



Position::Position() {
    // Initialize bitboards to zero
    piece_bitboards.fill(Bitboard(0));
    occWhite = 0;
    occBlack = 0;
    occAll = 0;
    side_to_move = WHITE;
    castling_rights = NO_CASTLING;
    en_passant_sq = NO_SQUARE;
    halfmove_clock = 0;
    fullmove_number = 1;
    hash_key = 0;
    history_size = 0;
}

void Position::set_from_fen(const std::string& fen) {
    // Reset
    piece_bitboards.fill(Bitboard(0));
    occWhite = 0;
    occBlack = 0;
    occAll = 0;
    history_size = 0;

    std::string actual_fen = (fen == "startpos") ? START_FEN : fen;
    std::istringstream iss(actual_fen);
    std::string board_str, side_str, castle_str, ep_str, hm_str, fm_str;
    iss >> board_str >> side_str >> castle_str >> ep_str >> hm_str >> fm_str;

    // Parse board
    int sq = 56; // Start from a8
    for (char c : board_str) {
        if (c == '/') {
            sq -= 16; // Next rank
        } else if (c >= '1' && c <= '8') {
            sq += c - '0';
        } else {
            PieceType pt;
            Color color = (c >= 'a') ? BLACK : WHITE;
            switch (tolower(c)) {
                case 'p': pt = WP; break;
                case 'n': pt = WN; break;
                case 'b': pt = WB; break;
                case 'r': pt = WR; break;
                case 'q': pt = WQ; break;
                case 'k': pt = WK; break;
                default: continue;
            }
            if (color == BLACK) pt = (PieceType)(pt + 6);
            set_bit(piece_bitboards[pt], (Square)sq);
            if (color == WHITE) set_bit(occWhite, (Square)sq);
            else set_bit(occBlack, (Square)sq);
            set_bit(occAll, (Square)sq);
            sq++;
        }
    }

    // Side to move
    side_to_move = (side_str == "w") ? WHITE : BLACK;

    // Castling
    castling_rights = NO_CASTLING;
    if (castle_str.find('K') != std::string::npos) castling_rights = (CastlingRights)(castling_rights | WHITE_KINGSIDE);
    if (castle_str.find('Q') != std::string::npos) castling_rights = (CastlingRights)(castling_rights | WHITE_QUEENSIDE);
    if (castle_str.find('k') != std::string::npos) castling_rights = (CastlingRights)(castling_rights | BLACK_KINGSIDE);
    if (castle_str.find('q') != std::string::npos) castling_rights = (CastlingRights)(castling_rights | BLACK_QUEENSIDE);

    // En passant
    en_passant_sq = (ep_str == "-") ? NO_SQUARE : (Square)((ep_str[1] - '1') * 8 + (ep_str[0] - 'a'));

    // Halfmove and fullmove
    halfmove_clock = std::stoi(hm_str);
    fullmove_number = std::stoi(fm_str);

    // Initialize piece_of_square
    for (int sq = 0; sq < 64; ++sq) {
        piece_of_square[sq] = piece_on_square((Square)sq);
    }

    // Compute hash
    hash_key = 0;
    for (int sq = 0; sq < 64; ++sq) {
        PieceType pt = piece_on_square((Square)sq);
        if (pt != NO_PIECE) {
            hash_key ^= Zobrist.piece_keys[pt][sq];
        }
    }
    if (side_to_move == BLACK) hash_key ^= Zobrist.side_to_move_key;
    hash_key ^= Zobrist.castling_keys[castling_rights];
    if (en_passant_sq != NO_SQUARE) hash_key ^= Zobrist.en_passant_keys[file_of(en_passant_sq)];
    if (hash_key == 0) hash_key = 1;
}

PieceType Position::piece_on_square(Square sq) const {
    for (int i = 0; i < 12; ++i) {
        if (piece_bitboards[i] & (1ULL << sq)) {
            return (PieceType)i;
        }
    }
    return NO_PIECE;
}

bool Position::is_square_attacked(Square sq, Color by_color) const {
    // Check pawns
    Bitboard pawns = piece_bitboards[by_color == WHITE ? WP : BP];
    Bitboard pawn_attacks = by_color == WHITE ? ((pawns & ~FILE_H) << 7) | ((pawns & ~FILE_A) << 9) : ((pawns & ~FILE_H) >> 9) | ((pawns & ~FILE_A) >> 7);
    if (pawn_attacks & (1ULL << sq)) return true;

    // Check knights
    Bitboard knights = piece_bitboards[by_color == WHITE ? WN : BN];
    while (knights) {
        Square from = pop_bit(knights);
        if (KNIGHT_ATTACKS[from] & (1ULL << sq)) return true;
    }

    // Check bishops/queens
    Bitboard bishops = piece_bitboards[by_color == WHITE ? WB : BB] | piece_bitboards[by_color == WHITE ? WQ : BQ];
    while (bishops) {
        Square from = pop_bit(bishops);
        Bitboard attacks = get_bishop_attacks(from, occAll);
        attacks &= ~(by_color == WHITE ? occWhite : occBlack);
        if (attacks & (1ULL << sq)) return true;
    }

    // Check rooks/queens
    Bitboard rooks = piece_bitboards[by_color == WHITE ? WR : BR] | piece_bitboards[by_color == WHITE ? WQ : BQ];
    while (rooks) {
        Square from = pop_bit(rooks);
        Bitboard attacks = get_rook_attacks(from, occAll);
        attacks &= ~(by_color == WHITE ? occWhite : occBlack);
        if (attacks & (1ULL << sq)) return true;
    }

    // Check king
    Bitboard king = piece_bitboards[by_color == WHITE ? WK : BK];
    if (king) {
        Square from = (Square)lsb_index(king);
        Bitboard attacks = KING_ATTACKS[from];
        if (attacks & (1ULL << sq)) return true;
    }

    return false;
}

bool Position::is_check() const {
    Square king_sq = get_king_square(*this, side_to_move);
    return is_square_attacked(king_sq, (Color)(1 - side_to_move));
}

Square get_king_square(const Position& pos, Color color) {
    Bitboard king_bb = pos.piece_bitboards[color == WHITE ? WK : BK];
    return (Square)lsb_index(king_bb);
}

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

bool Position::make_move(Move move) {
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
    si.captured_piece = captured_piece;
    si.promoted_piece = promoted_type == Move::NO_PROMOTION ? NO_PIECE : promotion_val_to_piece_type(promoted_type, side_to_move);
    si.prev_castle = castling_rights;
    si.prev_ep_file = en_passant_sq == NO_SQUARE ? -1 : file_of(en_passant_sq);
    si.prev_halfmove = halfmove_clock;
    si.prev_zobrist = hash_key;
    si.eval_delta = 0; // TODO: compute incremental eval delta
    si.nnue_delta_count = 0; // TODO: nnue deltas
    history[history_size++] = si;

    // Update halfmove clock (reset on pawn move or capture)
    if (piece_moved == WP || piece_moved == BP || captured_piece != NO_PIECE || (flags & Move::EN_PASSANT)) {
        halfmove_clock = 0;
    } else {
        halfmove_clock++;
    }

    // Remove piece from 'from' square
    clear_bit(piece_bitboards[piece_moved], from_sq);
    clear_bit((side_to_move == WHITE ? occWhite : occBlack), from_sq);
    clear_bit(occAll, from_sq);
    hash_key ^= Zobrist.piece_keys[piece_moved][from_sq];

    if (captured_piece != NO_PIECE && !(flags & Move::EN_PASSANT)) {
        clear_bit(piece_bitboards[captured_piece], to_sq);
        clear_bit((side_to_move == WHITE ? occBlack : occWhite), to_sq);
        clear_bit(occAll, to_sq);
        hash_key ^= Zobrist.piece_keys[captured_piece][to_sq];
    }

    // Place piece on 'to' square
    set_bit(piece_bitboards[piece_moved], to_sq);
    set_bit((side_to_move == WHITE ? occWhite : occBlack), to_sq);
    set_bit(occAll, to_sq);
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
                clear_bit(occWhite, H1);
                clear_bit(occAll, H1);
                hash_key ^= Zobrist.piece_keys[WR][H1];

                set_bit(piece_bitboards[WR], F1);
                set_bit(occWhite, F1);
                set_bit(occAll, F1);
                hash_key ^= Zobrist.piece_keys[WR][F1];
            } else if (to_sq == C1) { // White queenside castling
                clear_bit(piece_bitboards[WR], A1);
                clear_bit(occWhite, A1);
                clear_bit(occAll, A1);
                hash_key ^= Zobrist.piece_keys[WR][A1];

                set_bit(piece_bitboards[WR], D1);
                set_bit(occWhite, D1);
                set_bit(occAll, D1);
                hash_key ^= Zobrist.piece_keys[WR][D1];
            }
        } else { // Black
            if (to_sq == G8) { // Black kingside castling
                clear_bit(piece_bitboards[BR], H8);
                clear_bit(occBlack, H8);
                clear_bit(occAll, H8);
                hash_key ^= Zobrist.piece_keys[BR][H8];

                set_bit(piece_bitboards[BR], F8);
                set_bit(occBlack, F8);
                set_bit(occAll, F8);
                hash_key ^= Zobrist.piece_keys[BR][F8];
            } else if (to_sq == C8) { // Black queenside castling
                clear_bit(piece_bitboards[BR], A8);
                clear_bit(occBlack, A8);
                clear_bit(occAll, A8);
                hash_key ^= Zobrist.piece_keys[BR][A8];

                set_bit(piece_bitboards[BR], D8);
                set_bit(occBlack, D8);
                set_bit(occAll, D8);
                hash_key ^= Zobrist.piece_keys[BR][D8];
            }
        }
    }

    // Handle en passant capture
    if (flags & Move::EN_PASSANT) {
        Square captured_pawn_sq = (Square)((side_to_move == WHITE) ? (to_sq - 8) : (to_sq + 8));
        PieceType pawn_type = (side_to_move == WHITE) ? BP : WP;

        clear_bit(piece_bitboards[pawn_type], captured_pawn_sq);
        clear_bit((side_to_move == WHITE ? occBlack : occWhite), captured_pawn_sq);
        clear_bit(occAll, captured_pawn_sq);
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

    // Check if move is legal
    Square king_sq = get_king_square(*this, side_to_move == WHITE ? BLACK : WHITE);
    if (is_square_attacked(king_sq, side_to_move)) {
        unmake_move(move);
        return false;
    }

    return true;
}

void Position::make_null_move() {
    StateInfo si = {};
    si.from = NO_SQUARE;
    si.to = NO_SQUARE;
    si.captured_piece = NO_PIECE;
    si.promoted_piece = NO_PIECE;
    si.prev_castle = castling_rights;
    si.prev_ep_file = en_passant_sq == NO_SQUARE ? -1 : file_of(en_passant_sq);
    si.prev_halfmove = halfmove_clock;
    si.prev_zobrist = hash_key;
    si.eval_delta = 0;
    si.nnue_delta_count = 0;
    history[history_size++] = si;

    hash_key ^= Zobrist.side_to_move_key;

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;

    halfmove_clock++;
}

void Position::unmake_null_move() {
    StateInfo si = history[--history_size];

    castling_rights = (CastlingRights)si.prev_castle;
    en_passant_sq = si.prev_ep_file == -1 ? NO_SQUARE : (Square)(si.prev_ep_file + (side_to_move == WHITE ? 40 : 16));
    halfmove_clock = si.prev_halfmove;
    hash_key = si.prev_zobrist;

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;
}

void Position::unmake_move(Move move) {
    StateInfo si = history[--history_size];

    // Restore state from StateInfo
    castling_rights = (CastlingRights)si.prev_castle;
    en_passant_sq = si.prev_ep_file == -1 ? NO_SQUARE : (Square)(si.prev_ep_file + (side_to_move == WHITE ? 40 : 16));
    halfmove_clock = si.prev_halfmove;
    hash_key = si.prev_zobrist;
    // eval_delta is handled in search

    side_to_move = (side_to_move == WHITE) ? BLACK : WHITE;

    if (side_to_move == WHITE) {
        fullmove_number--;
    }

    // Reverse the make_move logic using the move info
    Square from_sq = move.from();
    Square to_sq = move.to();
    PieceType piece_moved = move.moving_piece();
    PieceType captured_piece = (PieceType)si.captured_piece;
    PieceType promoted_piece = (PieceType)si.promoted_piece;
    uint8_t flags = move.flags();

    // Undo the move on bitboards
    clear_bit(piece_bitboards[piece_moved], to_sq);
    clear_bit((side_to_move == WHITE ? occWhite : occBlack), to_sq);
    clear_bit(occAll, to_sq);

    set_bit(piece_bitboards[piece_moved], from_sq);
    set_bit((side_to_move == WHITE ? occWhite : occBlack), from_sq);
    set_bit(occAll, from_sq);

    if (captured_piece != NO_PIECE) {
        if (flags & Move::EN_PASSANT) {
            Square ep_sq = (Square)((side_to_move == WHITE) ? (to_sq - 8) : (to_sq + 8));
            set_bit(piece_bitboards[captured_piece], ep_sq);
            set_bit((side_to_move == WHITE ? occBlack : occWhite), ep_sq);
            set_bit(occAll, ep_sq);
        } else {
            set_bit(piece_bitboards[captured_piece], to_sq);
            set_bit((side_to_move == WHITE ? occBlack : occWhite), to_sq);
            set_bit(occAll, to_sq);
        }
    }

    if (promoted_piece != NO_PIECE) {
        clear_bit(piece_bitboards[piece_moved], to_sq);
        set_bit(piece_bitboards[promoted_piece], to_sq);
    }

    if (flags & Move::CASTLING) {
        if (side_to_move == WHITE) {
            if (to_sq == G1) {
                clear_bit(piece_bitboards[WR], F1);
                clear_bit(occWhite, F1);
                clear_bit(occAll, F1);
                set_bit(piece_bitboards[WR], H1);
                set_bit(occWhite, H1);
                set_bit(occAll, H1);
            } else if (to_sq == C1) {
                clear_bit(piece_bitboards[WR], D1);
                clear_bit(occWhite, D1);
                clear_bit(occAll, D1);
                set_bit(piece_bitboards[WR], A1);
                set_bit(occWhite, A1);
                set_bit(occAll, A1);
            }
        } else {
            if (to_sq == G8) {
                clear_bit(piece_bitboards[BR], F8);
                clear_bit(occBlack, F8);
                clear_bit(occAll, F8);
                set_bit(piece_bitboards[BR], H8);
                set_bit(occBlack, H8);
                set_bit(occAll, H8);
            } else if (to_sq == C8) {
                clear_bit(piece_bitboards[BR], D8);
                clear_bit(occBlack, D8);
                clear_bit(occAll, D8);
                set_bit(piece_bitboards[BR], A8);
                set_bit(occBlack, A8);
                set_bit(occAll, A8);
            }
        }
    }
}

