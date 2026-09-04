#include "superchess.h"
#include "ffi.h"
#include "zobrist.h"
#include <stdlib.h>
#include <string.h>

// Bounds the position-history table used for repetition detection. Well
// past any game self-play will realistically reach; the Python-side
// max-ply cutoff is the real backstop for pathological games, so once a
// game exceeds this, repetition detection simply stops recording further
// history rather than doing anything unsafe.
#define MAX_HISTORY 300

struct Game {
    Board board;
    Color turn;
    int fullmove_number;
    FfiOutcome outcome;
    Color winner; // valid only when outcome == SC_CHECKMATE
    int halfmove_clock; // plies since the last pawn move or capture
    unsigned long long hash_history[MAX_HISTORY];
    int history_len;
};

static int count_repetitions(Game *game) {
    if (game->history_len == 0)
        return 0;

    unsigned long long current = game->hash_history[game->history_len - 1];
    int count = 0;
    for (int i = 0; i < game->history_len; i++) {
        if (game->hash_history[i] == current)
            count++;
    }
    return count;
}

static void record_position(Game *game) {
    if (game->history_len < MAX_HISTORY) {
        game->hash_history[game->history_len++] =
            zobrist_hash(&game->board, game->turn);
    }
}

static void refresh_outcome(Game *game) {
    if (is_checkmate(&game->board, game->turn)) {
        game->outcome = SC_CHECKMATE;
        game->winner = (game->turn == White) ? Black : White;
    } else if (is_stalemate(&game->board, game->turn)) {
        game->outcome = SC_STALEMATE;
    } else if (is_insufficient_material(&game->board)) {
        game->outcome = SC_DRAW;
    } else if (game->halfmove_clock >= 100) {
        game->outcome = SC_DRAW;
    } else if (count_repetitions(game) >= 3) {
        game->outcome = SC_DRAW;
    } else {
        game->outcome = SC_ONGOING;
    }
}

Game *sc_new_game(void) {
    Game *game = malloc(sizeof(Game));
    if (!game)
        return NULL;
    sc_reset_game(game);
    return game;
}

void sc_free_game(Game *game) {
    free(game);
}

Game *sc_clone_game(Game *game) {
    Game *clone = malloc(sizeof(Game));
    if (!clone)
        return NULL;
    memcpy(clone, game, sizeof(Game));
    return clone;
}

void sc_reset_game(Game *game) {
    reset_board(&game->board);
    game->turn = White;
    game->fullmove_number = 0;
    game->outcome = SC_ONGOING;
    game->winner = White;
    game->halfmove_clock = 0;
    game->history_len = 0;
    record_position(game);
}

int sc_turn(Game *game) {
    return game->turn;
}

int sc_outcome(Game *game) {
    return game->outcome;
}

int sc_winner(Game *game) {
    return game->winner;
}

int sc_halfmove_clock(Game *game) {
    return game->halfmove_clock;
}

int sc_in_check(Game *game) {
    return is_king_in_check(&game->board, game->turn);
}

int sc_legal_moves(Game *game, FfiMove *out, int max_moves) {
    Board *board = &game->board;
    Color side = game->turn;
    int count = 0;

    for (int pos = 0; pos < BOARD_SIZE; pos++) {
        Piece piece = board->squares[pos];
        if (piece.type == Empty || piece.color != side)
            continue;

        int moves[BOARD_SIZE], num_moves;
        gen_moves_for_piece(board, pos, moves, &num_moves);
        int back_rank = (side == White) ? 7 : 0;

        for (int i = 0; i < num_moves; i++) {
            int to = moves[i];

            if (piece.type == Pawn && RANK(to) == back_rank) {
                static const char promos[] = "qbrnk";
                for (int k = 0; k < 5; k++) {
                    if (is_legal_move(board, pos, to, promos[k])) {
                        if (count < max_moves)
                            out[count] = (FfiMove){pos, to, promos[k]};
                        count++;
                    }
                }
            } else if (is_legal_move(board, pos, to, '\0')) {
                if (count < max_moves)
                    out[count] = (FfiMove){pos, to, '\0'};
                count++;
            }
        }

        // Castling isn't reachable through gen_moves_for_piece, so probe
        // both destinations directly.
        if (piece.type == King) {
            int rank = RANK(pos);
            int kingside = get_piece_idx(rank, 6);
            int queenside = get_piece_idx(rank, 2);
            if (is_legal_move(board, pos, kingside, 'c')) {
                if (count < max_moves)
                    out[count] = (FfiMove){pos, kingside, 'c'};
                count++;
            }
            if (is_legal_move(board, pos, queenside, 'c')) {
                if (count < max_moves)
                    out[count] = (FfiMove){pos, queenside, 'c'};
                count++;
            }
        }
    }

    return count;
}

FfiMoveResult sc_make_move(Game *game, int from, int to, char promo) {
    if (game->outcome != SC_ONGOING)
        return MOVE_GAME_OVER;

    if (from < 0 || from > 63 || to < 0 || to > 63)
        return MOVE_ILLEGAL;

    Piece piece = game->board.squares[from];
    if (piece.type == Empty || piece.color != game->turn)
        return MOVE_ILLEGAL;

    char special;
    if (piece.type == King && abs(FILE(to) - FILE(from)) == 2) {
        special = 'c'; // auto-detected; caller never passes 'c' explicitly
    } else if (promo != '\0') {
        if (!strchr("qbrnk", promo))
            return MOVE_ILLEGAL;
        special = promo;
    } else if (piece.type == Pawn && RANK(to) == ((game->turn == White) ? 7 : 0)) {
        return MOVE_PROMOTION_REQUIRED;
    } else {
        special = '\0';
    }

    if (!is_legal_move(&game->board, from, to, special))
        return MOVE_ILLEGAL;

    int resets_clock = (piece.type == Pawn) || (game->board.squares[to].type != Empty);

    apply_move(&game->board, from, to, special);
    game->turn = (game->turn == White) ? Black : White;
    game->fullmove_number++;
    game->halfmove_clock = resets_clock ? 0 : game->halfmove_clock + 1;
    record_position(game);
    refresh_outcome(game);

    return MOVE_OK;
}

void sc_board_array(Game *game, FfiSquare *out64) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        Piece p = game->board.squares[i];
        out64[i] = (FfiSquare){p.type, p.color, p.hasmoved, p.passable};
    }
}
