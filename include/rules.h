#ifndef RULES_H
#define RULES_H

#include "moves.h"

int is_square_attacked(Board *, int, Color);
int is_king_in_check(Board *, Color);
int is_legal_move(Board *, int, int, char);
int has_legal_moves(Board *, Color);

int is_checkmate(Board *, Color);
int is_stalemate(Board *, Color);

#endif
