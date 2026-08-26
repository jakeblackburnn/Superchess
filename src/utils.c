#include "superchess.h"

void print_whitespace(int n) {
    for (int i = 0; i < n; i++) {
        printf("\n");
    }
}

void write_to_file(char *path, char *content) {
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fputs(content, f);
    fclose(f);
}

void append_to_file(char *path, char *content) {
    FILE *f = fopen(path, "a");
    if (!f)
        return;
    fputs(content, f);
    fclose(f);
}
