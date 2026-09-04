#include "superchess.h"

const char *GLYPHS[2][6] = {
    {"♙", "♘", "♗", "♖", "♕", "♔"},
    {"♟", "♞", "♝", "♜", "♛", "♚"},
};

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
    return (Piece){type, color, False, False};
}

void gen_pawn_moves(Board *board, int pos, int *moves, int *num_moves) {
    *num_moves = 0;
    Piece piece = board->squares[pos];
    int rank = RANK(pos);
    int file = FILE(pos);

    int dir = (piece.color == White) ? 1 : -1;
    int start_rank = (piece.color == White) ? 1 : 6;

    // Single push.
    int r1 = rank + dir;
    if (r1 >= 0 && r1 <= 7) {
        int idx1 = get_piece_idx(r1, file);
        if (board->squares[idx1].type == Empty) {
            moves[(*num_moves)++] = idx1;

            // double push
            if (rank == start_rank) {
                int r2 = rank + 2 * dir;
                int idx2 = get_piece_idx(r2, file);
                if (board->squares[idx2].type == Empty)
                    moves[(*num_moves)++] = idx2;
            }
        }
    }

    // Diagonal captures (including en passant).
    for (int i = 0; i <= 1; i++) {
        int r = rank + dir;
        int f = file + PAWN_CAPTURE_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;

        int idx = get_piece_idx(r, f);
        Piece target = board->squares[idx];
        if (target.type != Empty && target.color != piece.color) {
            moves[(*num_moves)++] = idx;
            continue;
        }

        // en passant
        int adj_idx = get_piece_idx(rank, f);
        Piece adjacent = board->squares[adj_idx];
        if (target.type == Empty && adjacent.type == Pawn &&
            adjacent.color != piece.color && adjacent.passable)
            moves[(*num_moves)++] = idx;
    }
    // promotion handled in is_legal_move()
}

void gen_knight_moves(Board *board, int pos, int *moves, int *num_moves) {
    *num_moves = 0;
    Piece piece = board->squares[pos];
    int rank = RANK(pos);
    int file = FILE(pos);

    for (int i = 0; i < 8; i++) {
        int r = rank + KNIGHT_DR[i];
        int f = file + KNIGHT_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;

        int idx = get_piece_idx(r, f);
        Piece target = board->squares[idx];
        if (target.type == Empty || target.color != piece.color)
            moves[(*num_moves)++] = idx;
    }
}

static void slide(Board *board, int pos, const int *dr, const int *df, int num_dirs, int *moves, int *num_moves) {
    *num_moves = 0;
    Piece piece = board->squares[pos];
    int rank = RANK(pos);
    int file = FILE(pos);

    for (int i = 0; i < num_dirs; i++) {
        int r = rank;
        int f = file;
        for (;;) {
            r += dr[i];
            f += df[i];
            if (r < 0 || r > 7 || f < 0 || f > 7)
                break;

            int idx = get_piece_idx(r, f);
            Piece target = board->squares[idx];
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

void gen_bishop_moves(Board *board, int pos, int *moves, int *num_moves) {
    slide(board, pos, BISHOP_DR, BISHOP_DF, 4, moves, num_moves);
}

void gen_rook_moves(Board *board, int pos, int *moves, int *num_moves) {
    slide(board, pos, ROOK_DR, ROOK_DF, 4, moves, num_moves);
}

void gen_queen_moves(Board *board, int pos, int *moves, int *num_moves) {
    slide(board, pos, KING_DR, KING_DF, 8, moves, num_moves); // same 8 directions as the king, but sliding
}

void gen_king_moves(Board *board, int pos, int *moves, int *num_moves) {
    *num_moves = 0;
    Piece piece = board->squares[pos];
    int rank = RANK(pos);
    int file = FILE(pos);

    for (int i = 0; i < 8; i++) {
        int r = rank + KING_DR[i];
        int f = file + KING_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;

        int idx = get_piece_idx(r, f);
        Piece target = board->squares[idx];
        if (target.type == Empty || target.color != piece.color)
            moves[(*num_moves)++] = idx;
    }

    // castling and check detection handled in is_legal_move()
}
