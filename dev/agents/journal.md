# Journal · 2026-10-02 12:24 · main · 9d9ee95
## Start
**State:** `dev/` exists (`main.md` rules/roadmap, `journal.md` with the Oct 2 session goal). C engine in `engine/` plays standard chess only; AlphaZero Python in `az/`. `checkpoints/`, `logs/` untracked.
**Next (carried):** write `dev/agents/project/notes.md` (verified build/test/train commands); decide `.gitignore` for `checkpoints/` and `logs/`, project `CLAUDE.md`.
**Traps:** none known.
**Pointers:** `dev/main.md` (rules, roadmap), `dev/journal.md` (session goal), `README.md`.

## Task: Report on engine structure and superchess integration strategy · 12:24
**Goal:** give Jake a report that lets him hand-write the superchess ruleset in C and wire it through the CLI, FFI and `az/` without discovering the structure as he goes.
**Now:**
- No ruleset concept exists: `grep -i "variant\|ruleset"` over `engine/` and `az/` finds nothing. `sc_new_game(void)` (`engine/include/ffi.h:48`) and `main` (`engine/src/main.c:5`) take no options.
- Moves: per-piece pseudo-legal generators in `engine/src/piece.c:22-150`, dispatched by `gen_moves_for_piece` (`engine/src/moves.c:50`), filtered by `is_legal_move` (`engine/src/rules.c:120`).
- Standard chess is baked in at: `is_square_attacked` (`rules.c:7`, piece shapes hand-duplicated from the generators), `is_king_in_check` (`rules.c:82`, stops at the first king), `is_checkmate`/`is_stalemate` (`rules.c:183-189`), `is_insufficient_material` (`rules.c:195`).
- Legal-move enumeration is written three times: `has_legal_moves` (`rules.c:161`), `generate_legal_moves` (`engine/src/perft.c:9`), `sc_legal_moves` (`engine/src/ffi.c:113`). Promotion to king is already accepted in standard chess (`moves.c:39`, `rules.c:5`, `ffi.c:131`); perft omits it (`perft.c:12`).
- `az/encoding.py:3-6` picked a from×to×promo policy (24576) so new move shapes don't change the head; `PROMO_ORDER` already has `k` (`az/encoding.py:19`).
**Scope:** in: one report covering (a) engine map, module by module, (b) every site each of rules 1–9 touches, (c) where the ruleset switch lives and how it reaches CLI, FFI, bindings, self-play, (d) an implementation order with a test for each step, (e) rule ambiguities in `dev/main.md` listed as decisions for Jake. Out: writing any engine or `az/` code, refactors, `notes.md`, `.gitignore`, retraining.
**Constraints:** Jake implements by hand (`dev/journal.md:5`), so the report describes and points; signatures or a few lines of pseudocode at most, no patches. Ruleset is a runtime option chosen at game setup (`dev/main.md:27`). Standard chess must keep passing perft. Don't edit `dev/main.md` or `dev/journal.md`. Assumptions: report goes to `dev/agents/project/superchess-ruleset.md`; "rest of the project" means CLI, FFI, Python bindings, encoding, self-play and tests.
**Approach:**
- Read the rest of the engine (`board.c`, `ffi.c`, `zobrist.c`, tests) and the `az/` call sites of the bindings; build and run `make test`, `make test-draw`, and the `az` tests to record the baseline.
- Walk rules 1–9 one at a time against the code and list each touched function with `path:line`.
- Recommend one switch design (a ruleset field carried with the game state and passed to movegen/rules), and name the alternative only where it changes Jake's work.
- Order the steps so each is testable alone: switch plumbing with standard behavior unchanged → simple move additions (1–5) → king neighbor moves (6) → multi-king and win condition (7–9) → FFI/bindings → `az/`.
- Propose how to test superchess without reference perft numbers (hand-counted positions, per-rule unit positions).
**Risks:** rule 6 and rules 8–9 are underspecified, so a strategy built on a wrong reading misleads; the report flags each reading it assumes. The report could turn into the implementation; keep code out. Line references rot once Jake starts editing; cite function names alongside lines.
**Done when:**
- `dev/agents/project/superchess-ruleset.md` exists and each of rules 1–9 has the list of functions to change, with `path:line`.
- It states where the ruleset flag is stored and the exact signature changes needed in `ffi.h`, `_native.py`, `engine.py` and the CLI.
- It gives an ordered step list where every step names the test that proves it.
- It records baseline test results as run this session, with failures quoted verbatim.
- It lists the rule ambiguities with a recommended reading each.
- `git status` shows no changes outside `dev/agents/`.
**Open:**
1. Rule 6: do "neighboring pieces" include enemy pieces, and does a king next to a king inherit that king's inherited moves? Recommend: friendly and enemy both count, one level only (no recursion).
2. Rules 8–9: while a side still has pawns, may its king be left in or moved into check and simply captured? Recommend: yes, check only constrains a side with no pawns left; that reading makes 8 and 9 consistent.
3. Should the three duplicated legal-move enumerators be merged as a prerequisite step? Recommend: yes, as step 0 in the report, since every superchess rule otherwise has to be mirrored three times.

