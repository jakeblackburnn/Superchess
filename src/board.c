#include "superchess.h"

int get_piece_idx(int rank, int file) {
    return (rank * 8) + file;
}

void reset_board(Board *board) {
    int i;
    for (i = 0; i < 64; i++)
        if (i / 8 == 0 || i / 8 == 7) {
            switch (i % 8) {
                case 0:
                    board->squares[i] = create_piece(Rook, COLOR(i));
                    break;
                case 7:
                    board->squares[i] = create_piece(Rook, COLOR(i));
                    break;
                case 1:
                    board->squares[i] = create_piece(Knight, COLOR(i));
                    break;
                case 6:
                    board->squares[i] = create_piece(Knight, COLOR(i));
                    break;
                case 2:
                    board->squares[i] = create_piece(Bishop, COLOR(i));
                    break;
                case 5:
                    board->squares[i] = create_piece(Bishop, COLOR(i));
                    break;
                case 3:
                    board->squares[i] = create_piece(Queen, COLOR(i));
                    break;
                case 4:
                    board->squares[i] = create_piece(King, COLOR(i));
                    break;
            }
        } else if (i / 8 == 1 || i / 8 == 6)
            board->squares[i] = create_piece(Pawn, COLOR(i));
        else
            board->squares[i] = create_piece(Empty, COLOR(i));
}

int str_to_idx(char *str) {
    int rank, file;
    file = str[0] - 'a';
    rank = str[1] - '1';
    if (rank < 0 || rank > 7 || file < 0 || file > 7)
        return -1;
    return get_piece_idx(rank, file);
}

PieceType get_piece_type(Board *board, int rank, int file) {
    return board->squares[get_piece_idx(rank, file)].type;
}

static char fen_char(PieceType type, Color color) {
    static const char letters[6] = { 'P', 'N', 'B', 'R', 'Q', 'K' };
    char c = letters[type];
    return (color == White) ? c : (char)(c + ('a' - 'A'));
}

void board_to_str(Board *board, char *out) {
    char *p = out;
    for (int rank = 7; rank >= 0; rank--) {
        int empty_run = 0;
        for (int file = 0; file < 8; file++) {
            Piece piece = board->squares[get_piece_idx(rank, file)];
            if (piece.type == Empty) {
                empty_run++;
                continue;
            }
            if (empty_run) {
                *p++ = (char)('0' + empty_run);
                empty_run = 0;
            }
            *p++ = fen_char(piece.type, piece.color);
        }
        if (empty_run)
            *p++ = (char)('0' + empty_run);
        if (rank > 0)
            *p++ = '/';
    }
    *p = '\0';
}

// FEN piece-placement carries no move-history, so pieces created here
// always get create_piece's zeroed hasmoved/passable.
void str_to_board(char *str, Board *board) {
    int rank = 7, file = 0;
    for (char *p = str; *p && rank >= 0; p++) {
        char c = *p;
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (c >= '1' && c <= '8') {
            int run = c - '0';
            for (int i = 0; i < run && file < 8; i++, file++)
                board->squares[get_piece_idx(rank, file)] = create_piece(Empty, White);
            continue;
        }
        if (file >= 8)
            continue;

        Color color = (c >= 'a' && c <= 'z') ? Black : White;
        char upper = (color == Black) ? (char)(c - ('a' - 'A')) : c;
        PieceType type;
        switch (upper) {
            case 'P': type = Pawn;   break;
            case 'N': type = Knight; break;
            case 'B': type = Bishop; break;
            case 'R': type = Rook;   break;
            case 'Q': type = Queen;  break;
            case 'K': type = King;   break;
            default:  continue;
        }
        board->squares[get_piece_idx(rank, file)] = create_piece(type, color);
        file++;
    }
}

void print_board(Board b, Color color) {
    int rank, file;
    Piece p;
    if (color == White) {
        for (rank = 7; rank >= 0; rank--) {
            printf("%d ", rank + 1);
            for (file = 0; file < 8; file++) {
                p = b.squares[get_piece_idx(rank, file)];
                if (p.type == Empty)
                    printf("· ");
                else
                    printf("%s ", GLYPHS[p.color][p.type]);
            }
            printf("\n");
        }
        printf("  a b c d e f g h\n");
    } else {
        for (rank = 0; rank < 8; rank++) {
            printf("%d ", rank + 1);
            for (file = 7; file >= 0; file--) {
                p = b.squares[get_piece_idx(rank, file)];
                if (p.type == Empty)
                    printf("· ");
                else
                    printf("%s ", GLYPHS[p.color][p.type]);
            }
            printf("\n");
        }
        printf("  h g f e d c b a\n");
    }
}
