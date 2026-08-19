#include "superchess.h"
#include <string.h>

int getmove(char *move) {
    if (!fgets(move, MOVE_BUFSIZE, stdin))
        return 0; // failure
    size_t len = strlen(move);
    if (len > 0 && move[len - 1] == '\n')
        move[len - 1] = '\0';
    return 1; // success
}
