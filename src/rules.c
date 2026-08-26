#include "superchess.h"
#include <string.h>

// Valid promotion chars; castling uses 'c' directly, checked inline below.
static const char *PROMO_CHARS = "qbrnk";

int is_square_attacked(Board *board, int idx, Color by) {
    int rank = idx / 8;
    int file = idx % 8;

    int pawn_dr = (by == White) ? -1 : 1;
    for (int i = 0; i < 2; i++) {
        int r = rank + pawn_dr;
        int f = file + PAWN_CAPTURE_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;
        Piece p = board->squares[get_piece_idx(r, f)];
        if (p.type == Pawn && p.color == by)
            return 1;
    }

    for (int i = 0; i < 8; i++) {
        int r = rank + KNIGHT_DR[i];
        int f = file + KNIGHT_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;
        Piece p = board->squares[get_piece_idx(r, f)];
        if (p.type == Knight && p.color == by)
            return 1;
    }

    for (int i = 0; i < 8; i++) {
        int r = rank + KING_DR[i];
        int f = file + KING_DF[i];
        if (r < 0 || r > 7 || f < 0 || f > 7)
            continue;
        Piece p = board->squares[get_piece_idx(r, f)];
        if (p.type == King && p.color == by)
            return 1;
    }

    for (int i = 0; i < 4; i++) {
        int r = rank, f = file;
        for (;;) {
            r += BISHOP_DR[i];
            f += BISHOP_DF[i];
            if (r < 0 || r > 7 || f < 0 || f > 7)
                break;
            Piece p = board->squares[get_piece_idx(r, f)];
            if (p.type == Empty)
                continue;
            if (p.color == by && (p.type == Bishop || p.type == Queen))
                return 1;
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        int r = rank, f = file;
        for (;;) {
            r += ROOK_DR[i];
            f += ROOK_DF[i];
            if (r < 0 || r > 7 || f < 0 || f > 7)
                break;
            Piece p = board->squares[get_piece_idx(r, f)];
            if (p.type == Empty)
                continue;
            if (p.color == by && (p.type == Rook || p.type == Queen))
                return 1;
            break;
        }
    }

    return 0;
}

int is_king_in_check(Board *board, Color c) {
    Color enemy = (c == White) ? Black : White;
    for (int i = 0; i < BOARD_SIZE; i++) {
        Piece p = board->squares[i];
        if (p.type == King && p.color == c)
            return is_square_attacked(board, i, enemy);
    }
    return 0; // no king on the board (shouldn't happen mid-game)
}

static int is_castle_legal(Board *board, int from, int to, Piece piece) {
    if (piece.type != King || piece.hasmoved)
        return 0;

    int rank = from / 8;
    int from_file = from % 8;
    int to_file = to % 8;
    if (to != get_piece_idx(rank, 6) && to != get_piece_idx(rank, 2))
        return 0;

    int rook_file = (to_file == 6) ? 7 : 0;
    int step = (to_file == 6) ? 1 : -1;
    Piece rook = board->squares[get_piece_idx(rank, rook_file)];
    if (rook.type != Rook || rook.color != piece.color || rook.hasmoved)
        return 0;

    for (int f = from_file + step; f != rook_file; f += step)
        if (board->squares[get_piece_idx(rank, f)].type != Empty)
            return 0; // path between king and rook isn't clear

    Color enemy = (piece.color == White) ? Black : White;
    for (int f = from_file; f != to_file + step; f += step)
        if (is_square_attacked(board, get_piece_idx(rank, f), enemy))
            return 0; // king starts, crosses, or lands in check

    return 1;
}

int is_legal_move(Board *board, int from, int to, char special) {
    if (from < 0 || from > 63 || to < 0 || to > 63 || from == to)
        return 0;

    Piece piece = board->squares[from];
    if (piece.type == Empty)
        return 0;

    if (special == 'c') {
        if (!is_castle_legal(board, from, to, piece))
            return 0;
    } else {
        int moves[BOARD_SIZE], num_moves;
        gen_moves_for_piece(board, from, moves, &num_moves);

        int found = 0;
        for (int i = 0; i < num_moves; i++)
            if (moves[i] == to) { found = 1; break; }
        if (!found)
            return 0;

        int to_rank = to / 8;
        int back_rank = (piece.color == White) ? 7 : 0;
        if (special != '\0' && strchr(PROMO_CHARS, special)) {
            if (piece.type != Pawn || to_rank != back_rank)
                return 0; // promotion char only valid landing on the back rank
        } else if (special != '\0') {
            return 0; // unrecognized special for a non-castling move
        } else if (piece.type == Pawn && to_rank == back_rank) {
            return 0; // reaching the back rank requires a promotion choice
        }
    }

    Board temp = *board;
    apply_move(&temp, from, to, special);
    if (is_king_in_check(&temp, piece.color))
        return 0;

    return 1;
}

int has_legal_moves(Board *board, Color side) {
    int moves[BOARD_SIZE], num_moves;

    for (int pos = 0; pos < BOARD_SIZE; pos++) {
        Piece piece = board->squares[pos];
        if (piece.type == Empty || piece.color != side)
            continue;

        gen_moves_for_piece(board, pos, moves, &num_moves);
        int back_rank = (side == White) ? 7 : 0;
        for (int i = 0; i < num_moves; i++) {
            int to_rank = moves[i] / 8;
            char special = (piece.type == Pawn && to_rank == back_rank) ? 'q' : '\0';
            if (is_legal_move(board, pos, moves[i], special))
                return 1;
        }

        if (piece.type == King) {
            int rank = pos / 8;
            if (is_legal_move(board, pos, get_piece_idx(rank, 6), 'c'))
                return 1;
            if (is_legal_move(board, pos, get_piece_idx(rank, 2), 'c'))
                return 1;
        }
    }

    return 0;
}

int is_checkmate(Board *board, Color side) {
    return is_king_in_check(board, side) && !has_legal_moves(board, side);
}

int is_stalemate(Board *board, Color side) {
    return !is_king_in_check(board, side) && !has_legal_moves(board, side);
}
