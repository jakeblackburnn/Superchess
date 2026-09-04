#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"
#include "piece.h"

// Position hash for repetition detection. Deterministic across runs/builds
// (fixed-seed table, not system-entropy rand()) so games are reproducible.
// Captures piece placement, per-square hasmoved/passable flags (which is
// how castling rights and en-passant eligibility are represented here), and
// side to move — everything that distinguishes one position from another
// for repetition purposes.
unsigned long long zobrist_hash(const Board *board, Color turn);

#endif
