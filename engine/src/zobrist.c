#include "zobrist.h"

static unsigned long long piece_keys[BOARD_SIZE][6][2];
static unsigned long long hasmoved_keys[BOARD_SIZE];
static unsigned long long passable_keys[BOARD_SIZE];
static unsigned long long side_key;
static int initialized = 0;

// splitmix64 — deterministic, fixed-seed PRNG. Not for cryptographic use,
// just for spreading a fixed seed into well-distributed 64-bit keys so
// zobrist_hash is reproducible across runs/builds/platforms.
static unsigned long long next_key(unsigned long long *state) {
    unsigned long long z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static void zobrist_init(void) {
    if (initialized)
        return;

    unsigned long long state = 0x53757065724368ULL; // "SuperCh" — fixed seed
    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        for (int type = 0; type < 6; type++) {
            piece_keys[sq][type][White] = next_key(&state);
            piece_keys[sq][type][Black] = next_key(&state);
        }
        hasmoved_keys[sq] = next_key(&state);
        passable_keys[sq] = next_key(&state);
    }
    side_key = next_key(&state);

    initialized = 1;
}

unsigned long long zobrist_hash(const Board *board, Color turn) {
    zobrist_init();

    unsigned long long hash = 0;
    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        Piece p = board->squares[sq];
        if (p.type == Empty)
            continue;
        hash ^= piece_keys[sq][p.type][p.color];
        // hasmoved only affects future legality (and thus position
        // identity) for kings and rooks, via castling rights — a knight
        // that returns to its home square is the same position as one
        // that never left, even though its hasmoved flag stays set.
        if ((p.type == King || p.type == Rook) && p.hasmoved)
            hash ^= hasmoved_keys[sq];
        if (p.passable)
            hash ^= passable_keys[sq];
    }
    if (turn == Black)
        hash ^= side_key;

    return hash;
}
