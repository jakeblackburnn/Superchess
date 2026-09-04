#ifndef BOARD_H
#define BOARD_H

#include "piece.h"

#define BOARD_SIZE 64
#define FILE(idx) (idx % 8)
#define RANK(idx) (idx / 8)
#define FEN_BUFSIZE 73

// rank major flat arr of pieces 
// only the board should handle flat indices
// all other code should use file & rank
typedef struct {
    Piece squares[BOARD_SIZE];
} Board;

void reset_board(Board *);
int get_piece_idx(int, int); // rank, file
int str_to_idx(char *);
PieceType get_piece_type(Board *, int, int);
void print_board(Board, Color);

void board_to_str(Board *, char *);
void str_to_board(char *, Board *);

#endif
