#include "superchess.h"
// extern Board b;
// extern Board temp;

int pos;
int valid_moves[BOARD_SIZE];
int valid_move_count = 0;

const int KNIGHT_DR[8] = { 1,  1, -1, -1,  2,  2, -2, -2 };
const int KNIGHT_DF[8] = { 2, -2,  2, -2,  1, -1,  1, -1 };
const int KING_DR[8]   = { -1, -1, -1,  0,  0,  1,  1,  1 };
const int KING_DF[8]   = { -1,  0,  1, -1,  1, -1,  0,  1 };
const int BISHOP_DR[4] = { 1,  1, -1, -1 };
const int BISHOP_DF[4] = { 1, -1,  1, -1 };
const int ROOK_DR[4]   = {  1, -1,  0, 0 };
const int ROOK_DF[4]   = {  0,  0,  1, -1 };
const int PAWN_CAPTURE_DF[2] = { -1, 1 };

Piece create_piece(PieceType type, Color color) {
    return (Piece){type, color, 0, 0};
}

PieceType get_piece_type(int pos) {
    return b.squares[pos].type;
}

void gen_pawn_moves(int pos, int *moves, int *num_moves) {
    *num_moves = 0;
    if (get_piece_type(pos) != Pawn)
        return;

    Piece piece = b.squares[pos];
    int rank = pos / 8;
    int file = pos % 8;
    int dir = (piece.color == White) ? 1 : -1;
    int start_rank = (piece.color == White) ? 1 : 6;

    // Single push.
    int r1 = rank + dir;
    if (r1 >= 0 && r1 <= 7) {
        int idx1 = get_piece_idx(r1, file);
        if (b.squares[idx1].type == Empty) {
            moves[(*num_moves)++] = idx1;

            // Double push, only from the starting rank and only if both
            // squares ahead are clear.
            if (rank == start_rank) {
                int r2 = rank + 2 * dir;
                int idx2 = get_piece_idx(r2, file);
                if (b.squares[idx2].type == Empty)
                    moves[(*num_moves)++] = idx2;
            }
        }
    }

    // Diagonal captures (including en passant).
    for (int i = 0; i < 2; i++) {
        int r = rank + dir;
        int f = file + PAWN_CAPTURE_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;

        int idx = get_piece_idx(r, f);
        Piece target = b.squares[idx];
        if (target.type != Empty && target.color != piece.color) {
            moves[(*num_moves)++] = idx;
            continue;
        }

        // En passant: target square is empty, but the square beside us
        // (same rank as `pos`) holds an enemy pawn that just double-pushed.
        int adj_idx = get_piece_idx(rank, f);
        Piece adjacent = b.squares[adj_idx];
        if (target.type == Empty && adjacent.type == Pawn &&
            adjacent.color != piece.color && adjacent.justmoved)
            moves[(*num_moves)++] = idx;
    }

    // Promotion is not a distinct target square: reaching the back rank
    // via a push/capture above already yields the right index. Which
    // piece to promote to comes from `special` in getmove(), not here.
}

void gen_knight_moves(int pos, int *moves, int *num_moves) {
    Piece piece = b.squares[pos];
    int rank = pos / 8;
    int file = pos % 8;

    *num_moves = 0;
    for (int i = 0; i < 8; i++) {
        int r = rank + KNIGHT_DR[i];
        int f = file + KNIGHT_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;

        int idx = get_piece_idx(r, f);
        Piece target = b.squares[idx];
        if (target.type == Empty || target.color != piece.color)
            moves[(*num_moves)++] = idx;
    }
}

static void slide(int pos, const int *dr, const int *df, int num_dirs, int *moves, int *num_moves) {
    Piece piece = b.squares[pos];
    int rank = pos / 8;
    int file = pos % 8;

    *num_moves = 0;
    for (int i = 0; i < num_dirs; i++) {
        int r = rank;
        int f = file;
        for (;;) {
            r += dr[i];
            f += df[i];
            if (r < 0 || r > 7 || f < 0 || f > 7)
                break;

            int idx = get_piece_idx(r, f);
            Piece target = b.squares[idx];
            if (target.type == Empty) {
                moves[(*num_moves)++] = idx;
                continue;
            }
            if (target.color != piece.color)
                moves[(*num_moves)++] = idx;
            break;
        }
    }
}

void gen_bishop_moves(int pos, int *moves, int *num_moves) {
    slide(pos, BISHOP_DR, BISHOP_DF, 4, moves, num_moves);
}

void gen_rook_moves(int pos, int *moves, int *num_moves) {
    slide(pos, ROOK_DR, ROOK_DF, 4, moves, num_moves);
}

void gen_queen_moves(int pos, int *moves, int *num_moves) {
    slide(pos, KING_DR, KING_DF, 8, moves, num_moves); // same 8 directions as the king, but sliding
}

void gen_king_moves(int pos, int *moves, int *num_moves) {
    Piece piece = b.squares[pos];
    int rank = pos / 8;
    int file = pos % 8;

    *num_moves = 0;
    for (int i = 0; i < 8; i++) {
        int r = rank + KING_DR[i];
        int f = file + KING_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;

        int idx = get_piece_idx(r, f);
        Piece target = b.squares[idx];
        if (target.type == Empty || target.color != piece.color)
            moves[(*num_moves)++] = idx;
    }

    // Castling ('c') is intentionally not generated here: it needs
    // hasmoved/rook-position/attacked-square state beyond a single
    // king square, so it's handled as a separate check once
    // attacked-square detection exists.
}
