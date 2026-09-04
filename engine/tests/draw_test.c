#include "ffi.h"
#include <stdio.h>

// Hand-verified draw-detection checks against the real FFI (not perft_test's
// internal-struct style, since Game is opaque outside ffi.c — these go
// through sc_new_game/sc_make_move/sc_outcome exactly as az/bindings does).
// Move sequences were searched/verified via az/bindings against this same
// engine before being hardcoded here.

static int failures = 0;

static void expect_outcome(const char *label, Game *game, FfiOutcome expected) {
    FfiOutcome got = sc_outcome(game);
    if (got == expected) {
        printf("[PASS] %s outcome=%d\n", label, got);
    } else {
        printf("[FAIL] %s outcome=%d, expected %d\n", label, got, expected);
        failures++;
    }
}

static void apply(Game *game, int from, int to) {
    FfiMoveResult r = sc_make_move(game, from, to, '\0');
    if (r != MOVE_OK) {
        printf("[FAIL] move (%d,%d) rejected: result=%d\n", from, to, r);
        failures++;
    }
}

// Two knights per side shuffle home<->out, returning to the exact starting
// position (including side to move) for the 3rd time on the 8th ply.
static void test_threefold_repetition(void) {
    Game *game = sc_new_game();
    int seq[8][2] = {
        {1, 18}, {57, 42}, {18, 1}, {42, 57},
        {1, 18}, {57, 42}, {18, 1}, {42, 57},
    };
    for (int i = 0; i < 7; i++)
        apply(game, seq[i][0], seq[i][1]);
    expect_outcome("repetition (before 3rd occurrence)", game, SC_ONGOING);
    apply(game, seq[7][0], seq[7][1]);
    expect_outcome("repetition (3rd occurrence)", game, SC_DRAW);
    sc_free_game(game);
}

// 8 pawn moves (opening lines for bishops/queens — resets the halfmove
// clock on the last one) followed by exactly 100 plies of knight/bishop/
// rook/queen shuffling with no captures and no pawn moves, searched to
// avoid ever repeating a position 3 times. The clock should hit 100 on
// the very last of those quiet plies and trigger a draw there, not before.
static void test_fifty_move_rule(void) {
    Game *game = sc_new_game();
    int seq[108][2] = {
        {12, 28}, {52, 36}, {11, 27}, {51, 35}, {14, 22}, {54, 46}, {9, 17}, {49, 41},
        {1, 11}, {57, 51}, {0, 1}, {51, 57}, {1, 9}, {57, 51}, {3, 12}, {51, 57},
        {4, 3}, {57, 51}, {3, 4}, {51, 57}, {5, 14}, {57, 51}, {4, 3}, {51, 57},
        {6, 23}, {57, 51}, {3, 4}, {51, 57}, {4, 5}, {57, 51}, {5, 6}, {51, 57},
        {9, 1}, {57, 51}, {1, 0}, {51, 57}, {2, 9}, {57, 51}, {0, 1}, {51, 57},
        {1, 2}, {57, 51}, {2, 3}, {51, 57}, {3, 4}, {57, 51}, {4, 5}, {51, 57},
        {5, 3}, {57, 51}, {3, 4}, {51, 57}, {4, 5}, {57, 51}, {5, 2}, {51, 57},
        {2, 1}, {57, 51}, {1, 0}, {51, 57}, {6, 5}, {57, 51}, {0, 1}, {51, 57},
        {1, 2}, {57, 51}, {2, 3}, {51, 57}, {3, 4}, {57, 51}, {4, 2}, {51, 57},
        {2, 3}, {57, 51}, {3, 4}, {51, 57}, {4, 1}, {57, 51}, {1, 0}, {51, 57},
        {5, 4}, {57, 51}, {0, 1}, {51, 57}, {1, 2}, {57, 51}, {2, 3}, {51, 57},
        {3, 1}, {57, 51}, {1, 2}, {51, 57}, {2, 3}, {57, 51}, {3, 0}, {51, 57},
        {4, 3}, {57, 51}, {0, 1}, {51, 57}, {1, 2}, {57, 51}, {2, 0}, {51, 57},
        {0, 1}, {57, 51}, {1, 2}, {51, 57},
    };

    for (int i = 0; i < 8; i++)
        apply(game, seq[i][0], seq[i][1]);
    expect_outcome("fifty-move (after pawn-move warmup)", game, SC_ONGOING);

    for (int i = 8; i < 107; i++)
        apply(game, seq[i][0], seq[i][1]);
    expect_outcome("fifty-move (99 quiet plies, one short)", game, SC_ONGOING);

    apply(game, seq[107][0], seq[107][1]);
    expect_outcome("fifty-move (100th quiet ply)", game, SC_DRAW);

    sc_free_game(game);
}

static void test_clone_is_independent(void) {
    Game *game = sc_new_game();
    apply(game, 12, 28); // e2-e4

    Game *clone = sc_clone_game(game);
    apply(clone, 52, 36); // e7-e5, on the clone only

    if (sc_turn(game) == sc_turn(clone)) {
        printf("[FAIL] clone should have diverged turn from original\n");
        failures++;
    } else {
        printf("[PASS] clone diverges independently from original\n");
    }

    sc_free_game(game);
    sc_free_game(clone);
}

int main(void) {
    test_threefold_repetition();
    test_fifty_move_rule();
    test_clone_is_independent();

    if (failures) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("all draw-detection checks passed\n");
    return 0;
}
