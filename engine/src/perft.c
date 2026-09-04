#include "superchess.h"

typedef struct {
    int from;
    int to;
    char special;
} PerftMove;

static int generate_legal_moves(Board *board, Color side, PerftMove *out) {
    int moves[BOARD_SIZE], num_moves;
    int count = 0;
    static const char promo_chars[4] = { 'q', 'r', 'b', 'n' };

    for (int pos = 0; pos < BOARD_SIZE; pos++) {
        Piece piece = board->squares[pos];
        if (piece.type == Empty || piece.color != side)
            continue;

        gen_moves_for_piece(board, pos, moves, &num_moves);
        int back_rank = (side == White) ? 7 : 0;

        for (int i = 0; i < num_moves; i++) {
            int to_rank = moves[i] / 8;
            if (piece.type == Pawn && to_rank == back_rank) {
                for (int p = 0; p < 4; p++)
                    if (is_legal_move(board, pos, moves[i], promo_chars[p]))
                        out[count++] = (PerftMove){ pos, moves[i], promo_chars[p] };
            } else if (is_legal_move(board, pos, moves[i], '\0')) {
                out[count++] = (PerftMove){ pos, moves[i], '\0' };
            }
        }

        // Castling isn't in gen_king_moves's output; probe it directly.
        if (piece.type == King) {
            int rank = pos / 8;
            int kingside = get_piece_idx(rank, 6);
            int queenside = get_piece_idx(rank, 2);
            if (is_legal_move(board, pos, kingside, 'c'))
                out[count++] = (PerftMove){ pos, kingside, 'c' };
            if (is_legal_move(board, pos, queenside, 'c'))
                out[count++] = (PerftMove){ pos, queenside, 'c' };
        }
    }

    return count;
}

unsigned long long perft(Board *board, Color turn, int depth) {
    if (depth == 0)
        return 1;

    PerftMove moves[256];
    int num_moves = generate_legal_moves(board, turn, moves);

    if (depth == 1)
        return (unsigned long long)num_moves;

    unsigned long long nodes = 0;
    for (int i = 0; i < num_moves; i++) {
        Board saved_board = *board;

        apply_move(board, moves[i].from, moves[i].to, moves[i].special);
        nodes += perft(board, (turn == White) ? Black : White, depth - 1);

        *board = saved_board;
    }

    return nodes;
}
