#ifndef GAME_H
#define GAME_H

#include "piece.h"
#include "board.h" 
#include "moves.h"
#include "utils.h"

#define MOVE_BUFSIZE 7

typedef enum {
    Checkmate,
    Stalemate,
    Draw, // insufficient mating material
} Outcome;

typedef struct {
    Board board;
    Color winner;
    Outcome outcome;
    int turn_count;
    char *playfile;
    char *metafile;
    char *move_log;
} GameResult;

int getmove(char *);
void write_metafile(GameResult *);
void write_playfile(GameResult *);
void start_game(Board *, GameResult *, char *, char *);

#endif
