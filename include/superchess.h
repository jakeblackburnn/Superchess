#ifndef SUPERCHESS_H
#define SUPERCHESS_H

#include <stdio.h>
#include <stdlib.h>

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

typedef struct Piece {
    PieceType type;
    Color color;
} Piece;

// Piece
Piece create_piece(PieceType, Color);

typedef struct {
    Piece squares[64];
} Board;

// Board 
Board create_board(void);
int get_piece_idx(int, int);
void print_board(Board, Color);

// Moves
#define MOVE_BUFSIZE 16

int getmove(char *);

// UI utils 
void print_whitespace(int);

#endif 
