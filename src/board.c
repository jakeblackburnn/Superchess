#include "superchess.h"

#define COLOR(idx) (idx / 8 < 4 ? White : Black)

int get_piece_idx(int rank, int file) {
    return (rank * 8) + file;
}

Board create_board(void) {
    int i;
    Board b;

    for (i = 0; i < 64; i++)
        if (i / 8 == 0 || i / 8 == 7) {
            switch (i % 8) {
                case 0:
                    b.squares[i] = create_piece(Rook, COLOR(i));
                    break;
                case 7:
                    b.squares[i] = create_piece(Rook, COLOR(i));
                    break;
                case 1:
                    b.squares[i] = create_piece(Knight, COLOR(i));
                    break;
                case 6:
                    b.squares[i] = create_piece(Knight, COLOR(i));
                    break;
                case 2:
                    b.squares[i] = create_piece(Bishop, COLOR(i));
                    break;
                case 5:
                    b.squares[i] = create_piece(Bishop, COLOR(i));
                    break;
                case 3:
                    b.squares[i] = create_piece(Queen, COLOR(i));
                    break;
                case 4:
                    b.squares[i] = create_piece(King, COLOR(i));
                    break;
            }
        } else if (i / 8 == 1 || i / 8 == 6)
            b.squares[i] = create_piece(Pawn, COLOR(i));
        else
            b.squares[i] = create_piece(Empty, COLOR(i));

    return b;
}

static const char *GLYPHS[2][6] = {
    {"♙", "♘", "♗", "♖", "♕", "♔"},
    {"♟", "♞", "♝", "♜", "♛", "♚"},
};

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
