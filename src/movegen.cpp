#include "movegen.h"
#include "board.h"
#include "attack.h"
#include "move.h"
#include <iostream>

void generate_pawn_moves(const Position& pos, Move* captures, int& num_captures, Move* quiets, int& num_quiets, bool captures_only) {
    Color us = pos.side_to_move;
    Color them = (us == WHITE) ? BLACK : WHITE;
    Bitboard our_pawns = pos.piece_bitboards[us == WHITE ? WP : BP];
    Bitboard their_pieces = (them == WHITE ? pos.occWhite : pos.occBlack);
    Bitboard all_pieces = pos.occAll;

    int push_offset = (us == WHITE) ? 8 : -8;
    int dbl_push_offset = (us == WHITE) ? 16 : -16;
    Rank dbl_push_rank = (us == WHITE) ? RANK_2 : RANK_7;
    Rank promotion_rank = (us == WHITE) ? RANK_8 : RANK_1;

    Bitboard pawns = our_pawns;
    while (pawns) {
        Square from = pop_bit(pawns);
        Rank r = get_rank(from);

        // --- Pushes ---
        if (!captures_only) {
            Square to = static_cast<Square>(from + push_offset);
            if (!get_bit(all_pieces, to)) {
                if (get_rank(to) == promotion_rank) {
                    quiets[num_quiets++] = create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_Q);
                    quiets[num_quiets++] = create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_R);
                    quiets[num_quiets++] = create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_B);
                    quiets[num_quiets++] = create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_N);
                } else {
                    quiets[num_quiets++] = create_move(from, to, pos.piece_on_square(from));
                }

                // Double push
                if (r == dbl_push_rank) {
                    to = static_cast<Square>(from + dbl_push_offset);
                    if (!get_bit(all_pieces, to)) {
                        quiets[num_quiets++] = create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::NO_PROMOTION, Move::DOUBLE_PAWN_PUSH);
                    }
                }
            }
        }

        // --- Captures ---
        Bitboard attacks = PAWN_ATTACKS[us][from];
        Bitboard attackable = their_pieces;
        if (pos.en_passant_sq != NO_SQUARE) {
            attackable |= (1ULL << pos.en_passant_sq);
        }

        attacks &= attackable;
        while (attacks) {
            Square to = pop_bit(attacks);
            if (to == pos.en_passant_sq) {
                captures[num_captures++] = create_move(from, to, pos.piece_on_square(from), (us == WHITE ? BP : WP), Move::NO_PROMOTION, Move::EN_PASSANT);
            } else {
                PieceType captured = pos.piece_on_square(to);
                if (get_rank(to) == promotion_rank) {
                     captures[num_captures++] = create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_Q);
                     captures[num_captures++] = create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_R);
                     captures[num_captures++] = create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_B);
                     captures[num_captures++] = create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_N);
                } else {
                captures[num_captures++] = create_move(from, to, pos.piece_on_square(from), captured);
                }
            }
        }
    }
}

template<PieceType Pt>
void generate_piece_moves(const Position& pos, Move* captures, int& num_captures, Move* quiets, int& num_quiets, bool captures_only) {
    Color us = pos.side_to_move;
    Bitboard our_pieces = pos.piece_bitboards[Pt];
    Bitboard their_pieces = (us == WHITE ? pos.occBlack : pos.occWhite);
    Bitboard all_pieces = pos.occAll;

    while (our_pieces) {
        Square from = pop_bit(our_pieces);
        Bitboard attacks = get_piece_attacks(Pt, from, all_pieces);

        Bitboard targets = captures_only ? (attacks & their_pieces) : ((attacks & ~all_pieces) | (attacks & their_pieces));

        while (targets) {
            Square to = pop_bit(targets);
            PieceType captured = get_bit(their_pieces, to) ? pos.piece_on_square(to) : NO_PIECE;
            if (captured != NO_PIECE) {
                captures[num_captures++] = create_move(from, to, Pt, captured);
            } else {
                quiets[num_quiets++] = create_move(from, to, Pt, captured);
            }
        }
    }
}

