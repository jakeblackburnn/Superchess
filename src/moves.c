#include "superchess.h"
#include <string.h>

char move[MOVE_BUFSIZE];
int from;
int to;
char special;

// Valid special chars: q/b/r/n/k = promotion piece, c = castling.
static const char *SPECIAL_CHARS = "qbrnkc";
static const char *PROMO_CHARS = "qbrnk";

int getmove() {
    if (!fgets(move, MOVE_BUFSIZE, stdin))
        return 0; // failure

    size_t len = strlen(move);
    if (len > 0 && move[len - 1] == '\n')
        move[len - 1] = '\0';
    len = strlen(move);

    if (len != 4 && len != 5)
        return 0; // failure

    from = str_to_idx(&move[0]);
    to = str_to_idx(&move[2]);

    if (from == -1 || to == -1)
        return 0; // failure

    if (len == 5) {
        special = move[4];
        if (!strchr(SPECIAL_CHARS, special))
            return 0; // failure: not one of q/b/r/n/k/c
    } else {
        special = '\0';
    }
    return 1; // success
}

void apply_move(Board *board, int from, int to, char special) {
    Piece piece = board->squares[from];
    int from_rank = from / 8, from_file = from % 8;
    int to_rank = to / 8, to_file = to % 8;

    for (int i = 0; i < BOARD_SIZE; i++)
        if (board->squares[i].color == piece.color)
            board->squares[i].justmoved = 0;
    piece.justmoved = 0;

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
        piece.justmoved = 1;

    board->squares[to] = piece;
    board->squares[from] = create_piece(Empty, White);
}

void gen_moves_for_piece(int pos, int *moves, int *num_moves) {
    switch (get_piece_type(pos)) {
        case Pawn:   gen_pawn_moves(pos, moves, num_moves);   break;
        case Knight: gen_knight_moves(pos, moves, num_moves); break;
        case Bishop: gen_bishop_moves(pos, moves, num_moves); break;
        case Rook:   gen_rook_moves(pos, moves, num_moves);   break;
        case Queen:  gen_queen_moves(pos, moves, num_moves);  break;
        case King:   gen_king_moves(pos, moves, num_moves);   break;
        default:     *num_moves = 0; break;
    }
}

static int is_castle_legal(int from, int to, Piece piece) {
    if (piece.type != King || piece.hasmoved)
        return 0;

    int rank = from / 8;
    int from_file = from % 8;
    int to_file = to % 8;
    if (to != get_piece_idx(rank, 6) && to != get_piece_idx(rank, 2))
        return 0;

    int rook_file = (to_file == 6) ? 7 : 0;
    int step = (to_file == 6) ? 1 : -1;
    Piece rook = b.squares[get_piece_idx(rank, rook_file)];
    if (rook.type != Rook || rook.color != piece.color || rook.hasmoved)
        return 0;

    for (int f = from_file + step; f != rook_file; f += step)
        if (b.squares[get_piece_idx(rank, f)].type != Empty)
            return 0; // path between king and rook isn't clear

    Color enemy = (piece.color == White) ? Black : White;
    for (int f = from_file; f != to_file + step; f += step)
        if (is_square_attacked(&b, get_piece_idx(rank, f), enemy))
            return 0; // king starts, crosses, or lands in check

    return 1;
}

int is_legal_move(int from, int to, char special) {
    if (from < 0 || from > 63 || to < 0 || to > 63 || from == to)
        return 0;

    Piece piece = b.squares[from];
    if (piece.type == Empty || piece.color != turn)
        return 0;

    if (special == 'c') {
        if (!is_castle_legal(from, to, piece))
            return 0;
    } else {
        int moves[BOARD_SIZE], num_moves;
        gen_moves_for_piece(from, moves, &num_moves);

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

    temp = b;
    apply_move(&temp, from, to, special);
    if (king_in_check(&temp, piece.color))
        return 0;

    return 1;
}

int has_legal_moves(Color side) {
    int moves[BOARD_SIZE], num_moves;

    for (int pos = 0; pos < BOARD_SIZE; pos++) {
        Piece piece = b.squares[pos];
        if (piece.type == Empty || piece.color != side)
            continue;

        gen_moves_for_piece(pos, moves, &num_moves);
        int back_rank = (side == White) ? 7 : 0;
        for (int i = 0; i < num_moves; i++) {
            int to_rank = moves[i] / 8;
            char special = (piece.type == Pawn && to_rank == back_rank) ? 'q' : '\0';
            if (is_legal_move(pos, moves[i], special))
                return 1;
        }

        if (piece.type == King) {
            int rank = pos / 8;
            if (is_legal_move(pos, get_piece_idx(rank, 6), 'c'))
                return 1;
            if (is_legal_move(pos, get_piece_idx(rank, 2), 'c'))
                return 1;
        }
    }

    return 0;
}

int is_checkmate(Color side) {
    return king_in_check(&b, side) && !has_legal_moves(side);
}

int is_stalemate(Color side) {
    return !king_in_check(&b, side) && !has_legal_moves(side);
}

int str_to_idx(char *str) {
    int rank, file;
    file = str[0] - 'a';
    rank = str[1] - '1';
    if (rank < 0 || rank > 7 || file < 0 || file > 7)
        return -1;
    return get_piece_idx(rank, file);
}

