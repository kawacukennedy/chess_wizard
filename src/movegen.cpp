#include "movegen.h"
#include "position.h"
#include "attack.h"
#include "move.h"
#include <iostream>

void generate_pawn_moves(const Position& pos, MoveList& moves, bool captures_only) {
    Color us = pos.side_to_move;
    Color them = (us == WHITE) ? BLACK : WHITE;
    Bitboard our_pawns = pos.piece_bitboards[us == WHITE ? WP : BP];
    Bitboard their_pieces = pos.occupancy_bitboards[them];
    Bitboard all_pieces = pos.occupancy_bitboards[BOTH];

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
                    moves.push_back(create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_Q));
                    moves.push_back(create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_R));
                    moves.push_back(create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_B));
                    moves.push_back(create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::PROMOTION_N));
                } else {
                    moves.push_back(create_move(from, to, pos.piece_on_square(from)));
                }

                // Double push
                if (r == dbl_push_rank) {
                    to = static_cast<Square>(from + dbl_push_offset);
                    if (!get_bit(all_pieces, to)) {
                        moves.push_back(create_move(from, to, pos.piece_on_square(from), NO_PIECE, Move::NO_PROMOTION, Move::DOUBLE_PAWN_PUSH));
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
                moves.push_back(create_move(from, to, pos.piece_on_square(from), (us == WHITE ? BP : WP), Move::NO_PROMOTION, Move::EN_PASSANT));
            } else {
                PieceType captured = pos.piece_on_square(to);
                if (get_rank(to) == promotion_rank) {
                     moves.push_back(create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_Q));
                     moves.push_back(create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_R));
                     moves.push_back(create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_B));
                     moves.push_back(create_move(from, to, pos.piece_on_square(from), captured, Move::PROMOTION_N));
                } else {
                moves.push_back(create_move(from, to, pos.piece_on_square(from), captured));
                }
            }
        }
    }
}

template<PieceType Pt>
void generate_piece_moves(const Position& pos, MoveList& moves, bool captures_only) {
    Color us = pos.side_to_move;
    Bitboard our_pieces = pos.piece_bitboards[Pt];
    Bitboard their_pieces = pos.occupancy_bitboards[us == WHITE ? BLACK : WHITE];
    Bitboard all_pieces = pos.occupancy_bitboards[BOTH];

    while (our_pieces) {
        Square from = pop_bit(our_pieces);
        Bitboard attacks = get_piece_attacks(Pt, from, all_pieces);

        Bitboard targets = captures_only ? (attacks & their_pieces) : ((attacks & ~all_pieces) | (attacks & their_pieces));

        while (targets) {
            Square to = pop_bit(targets);
            PieceType captured = get_bit(their_pieces, to) ? pos.piece_on_square(to) : NO_PIECE;
            moves.push_back(create_move(from, to, Pt, captured));
        }
    }
}

void generate_castling_moves(const Position& pos, MoveList& moves) {
    Color us = pos.side_to_move;
    if (pos.is_check()) return;

    if (us == WHITE) {
        if ((pos.castling_rights & WHITE_KINGSIDE) &&
            !get_bit(pos.occupancy_bitboards[BOTH], F1) &&
            !get_bit(pos.occupancy_bitboards[BOTH], G1) &&
            !pos.is_square_attacked(E1, BLACK) &&
            !pos.is_square_attacked(F1, BLACK) &&
            !pos.is_square_attacked(G1, BLACK)) {
            moves.push_back(create_move(E1, G1, WK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING));
        }
        if ((pos.castling_rights & WHITE_QUEENSIDE) &&
            !get_bit(pos.occupancy_bitboards[BOTH], D1) &&
            !get_bit(pos.occupancy_bitboards[BOTH], C1) &&
            !get_bit(pos.occupancy_bitboards[BOTH], B1) &&
            !pos.is_square_attacked(E1, BLACK) &&
            !pos.is_square_attacked(D1, BLACK) &&
            !pos.is_square_attacked(C1, BLACK)) {
            moves.push_back(create_move(E1, C1, WK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING));
        }
    } else { // BLACK
        if ((pos.castling_rights & BLACK_KINGSIDE) &&
            !get_bit(pos.occupancy_bitboards[BOTH], F8) &&
            !get_bit(pos.occupancy_bitboards[BOTH], G8) &&
            !pos.is_square_attacked(E8, WHITE) &&
            !pos.is_square_attacked(F8, WHITE) &&
            !pos.is_square_attacked(G8, WHITE)) {
            moves.push_back(create_move(E8, G8, BK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING));
        }
        if ((pos.castling_rights & BLACK_QUEENSIDE) &&
            !get_bit(pos.occupancy_bitboards[BOTH], D8) &&
            !get_bit(pos.occupancy_bitboards[BOTH], C8) &&
            !get_bit(pos.occupancy_bitboards[BOTH], B8) &&
            !pos.is_square_attacked(E8, WHITE) &&
            !pos.is_square_attacked(D8, WHITE) &&
            !pos.is_square_attacked(C8, WHITE)) {
            moves.push_back(create_move(E8, C8, BK, NO_PIECE, Move::NO_PROMOTION, Move::CASTLING));
        }
    }
}


void generate_moves(const Position& pos, MoveList& moves, bool captures_only) {
    moves.clear();
    
    generate_pawn_moves(pos, moves, captures_only);

    if (pos.side_to_move == WHITE) {
        generate_piece_moves<WN>(pos, moves, captures_only);
        generate_piece_moves<WB>(pos, moves, captures_only);
        generate_piece_moves<WR>(pos, moves, captures_only);
        generate_piece_moves<WQ>(pos, moves, captures_only);
        generate_piece_moves<WK>(pos, moves, captures_only);
    } else {
        generate_piece_moves<BN>(pos, moves, captures_only);
        generate_piece_moves<BB>(pos, moves, captures_only);
        generate_piece_moves<BR>(pos, moves, captures_only);
        generate_piece_moves<BQ>(pos, moves, captures_only);
        generate_piece_moves<BK>(pos, moves, captures_only);
    }

    if (!captures_only) {
        generate_castling_moves(pos, moves);
    }
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
