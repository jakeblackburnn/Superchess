#include "superchess.h"

int main() {
    print_whitespace(20);
    printf("Superchess\n");
    print_whitespace(1);

    Board b = create_board();
    print_board(b, White);
    print_whitespace(1);
    print_board(b, Black);

    char move[MOVE_BUFSIZE];

    printf("white to move: ");
    if (getmove(move))
        printf("your move: %s\n", move);

    return 0;
}
