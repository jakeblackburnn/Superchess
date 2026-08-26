#ifndef PIECE_H
#define PIECE_H

#include "utils.h"

typedef enum {
    Pawn, 
    Knight, 
    Bishop, 
    Rook,
    Queen, 
    King,
    Empty,
} PieceType;

typedef enum {
    White,
    Black
} Color;

// printable piece chars - indices match color and piece enums
extern const char *GLYPHS[2][6];

#define COLOR(idx) (idx / 8 < 4 ? White : Black)

typedef struct Piece {
    PieceType type;
    Color color;
    int hasmoved;
    int passable;
} Piece;

Piece create_piece(PieceType, Color);

extern const int KNIGHT_DR[8], KNIGHT_DF[8];
extern const int KING_DR[8], KING_DF[8]; 
extern const int BISHOP_DR[4], BISHOP_DF[4];
extern const int ROOK_DR[4], ROOK_DF[4];
extern const int PAWN_CAPTURE_DF[2];

#endif
