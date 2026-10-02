# Superchess ruleset: engine map and integration strategy

Written 2026-10-02 at `9d9ee95`. Line numbers are from that commit; function names are given so
the references survive edits. This is a plan for you to implement by hand, so it has no patches.

## Summary

- The engine is about 1100 lines of C and plays standard chess only. Nothing in it knows about a ruleset.
- Rules 1–5 are small additions to five generators plus matching edits in `is_square_attacked`.
- Rule 6 (king inherits neighbours' moves) and rules 8–9 (win condition) are the real work. They break three assumptions the code makes: one king per side, the king can never be captured, and a two-file king move is always castling.
- Recommended switch: one `Ruleset` field inside `Board`. No function signature changes in movegen or rules.
- Do two cleanups first (step 0): merge the three legal-move enumerators and fix the perft test, which does not compile today.
- Six rule readings need your decision (section 4). The plan uses my recommended reading for each and says where a different answer changes the work.

## 1. Baseline, as run this session

| What | Command | Result |
|---|---|---|
| Engine build | `cd engine && make && make lib` | up to date, x86-64 ELF |
| Perft test | `cd engine/tests && make test` | fails, see below |
| Draw test | `cd engine/tests && make test-draw` | fails the same way |
| Draw test, rebuilt in scratch | `cc ... tests/draw_test.c src/*.c` (no `main.c`) | 6 of 6 pass |
| Perft, scratch harness | calls `perft(&b, White, d)` | startpos 20 / 400 / 8902 / 197281 and kiwipete 48 / 2039 / 97862, all pass |
| Python tests | `PYTHONPATH=. ~/Code/python_venv_01_main/bin/python -m pytest -q az/tests` | 31 passed |

Failures, verbatim:

```
./perft_test: ./perft_test: cannot execute binary file
make: *** [Makefile:12: test] Error 126
```

`engine/tests/perft_test` and `engine/tests/draw_test` are tracked macOS arm64 binaries, so `make`
sees them as up to date and never rebuilds. Rebuilding `perft_test.c` from source then fails:

```
tests/perft_test.c:13:30: error: too few arguments to function ‘perft’; expected 3, have 1
tests/perft_test.c:29:9: error: ‘b’ undeclared (first use in this function)
tests/perft_test.c:60:5: error: implicit declaration of function ‘create_board’; did you mean ‘reset_board’?
tests/perft_test.c:61:5: error: ‘temp’ undeclared (first use in this function)
```

The test was written against an older API with global `b`, `temp`, `turn` and `perft(depth)`. The
engine itself is correct on the standard perft numbers; only the test file is stale.

The project `.venv` has no `pytest`, which is why the Python run used the fallback venv.

## 2. Engine map

| File | Lines | What it holds | Superchess impact |
|---|---|---|---|
| `include/piece.h`, `src/piece.c` | 41, 150 | `Piece` struct, direction tables, **all six `gen_*_moves` generators** (declared in `moves.h`, defined in `piece.c`) | rules 1–6 |
| `include/board.h`, `src/board.c` | 27, 153 | `Board` (64 `Piece`s), `reset_board`, FEN placement in and out, printing | holds the switch |
| `src/moves.c` | 60 | `apply_move`, `gen_moves_for_piece` dispatch | promotion to king |
| `src/rules.c` | 209 | `is_square_attacked`, `is_king_in_check`, `is_castle_legal`, `is_legal_move`, `has_legal_moves`, checkmate, stalemate, insufficient material | rules 2–9, most of the work |
| `src/perft.c` | 69 | `generate_legal_moves`, `perft` | merged in step 0 |
| `src/game.c`, `src/main.c` | 134, 33 | CLI loop, move parsing, game log files | switch, win condition |
| `src/ffi.c`, `include/ffi.h` | 212, 85 | opaque `Game` (board, turn, clocks, hash history), `sc_*` API | switch, outcome, castling detect |
| `src/zobrist.c` | 59 | position hash for repetition | none |
| `src/utils.c` | 23 | file writing, whitespace | none |

How a move flows today: a generator in `piece.c` fills `int moves[64]` with target squares
(pseudo-legal, meaning the shape is right but the king may be left in check).
`is_legal_move` (`rules.c:120`) checks the target is in that list, checks the promotion
character, then copies the board, applies the move and rejects it if the mover's king is attacked
(`rules.c:153-156`). Castling skips the generator and goes through `is_castle_legal` (`rules.c:92`).

Things in the current code that matter for superchess:

- **Legal moves are enumerated three times**: `has_legal_moves` (`rules.c:161`),
  `generate_legal_moves` (`perft.c:9`), `sc_legal_moves` (`ffi.c:113`). They already disagree:
  perft tries promotions `qrbn`, the FFI tries `qbrnk`.
- **The end-of-game chain is written twice**: `game.c:108-123` and `refresh_outcome`
  (`ffi.c:45`). The 50-move rule and repetition exist only in `ffi.c`.
- **Promotion to king is already legal in standard chess**: `PROMO_CHARS` (`rules.c:5`),
  `apply_move` (`moves.c:39`), `sc_legal_moves` (`ffi.c:131`), `sc_make_move` (`ffi.c:183`),
  `getmove` (`game.c:34`). A superchess rule leaked into the base game. Perft does not see it
  because it only tries four promotions.
- **`is_king_in_check` stops at the first king it finds** (`rules.c:82-88`). After a king
  promotion the second king is unprotected.
- **`is_square_attacked` repeats every piece shape by hand** (`rules.c:7-80`). It does not call
  the generators. Every new move that can capture must be added here too, or check detection goes
  silently wrong.
- **Generators trust the caller about piece type.** They read only the colour at `pos`, then
  generate by shape. Rule 6 can use this: calling `gen_rook_moves` on a king's square gives the
  rook moves a king there would have.
- **`slide` and every generator start with `*num_moves = 0`** (`piece.c:92`). To add moves to a
  sliding piece, call `slide` first and append after.
- **Callers size the move buffer as `int moves[BOARD_SIZE]`** (64). A deduplicated target list
  always fits. A list with duplicates does not: rule 6 can overflow it.

## 3. The switch

**Recommendation: a `Ruleset` enum (`Standard`, `Super`) stored as a field in `Board`.**

Why there: every movegen and rules function already takes `Board *`, and the board is copied by
value wherever a move is tried (`rules.c:153`, `perft.c:60`, `sc_clone_game`). A field in `Board`
reaches all of them with zero signature changes, and a copied board keeps its ruleset.

What it costs:

- `reset_board` (`board.c:7`) must take the ruleset or leave the field alone. `sc_reset_game`
  must keep the game's ruleset.
- `str_to_board` (`board.c:88`) does not set it. Tests must set it before loading a position.
- The hash does not need it, because it never changes during a game.

The alternative is a `Ruleset` parameter on every function. It keeps `Board` as pure position but
changes about 15 signatures and every call site. I would only pick it if you want the practice.

Inside the functions, branch with a plain `if (board->rules == Super)` next to the standard code.
No function pointers or tables: there are two rulesets and the differences are small.

How the switch reaches each layer:

| Layer | Change |
|---|---|
| `board.h` | add the enum and the field; `reset_board(Board *, Ruleset)` |
| CLI | `main.c:5` reads `argv` (for example `./superchess --super`); `start_game` (`game.c:62`) takes the ruleset and passes it to `reset_board` |
| FFI | `Game *sc_new_game(int ruleset)` (`ffi.h:48`); add `int sc_ruleset(Game *)`; `sc_reset_game` preserves it |
| `_native.py:43` | `lib.sc_new_game.argtypes = [ctypes.c_int]`, plus the new getter |
| `engine.py:69` | `Engine.__init__(self, ruleset=Ruleset.STANDARD)`, a `Ruleset` `IntEnum`. `clone()` needs no change, since `sc_clone_game` copies the whole struct |
| `az` | `Engine()` is constructed at `selfplay/worker.py:59` and `train/loop.py:150`. Add `--ruleset` in `train/cli.py`, pass it through `play_game` (the driver forwards kwargs at `selfplay/driver.py:28`) and the promotion match |
| C tests | `draw_test.c` calls `sc_new_game()` three times |

## 4. Rule readings to decide

Each one changes code, so settle them before the step that needs them.

| # | Question | Recommended reading | If you pick the other |
|---|---|---|---|
| A | Rule 1: is the pawn's backward step a plain move, or can it capture? | Plain move, straight back, onto an empty square only | A backward capture needs a new entry in `is_square_attacked` |
| B | Rule 2: do knights get the double push, en passant or promotion? | No. One step forward onto an empty square, diagonal capture of an enemy piece | Each extra needs its own special case in `apply_move` |
| C | Rule 6: which neighbours count, and how deep? | The 8 adjacent squares, both colours, one level (a neighbouring king gives nothing new). The king gets the superchess move set of each neighbour's type. From a pawn it gets one step forward, one step back, and diagonal capture, in the king's own direction, with no double push, en passant or promotion | Friendly-only is a one-line filter. Recursion through chains of kings needs a visited set and is much harder to test |
| D | Rules 8–9: when does check apply? | A side loses when it has no kings and no pawns. Check, checkmate and stalemate apply only to a side with **no pawns and exactly one king**. Otherwise kings are ordinary pieces: they may move into attack and be captured | If every king is royal once the pawns are gone, `is_king_in_check` must loop over all kings, and a fork of two kings is mate |
| E | An unmoved king's inherited rook or queen slide lands on the castling square (e1 to g1 or c1). Is that castling? | Try castling first; if castling is not legal, treat it as a normal move | The policy encoding cannot tell the two apart (`az/encoding.py:53-60`), so one of them must win |
| F | Rule 7 in standard chess | Promotion to king is rejected when the ruleset is `Standard` | Leaving it means standard mode is not standard chess |

Smaller ones, with the reading I assumed:

- A pawn that steps back onto its start rank may double-push again. The code already keys the
  double push on the rank (`piece.c:39`), so this is free.
- Stalemate stays a draw.
- Castling works as today. Under reading D, a side that is not subject to check may castle
  through attacked squares.

## 5. Rule by rule

For each rule: where the moves are generated, where attacks are detected, and what else it disturbs.

### Rule 1: pawns step backward

- **Generate:** `gen_pawn_moves` (`piece.c:22`). Add the square at `rank - dir` if it is on the
  board and empty.
- **Attack:** nothing, under reading A.
- **Side effects:** none in `apply_move`. The en passant removal (`moves.c:17`) triggers on a file
  change, and a backward step keeps the file. A pawn stepping back onto its own home rank is not a
  promotion, because the promotion check uses the far rank (`rules.c:142`).
- **Note:** the 50-move clock resets on any pawn move (`ffi.c:195`). Pawn moves are now
  reversible, so the clock is weaker in superchess. Repetition still catches shuffling.

### Rule 2: knights move like pawns

- **Generate:** `gen_knight_moves` (`piece.c:72`). After the jumps, add one step forward if empty
  and the two forward diagonals if they hold an enemy piece. Direction comes from the colour, as
  in `gen_pawn_moves`.
- **Attack:** the pawn block in `is_square_attacked` (`rules.c:11-21`) must also match a `Knight`.
- **Side effects:** none. The piece type is still `Knight`, so no promotion or en passant fires.

### Rule 3: bishops step like a king

- **Generate:** `gen_bishop_moves` (`piece.c:119`). After `slide`, add the four orthogonal single
  steps. The diagonal steps are already in the slide.
- **Attack:** the king block (`rules.c:34-43`) must match a `Bishop` on the four orthogonal
  neighbours. The block loops over all 8 king directions, so test the direction as well as the type.
- **Side effects:** bishops change square colour. `is_insufficient_material` is affected, see rules 8–9.

### Rule 4: rooks step like a king

- **Generate:** `gen_rook_moves` (`piece.c:123`). After `slide`, add the four diagonal single steps.
- **Attack:** the king block must match a `Rook` on the four diagonal neighbours.
- **Side effects:** none. A rook that steps diagonally sets `hasmoved` through `apply_move`, so
  castling rights are lost correctly.

### Rule 5: queens jump like a knight

- **Generate:** `gen_queen_moves` (`piece.c:127`). After `slide`, add the 8 knight jumps.
- **Attack:** the knight block (`rules.c:23-32`) must also match a `Queen`.

### Rule 6: kings inherit neighbours' moves

- **Generate:** `gen_king_moves` (`piece.c:131`). After the normal 8 steps, look at each of the 8
  neighbours. For each neighbour type that is not `Empty` or `King`, call that type's generator
  with the king's square into a scratch buffer and merge the results.
- **Deduplicate.** Use a `seen[64]` array while merging. Without it the 64-entry buffers in every
  caller can overflow (a queen neighbour alone gives up to 35 targets, and several neighbours overlap).
- **Pawn neighbour:** do not call `gen_pawn_moves` directly. It would give the king a double push
  from the start rank and en passant captures, and `apply_move` only removes the passed pawn when
  the mover's type is `Pawn` (`moves.c:17`). Write the three pawn-shaped king moves inline.
- **Attack:** this is the hard part. A king's attack set now depends on its neighbours, so the
  fixed pattern in `rules.c:34-43` is not enough. Simplest correct approach: for each enemy king on
  the board, run `gen_king_moves` and see whether the target square is in the list. That is exact
  when the target square is occupied by the piece under attack, which is the check-detection case.
  It is wrong for empty squares in the pawn-shaped case (a forward step is listed but is not a
  capture). Empty squares are only queried by the castling path check (`rules.c:113`), so handle
  the pawn shape explicitly there or accept the edge case and note it.
- **Castling collision:** `sc_make_move` treats any king move of two files as castling
  (`ffi.c:180`). A king next to a knight or queen can now jump or slide two files legitimately,
  and today that returns `MOVE_ILLEGAL`. Apply reading E: require same rank and target file 2 or 6
  for the castle attempt, and fall back to a normal move when `is_castle_legal` says no.
  `sc_legal_moves` (`ffi.c:148-161`) must not emit the same from/to pair twice, once as castle and
  once as a normal move.
- **CLI:** castling is explicit there (`c` suffix), so no collision.

### Rule 7: promotion to king

- Already implemented everywhere except `perft.c:12`. The work is the reverse: gate `k` on the
  ruleset at `rules.c:5`/`143`, `ffi.c:131` and `ffi.c:183`. After step 0 the promotion list lives
  in one place.
- A promoted king has `hasmoved` set by `apply_move` (`moves.c:31`), so it cannot castle. Good.

### Rules 8–9: win condition and check

Under reading D, define two small helpers in `rules.c`:

- **Eliminated:** a side has zero kings and zero pawns. That side has lost.
- **Royal:** a side has zero pawns and exactly one king. Only then do check rules apply to it.

Then:

- `is_legal_move` (`rules.c:153-156`): reject a move for leaving the king attacked only if the
  mover is royal on the board **after** the move. Evaluate after, because a side's last pawn can
  promote to its only king.
- `is_king_in_check`, `is_checkmate`, `is_stalemate` (`rules.c:82`, `183-189`): check and
  checkmate report true only for a royal side. Stalemate (no legal moves, not in check) is
  unchanged and can now happen to a non-royal side.
- `is_castle_legal` (`rules.c:112-115`): skip the attacked-square test for a non-royal side.
- `is_insufficient_material` (`rules.c:195`): in superchess, return true only for king against
  king. A lone bishop can change square colour and its king can borrow its moves, so the standard
  minor-piece draws do not hold. Leave the rest to the 50-move rule and repetition.
- **Outcome:** add an elimination check at the front of both end-of-game chains (`game.c:108`,
  `ffi.c:45`). The eliminated side is always the side to move, since the capture just happened.
- **New outcome value.** Add `SC_ELIMINATION = 4` to `FfiOutcome` (`ffi.h:14`) and `Elimination`
  to `Outcome` (`game.h:11`), with a message in `write_metafile` (`game.c:40`). The cheaper
  alternative is to report elimination as `SC_CHECKMATE`, which needs no Python changes but makes
  the name lie.
- `sc_winner` and the `winner` field (`ffi.c:19`, `ffi.h:59`) are documented as valid only for
  checkmate. Extend that to elimination.
- `apply_move` needs no change. Capturing a king is just a capture.
- A side with no king but with pawns keeps playing. `is_king_in_check` already returns 0 when it
  finds no king (`rules.c:89`).

## 6. Python side

- **Bindings:** section 3, plus `Outcome.ELIMINATION = 4` in `engine.py:26`. `Engine.winner()`
  (`engine.py:97`) returns `None` unless the outcome is checkmate; include elimination.
- **Decisive-result checks** that test `Outcome.CHECKMATE` only: `selfplay/worker.py:32` and `:85`,
  `mcts/search.py:32` (`_terminal_value`), `train/loop.py:165`. Each must treat elimination as a
  win. `_terminal_value` returning -1 for the side to move is still right, because the eliminated
  side is the side to move.
- **Encoding:** no shape changes. The policy is from × to × promo (`az/encoding.py:22`), every
  superchess move has a distinct from/to/promo triple except the castling collision in reading E,
  and `k` is already a promo slot. Board planes are per piece type, so several kings are several
  ones in the king plane.
- **Move count:** `SC_MAX_LEGAL_MOVES` is 256 (`ffi.h:46`, `_native.py:39`) and `perft.c:52` uses
  a fixed 256-entry array. Standard chess peaks at 218. Superchess has no known bound; queens and
  kings both gain moves. `Engine.legal_moves` raises on overflow (`engine.py:110`), but the perft
  array would be overrun. Raise both to 512.
- **Checkpoints:** a network trained on standard chess is wrong for superchess. Store the ruleset
  in the `extra` dict that `save_checkpoint` already takes (`train/checkpoint.py:8`) and check it
  on resume. `nn/pretrain.py` learns from human PGN games, which are standard chess only.

## 7. Order of work

Each step leaves standard chess passing and names its test.

**Step 0. Cleanups, no behaviour change.**
- Remove the tracked binaries `engine/tests/perft_test` and `engine/tests/draw_test`, and ignore them.
- Fix `perft_test.c` to the current API: a local `Board`, `reset_board`, `str_to_board`,
  `perft(&b, turn, depth)`.
- Merge the three enumerators into one function in `rules.c` that fills an array of
  from/to/special moves. `has_legal_moves`, `perft` and `sc_legal_moves` call it. `FfiMove`
  (`ffi.h:30`) already has the right shape.
- Move the end-of-game chain out of `game.c` and `ffi.c` into one function in `rules.c` that
  takes the board and the side to move. This is what lets C tests check outcomes from a FEN
  position without scripting whole games through the FFI.
- *Test:* `make test` shows the seven perft numbers from section 1; `make test-draw` passes 6 of 6.

**Step 1. Switch plumbing, standard behaviour only.**
- `Ruleset` in `Board`, `reset_board`, `sc_new_game(int)`, CLI flag, Python `Engine(ruleset)`.
- Gate promotion to king on the ruleset (reading F).
- *Test:* perft and draw tests unchanged; 31 Python tests pass; a new test that `k` promotion is
  rejected in standard mode.

**Step 2. Rules 1–5.** One rule at a time: generator, then the matching `is_square_attacked` edit.
- *Test:* a new `super_test.c` with one position per rule on an otherwise empty board, counting
  generator output by hand. Expected counts under the recommended readings:

  | Position (white piece, empty board) | Targets |
  |---|---|
  | pawn e4 | 2 (e5, e3) |
  | pawn e2 | 3 (e3, e4, e1) |
  | knight d4 | 9 (8 jumps, d5) |
  | bishop d4 | 17 (13 slides, 4 steps) |
  | rook d4 | 18 (14 slides, 4 steps) |
  | queen d4 | 35 (27 slides, 8 jumps) |

  Add one attack test for each of rules 2–5: an enemy king placed on the new target square must be reported
  as attacked.

**Step 3. Rule 6.**
- *Test:* white king d4 with a white rook on d3, empty board otherwise: 15 targets (7 king steps,
  plus d6 d7 d8 a4 b4 f4 g4 h4). A second position where two neighbours overlap, to prove the
  deduplication. Superchess start position `perft(1)` is **24**: the standard 20, plus the queen's
  jumps to c3 and e3, plus the king's jumps to d3 and f3 inherited from the queen. Recount if you
  change reading C.

**Step 4. Rules 8–9.**
- *Test:* FEN positions through the shared outcome function from step 0: a side with one pawn and
  no king is still playing; capturing the last pawn of a kingless side ends the game; a side with
  pawns may leave its king attacked; a side with one king and no pawns may not; two kings and no
  pawns are not in check.

**Step 5. FFI and bindings.**
- Castling detection in `sc_make_move`, `SC_ELIMINATION`, `sc_winner`, buffer size 512.
- *Test:* extend `az/tests/bindings/` with a superchess game that ends by elimination and a king
  making a two-file inherited move.

**Step 6. `az`.**
- Ruleset flag through CLI, self-play and promotion match; the four decisive-result checks; the
  checkpoint tag.
- *Test:* a random-playout smoke test in superchess mode: a few hundred games of uniformly random
  legal moves must finish without a crash, with every game ending in a recognised outcome or the
  ply cap. This is the cheapest broad check available, since no reference engine exists.

## 8. Testing without reference numbers

Standard chess has published perft counts. Superchess has none, so correctness rests on three things:

1. Standard perft must stay identical after every step. This proves the `if (Super)` branches do
   not leak.
2. Hand-counted single-piece positions, as in steps 2 and 3. They are small enough to verify on
   paper and they pin down each rule separately.
3. Random playouts through the FFI with internal consistency checks: every move from
   `sc_legal_moves` is accepted by `sc_make_move`, and the game always reaches an outcome.

Once the hand-counted numbers pass, record superchess `perft(1..3)` from the start position as
regression values. They do not prove correctness, but they catch accidental changes later.
