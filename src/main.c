#include "superchess.h"
#include <sys/stat.h>
#include <time.h>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    mkdir("game_history", 0755);

    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", localtime(&now));

    char dir[64];
    snprintf(dir, sizeof(dir), "game_history/%s", timestamp);
    mkdir(dir, 0755);

    char metafile[96], playfile[96];
    snprintf(metafile, sizeof(metafile), "%s/meta.txt", dir);
    snprintf(playfile, sizeof(playfile), "%s/play.txt", dir);

    Board board;
    GameResult result;

    print_whitespace(20);
    printf("Superchess\n");
    print_whitespace(1);

    start_game(&board, &result, metafile, playfile);

    return 0;
}