## Close · 2026-10-02 12:30 · 9d9ee95..636abc2
- **changed:** `dev/agents/project/superchess-ruleset.md`: new report (engine map, per-rule change sites, switch design, ordered steps with tests, six rule readings to decide)
- **changed:** `dev/agents/project/notes.md`: new, verified commands and traps
- **why:** Jake implements the superchess ruleset by hand; the report is the map and the order of work
- **verified:** scratch perft harness → startpos 20/400/8902/197281, kiwipete 48/2039/97862, all pass. `draw_test.c` rebuilt in scratch → 6 of 6 pass. `PYTHONPATH=. ~/Code/python_venv_01_main/bin/python -m pytest -q az/tests` → 31 passed. `cd engine/tests && make test` → `./perft_test: cannot execute binary file` (Error 126); `make test-draw` same. `perft_test.c` rebuilt → `error: too few arguments to function ‘perft’; expected 3, have 1`.
- **not verified:** the hand-counted superchess numbers in the report (24 at the start position, 15 for king d4 + rook d3, the single-piece table) are counted on paper, no code ran them.
- **by:** claude
- `636abc2` notes: Oct 2 session goal, superchess ruleset report
- 3 files in `dev/`, no code changed
- ⚠ uncommitted: `checkpoints/`, `logs/` (untracked run artifacts, not from this session)

### Next
1. Jake: decide readings A–F in `dev/agents/project/superchess-ruleset.md` section 4. D (when check applies) and C (rule 6 neighbours) change the most code.
2. Step 0 of the report: `git rm` the two tracked test binaries, fix `engine/tests/perft_test.c` to `perft(Board *, Color, int)`, merge the three legal-move enumerators, share the end-of-game chain.
3. Step 1: `Ruleset` field in `Board`, `sc_new_game(int)`, CLI flag, `Engine(ruleset)`.
4. Still open from before: `.gitignore` for `checkpoints/` and `logs/`; project `CLAUDE.md`.
### Traps
- C tests cannot run as committed: stale macOS binaries are tracked and `perft_test.c` is written against an old API. Details in `dev/agents/project/notes.md`.
- Project `.venv` has no `pytest`. Run from the repo root with `PYTHONPATH=.` and the fallback venv.
- `sc_make_move` treats any two-file king move as castling (`engine/src/ffi.c:180`); rule 6 makes such moves legal as normal moves.
- Promotion to king is already accepted in standard mode (`engine/src/rules.c:5`), and `is_king_in_check` only looks at the first king (`engine/src/rules.c:82`).
### Pointers
`dev/agents/project/superchess-ruleset.md` (the report), `dev/agents/project/notes.md`, `engine/src/rules.c` (`is_square_attacked`, `is_legal_move`), `engine/src/piece.c` (generators), `engine/src/ffi.c:45` (`refresh_outcome`).
