#include "superchess.h"
#include <ctype.h>

// Standalone perft regression check. Builds against the real src/*.c
// engine (not main.c — see tests/Makefile) and compares node counts at
// fixed depths against known-correct values from established chess
// engines. Not wired into the `superchess` binary; run via `make test`
// in this directory.

static int failures = 0;

static void check_perft(const char *label, int depth, unsigned long long expected) {
    unsigned long long got = perft(depth);
    if (got == expected) {
        printf("[PASS] %-10s perft(%d) = %llu\n", label, depth, got);
    } else {
        printf("[FAIL] %-10s perft(%d) = %llu, expected %llu\n", label, depth, got, expected);
        failures++;
    }
}

// Minimal FEN *piece placement* loader (just the first field) — enough to
// set up known test positions without hand-computing 64 square indices.
// hasmoved is left at 0 for every piece (create_piece's default), so a
// king/rook placed on its home square is automatically "eligible" to
// castle, matching a fully-permissive castling-rights field like KQkq.
static void load_fen_placement(const char *fen) {
    for (int i = 0; i < BOARD_SIZE; i++)
        b.squares[i] = create_piece(Empty, White);

    int rank = 7, file = 0;
    for (const char *c = fen; *c && *c != ' '; c++) {
        if (*c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (*c >= '1' && *c <= '8') {
            file += *c - '0';
            continue;
        }

        Color color = isupper((unsigned char)*c) ? White : Black;
        PieceType type;
        switch (tolower((unsigned char)*c)) {
            case 'p': type = Pawn;   break;
            case 'n': type = Knight; break;
            case 'b': type = Bishop; break;
            case 'r': type = Rook;   break;
            case 'q': type = Queen;  break;
            case 'k': type = King;   break;
            default: continue;
        }
        b.squares[get_piece_idx(rank, file)] = create_piece(type, color);
        file++;
    }
}

static int run_perft_tests(void) {
    create_board();
    temp = b;
    turn = White;
    check_perft("startpos", 1, 20ULL);
    check_perft("startpos", 2, 400ULL);
    check_perft("startpos", 3, 8902ULL);
    check_perft("startpos", 4, 197281ULL);

    // "Kiwipete" — a well-known perft stress position that packs
    // castling (both sides, both colors), en passant, promotions, and
    // pins onto one board, unlike the quiet opening position above.
    load_fen_placement("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R");
    turn = White;
    check_perft("kiwipete", 1, 48ULL);
    check_perft("kiwipete", 2, 2039ULL);
    check_perft("kiwipete", 3, 97862ULL);

    if (failures) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("all perft checks passed\n");
    return 0;
}

// superchess.h declares `int main(int, char **)` for the real project
// binary; match that signature here since the whole engine's headers
// (and this file's #include of superchess.h) are shared with it.
int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return run_perft_tests();
}
