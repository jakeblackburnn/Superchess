#include "superchess.h"
#include <string.h>
#include <stdlib.h>

static void append_move_log(GameResult *result, size_t *cap, size_t *len, const char *move) {
    size_t add = strlen(move) + 1; // +1 for trailing '\n'
    if (*len + add + 1 > *cap) {
        *cap = (*cap == 0) ? 256 : *cap * 2;
        while (*len + add + 1 > *cap)
            *cap *= 2;
        result->move_log = realloc(result->move_log, *cap);
    }
    memcpy(result->move_log + *len, move, strlen(move));
    *len += strlen(move);
    result->move_log[(*len)++] = '\n';
    result->move_log[*len] = '\0';
}

int getmove(char *buf) {
    if (!fgets(buf, MOVE_BUFSIZE, stdin))
        return 0; // failure

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    len = strlen(buf);

    if (len != 4 && len != 5)
        return 0; // failure

    if (str_to_idx(&buf[0]) == -1 || str_to_idx(&buf[2]) == -1)
        return 0; // failure

    if (len == 5 && !strchr("qbrnkc", buf[4]))
        return 0; // failure: not one of q/b/r/n/k/c

    return 1; // success
}

void write_metafile(GameResult *result) {
    char fen[FEN_BUFSIZE];
    board_to_str(&result->board, fen);

    char out[512];
    if (result->outcome == Checkmate)
        snprintf(out, sizeof(out), "checkmate \xe2\x80\x94 %s wins\nturns: %d\nboard: %s\n",
                 result->winner == White ? "white" : "black", result->turn_count, fen);
    else if (result->outcome == Stalemate)
        snprintf(out, sizeof(out), "stalemate\nturns: %d\nboard: %s\n",
                 result->turn_count, fen);
    else
        snprintf(out, sizeof(out), "draw \xe2\x80\x94 insufficient material\nturns: %d\nboard: %s\n",
                 result->turn_count, fen);

    write_to_file(result->metafile, out);
}

void write_playfile(GameResult *result) {
    write_to_file(result->playfile, result->move_log ? result->move_log : "");
}

void start_game(Board *board, GameResult *result, char *metafile_path, char *playfile_path) {
    reset_board(board);

    Color turn = White;
    int turn_count = 0;

    result->metafile = metafile_path;
    result->playfile = playfile_path;
    result->move_log = NULL;
    size_t log_cap = 0, log_len = 0;

    print_board(*board, White);

    for (;;) {
        print_whitespace(1);
        printf("%s to move: ", turn == White ? "white" : "black");

        char buf[MOVE_BUFSIZE];
        if (!getmove(buf)) {
            if (feof(stdin)) // eof / ctrl-d
                break;
            printf("invalid move\n");
            continue;
        }

        int from = str_to_idx(&buf[0]);
        int to = str_to_idx(&buf[2]);
        char special = (strlen(buf) == 5) ? buf[4] : '\0';

        if (board->squares[from].color != turn || from == to) {
            printf("illegal move\n");
            continue;
        }
        if (!is_legal_move(board, from, to, special)) {
            printf("illegal move\n");
            continue;
        }

        append_move_log(result, &log_cap, &log_len, buf);
        apply_move(board, from, to, special);
        turn = (turn == White) ? Black : White;
        turn_count++;

        print_whitespace(7);
        print_board(*board, turn);

        if (is_checkmate(board, turn)) {
            result->outcome = Checkmate;
            result->winner = (turn == White) ? Black : White;
            printf("checkmate \xe2\x80\x94 %s wins\n", result->winner == White ? "white" : "black");
            break;
        }
        if (is_stalemate(board, turn)) {
            result->outcome = Stalemate;
            printf("stalemate\n");
            break;
        }
        if (is_insufficient_material(board)) {
            result->outcome = Draw;
            printf("draw \xe2\x80\x94 insufficient material\n");
            break;
        }
    }

    result->turn_count = turn_count;
    result->board = *board;

    write_metafile(result);
    write_playfile(result);

    free(result->move_log);
    result->move_log = NULL;
}