void generate_castling_moves(const Position& pos, Move* quiets, int& num_quiets) {
    Color us = pos.side_to_move;
    if (pos.is_check()) return;

    if (us == WHITE) {
        if ((pos.castling_rights & WHITE_KINGSIDE) &&
            !get_bit(pos.occAll, F1) &&
            !get_bit(pos.occAll, G1) &&
            !pos.is_square_attacked(E1, BLACK) &&
            !pos.is_square_attacked(F1, BLACK) &&
            !pos.is_square_attacked(G1, BLACK)) {
            quiets[num_quiets++] = create_move(E1, G1, WK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING);
        }
        if ((pos.castling_rights & WHITE_QUEENSIDE) &&
            !get_bit(pos.occAll, D1) &&
            !get_bit(pos.occAll, C1) &&
            !get_bit(pos.occAll, B1) &&
            !pos.is_square_attacked(E1, BLACK) &&
            !pos.is_square_attacked(D1, BLACK) &&
            !pos.is_square_attacked(C1, BLACK)) {
            quiets[num_quiets++] = create_move(E1, C1, WK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING);
        }
    } else { // BLACK
        if ((pos.castling_rights & BLACK_KINGSIDE) &&
            !get_bit(pos.occAll, F8) &&
            !get_bit(pos.occAll, G8) &&
            !pos.is_square_attacked(E8, WHITE) &&
            !pos.is_square_attacked(F8, WHITE) &&
            !pos.is_square_attacked(G8, WHITE)) {
            quiets[num_quiets++] = create_move(E8, G8, BK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING);
        }
        if ((pos.castling_rights & BLACK_QUEENSIDE) &&
            !get_bit(pos.occAll, D8) &&
            !get_bit(pos.occAll, C8) &&
            !get_bit(pos.occAll, B8) &&
            !pos.is_square_attacked(E8, WHITE) &&
            !pos.is_square_attacked(D8, WHITE) &&
            !pos.is_square_attacked(C8, WHITE)) {
            quiets[num_quiets++] = create_move(E8, C8, BK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING);
        }
    }
}


void generate_moves(const Position& pos, MoveList& moves, bool captures_only) {
    Move captures[256];
    Move quiets[256];
    int num_captures = 0, num_quiets = 0;
    generate_moves(pos, captures, num_captures, quiets, num_quiets, captures_only);
    moves.clear();
    for (int i = 0; i < num_captures; ++i) moves.push_back(captures[i]);
    for (int i = 0; i < num_quiets; ++i) moves.push_back(quiets[i]);
}

void generate_moves(const Position& pos, Move* captures, int& num_captures, Move* quiets, int& num_quiets, bool captures_only) {
    num_captures = 0;
    num_quiets = 0;

    generate_pawn_moves(pos, captures, num_captures, quiets, num_quiets, captures_only);

    if (pos.side_to_move == WHITE) {
        generate_piece_moves<WN>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<WB>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<WR>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<WQ>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<WK>(pos, captures, num_captures, quiets, num_quiets, captures_only);
    } else {
        generate_piece_moves<BN>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<BB>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<BR>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<BQ>(pos, captures, num_captures, quiets, num_quiets, captures_only);
        generate_piece_moves<BK>(pos, captures, num_captures, quiets, num_quiets, captures_only);
    }

    if (!captures_only) {
        generate_castling_moves(pos, quiets, num_quiets);
    }
}

void generate_moves(const Position& pos, Move* moves, int& num_moves, bool captures_only) {
    Move captures[256];
    Move quiets[256];
    int num_captures = 0, num_quiets = 0;
    generate_moves(pos, captures, num_captures, quiets, num_quiets, captures_only);
    num_moves = 0;
    for (int i = 0; i < num_captures; ++i) moves[num_moves++] = captures[i];
    for (int i = 0; i < num_quiets; ++i) moves[num_moves++] = quiets[i];
}

void generate_legal_moves(const Position& pos, MoveList& legal_moves) {
    legal_moves.clear();
    MoveList pseudo_legal_moves;
    generate_moves(pos, pseudo_legal_moves, false);

    for (const auto& move : pseudo_legal_moves) {
        Position temp_pos = pos;
        if (temp_pos.make_move(move)) {
            legal_moves.push_back(move);
        }
    }
}
