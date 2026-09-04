#ifndef MOVES_H
#define MOVES_H

#include "piece.h"
#include "board.h"


// pseudo-legal moves
void gen_pawn_moves(Board *, int, int *, int *);
void gen_knight_moves(Board *, int, int *, int *);
void gen_bishop_moves(Board *, int, int *, int *);
void gen_rook_moves(Board *, int, int *, int *);
void gen_queen_moves(Board *, int, int *, int *);
void gen_king_moves(Board *, int, int *, int *);

void gen_moves_for_piece(Board *, int, int *, int *);
void apply_move(Board *, int, int, char);

#endif
