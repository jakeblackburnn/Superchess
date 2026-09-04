#ifndef SUPERCHESS_H
#define SUPERCHESS_H

#include <stdio.h>
#include <stdlib.h>

#include "piece.h"
#include "board.h"
#include "moves.h"
#include "rules.h"
#include "utils.h"
#include "game.h"
#include "ffi.h"

int main(int, char **);

unsigned long long perft(Board *, Color, int);

#endif
