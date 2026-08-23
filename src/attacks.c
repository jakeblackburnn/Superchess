#include "superchess.h"

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

int king_in_check(Board *board, Color c) {
    Color enemy = (c == White) ? Black : White;
    for (int i = 0; i < BOARD_SIZE; i++) {
        Piece p = board->squares[i];
        if (p.type == King && p.color == c)
            return is_square_attacked(board, i, enemy);
    }
    return 0; // no king on the board (shouldn't happen mid-game)
}
