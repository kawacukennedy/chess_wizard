#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <array>
#include <vector>
#include <string>

using Bitboard = uint64_t;
using Square = int;
using Color = int;
using PieceType = int;
using CastlingRights = int;

const Square NO_SQUARE = -1;
const PieceType NO_PIECE = -1;

enum { WHITE, BLACK, BOTH };
enum { WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK };
enum { NO_CASTLING, WHITE_KINGSIDE = 1, WHITE_QUEENSIDE = 2, BLACK_KINGSIDE = 4, BLACK_QUEENSIDE = 8 };

#endif