#include "superchess.h"
#include <string.h>

// Valid special chars: q/b/r/n/k = promotion piece, c = castling.
static const char *SPECIAL_CHARS = "qbrnkc";

void apply_move(Board *board, int from, int to, char special) {
    Piece piece = board->squares[from];
    int from_rank = from / 8, from_file = from % 8;
    int to_rank = to / 8, to_file = to % 8;

    for (int i = 0; i < BOARD_SIZE; i++)
        if (board->squares[i].color == piece.color)
            board->squares[i].passable = 0;
    piece.passable = 0;

    if (piece.type == Pawn && from_file != to_file && board->squares[to].type == Empty)
        board->squares[get_piece_idx(from_rank, to_file)] = create_piece(Empty, White);

    if (special == 'c' && piece.type == King) {
        int rook_from_file = (to_file == 6) ? 7 : 0;
        int rook_to_file   = (to_file == 6) ? 5 : 3;
        int rook_from = get_piece_idx(from_rank, rook_from_file);
        int rook_to   = get_piece_idx(from_rank, rook_to_file);

        board->squares[rook_to] = board->squares[rook_from];
        board->squares[rook_to].hasmoved = 1;
        board->squares[rook_from] = create_piece(Empty, White);
    }

    piece.hasmoved = 1;

    if (special != 'c' && special != '\0' && strchr(SPECIAL_CHARS, special)) {
        switch (special) {
            case 'q': piece.type = Queen; break;
            case 'b': piece.type = Bishop; break;
            case 'r': piece.type = Rook; break;
            case 'n': piece.type = Knight; break;
            case 'k': piece.type = King; break;
        }
    }

    if (piece.type == Pawn && (to_rank - from_rank == 2 || to_rank - from_rank == -2))
        piece.passable = 1;

    board->squares[to] = piece;
    board->squares[from] = create_piece(Empty, White);
}

void gen_moves_for_piece(Board *board, int pos, int *moves, int *num_moves) {
    switch (board->squares[pos].type) {
        case Pawn:   gen_pawn_moves(board, pos, moves, num_moves);   break;
        case Knight: gen_knight_moves(board, pos, moves, num_moves); break;
        case Bishop: gen_bishop_moves(board, pos, moves, num_moves); break;
        case Rook:   gen_rook_moves(board, pos, moves, num_moves);   break;
        case Queen:  gen_queen_moves(board, pos, moves, num_moves);  break;
        case King:   gen_king_moves(board, pos, moves, num_moves);   break;
        default:     *num_moves = 0; break;
    }
}
