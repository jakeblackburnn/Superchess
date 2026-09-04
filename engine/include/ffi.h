#ifndef FFI_H
#define FFI_H

#include "board.h"
#include "piece.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle. Callers only ever hold a pointer; internals live in ffi.c.
typedef struct Game Game;

typedef enum {
    SC_ONGOING   = 0,
    SC_CHECKMATE = 1,
    SC_STALEMATE = 2,
    SC_DRAW      = 3, // insufficient mating material
} FfiOutcome;

typedef enum {
    MOVE_OK                = 0,
    MOVE_ILLEGAL            = 1,
    MOVE_PROMOTION_REQUIRED = 2,
    MOVE_GAME_OVER          = 3,
} FfiMoveResult;

// from/to are flat square indices (rank*8+file); special is '\0', one of
// "qbrnk" for promotion, or 'c' for castling.
typedef struct {
    int  from;
    int  to;
    char special;
} FfiMove;

// Mirrors Piece, laid out for ctypes rather than passing Piece by value.
typedef struct {
    int type;      // PieceType; Empty (6) means the square is empty
    int color;      // Color; meaningless when type == Empty
    int hasmoved;
    int passable;
} FfiSquare;

// Large enough for any legal-move count reachable on a 64-square board,
// standard chess or SuperChess.
#define SC_MAX_LEGAL_MOVES 256

Game *sc_new_game(void);
void  sc_free_game(Game *game);
void  sc_reset_game(Game *game);

int sc_turn(Game *game);      // Color
int sc_outcome(Game *game);   // FfiOutcome
int sc_winner(Game *game);    // Color; valid only when outcome == SC_CHECKMATE

// Whether the side to move's king is currently in check (independent of
// whether they still have legal moves — checkmate is check + no moves).
int sc_in_check(Game *game);  // 1/0

// Fills `out` with up to max_moves legal moves for the side to move; returns
// the true legal-move count regardless of how many fit (snprintf-style), so
// a caller passing a too-small buffer can detect truncation.
int sc_legal_moves(Game *game, FfiMove *out, int max_moves);

// Applies a move if legal. `promo` is '\0' unless promoting ('q'/'b'/'r'/
//'n'/'k'); castling is detected automatically from a two-file king move —
// callers never need to pass special='c' themselves.
FfiMoveResult sc_make_move(Game *game, int from, int to, char promo);

// Fills exactly 64 entries, index = rank*8+file, matching Board.squares.
void sc_board_array(Game *game, FfiSquare *out64);

#ifdef __cplusplus
}
#endif

#endif
