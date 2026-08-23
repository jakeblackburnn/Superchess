#include "superchess.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    create_board();
    temp = b;

    print_whitespace(20);
    printf("Superchess\n");
    print_whitespace(1);

    print_board(b, White);

    for (;;) {
        print_whitespace(1);
        printf("%s to move: ", turn == White ? "white" : "black");

        if (!getmove()) {
            if (feof(stdin)) // eof / ctrl-d
                break; 
            printf("invalid move\n");
            continue;
        }

        if (!is_legal_move(from, to, special)) {
            printf("illegal move\n");
            continue;
        }

        apply_move(&b, from, to, special);
        turn = (turn == White) ? Black : White;

        print_whitespace(20);
        print_board(b, turn);

        if (is_checkmate(turn)) {
            printf("checkmate — %s wins\n", turn == White ? "black" : "white");
            break;
        }
        if (is_stalemate(turn)) {
            printf("stalemate\n");
            break;
        }
    }

    return 0;
}
