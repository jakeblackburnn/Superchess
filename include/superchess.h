#ifndef SUPERCHESS_H

#include <stdio.h>
#include <stdlib.h>

#define BOARD_SIZE 64

// Pieces
typedef enum {
    Pawn, 
    Knight, 
    Bishop, 
    Rook,
    Queen, 
    King,
    Empty,
} PieceType;

#define COLOR(idx) (idx / 8 < 4 ? White : Black)

typedef enum {
    White,
    Black
} Color;

typedef struct Piece {
    PieceType type;
    Color color;
    int hasmoved;
    int justmoved;
} Piece;

Piece create_piece(PieceType, Color);
PieceType get_piece_type(int);

// Shared direction tables: the offset sets a piece type moves along are
// identical wherever that piece type appears (move generation here,
// attacked-square probing in attacks.c), so they're defined once in
// piece.c rather than duplicated per file.
extern const int KNIGHT_DR[8], KNIGHT_DF[8];
extern const int KING_DR[8], KING_DF[8]; // also queen's 8 slide directions
extern const int BISHOP_DR[4], BISHOP_DF[4];
extern const int ROOK_DR[4], ROOK_DF[4];
extern const int PAWN_CAPTURE_DF[2];

void gen_pawn_moves(int, int *, int *);
void gen_knight_moves(int, int *, int *);
void gen_bishop_moves(int, int *, int *);
void gen_rook_moves(int, int *, int *);
void gen_queen_moves(int, int *, int *);
void gen_king_moves(int, int *, int *);


// Board 
typedef struct {
    Piece squares[BOARD_SIZE];
} Board;

extern Board b;
extern Board temp;
extern Color turn;

void create_board(void);
int get_piece_idx(int, int);
void print_board(Board, Color);

// Moves
#define MOVE_BUFSIZE 7
extern char move[MOVE_BUFSIZE];
extern int from;
extern int to;
extern char special;

extern int valid_moves[BOARD_SIZE];
extern int valid_move_count;

int str_to_idx(char *);
int getmove(void);
void apply_move(Board *, int, int, char);
int is_square_attacked(Board *, int, Color);
int king_in_check(Board *, Color);
void gen_moves_for_piece(int, int *, int *);
int is_legal_move(int, int, char);
int has_legal_moves(Color);
int is_checkmate(Color);
int is_stalemate(Color);
unsigned long long perft(int);

// UI utils 
void print_whitespace(int);


// Main 
typedef struct {
    Color winner;
    Board board;
} ChessResult;

extern ChessResult *results;

int main(int, char **);

#endif 
