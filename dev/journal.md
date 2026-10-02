# Superchess dev journal

## Oct 2 2026
I'm using superchess as a practice project for learning C.
I want to implement the superchess ruleset and wire it in with the rest of the project manually.

session goal: figure out how to do this before implementing.
agent deliverable: a report on the engine structure and how to seamlessly add superchess ruleset
and integrate with the rest of the project.


## Results: (summarize the implementation strategy)

## Results · 2026-10-02 12:30 (agent)
1. Report on engine structure and how to add the superchess ruleset: done — `dev/agents/project/superchess-ruleset.md`

Strategy in short:
- Switch: a `Ruleset` field in `Board`, so no movegen or rules signatures change. It reaches the CLI by a flag, the FFI by `sc_new_game(int)`, Python by `Engine(ruleset)`.
- Step 0 first: fix the perft test (it does not compile, and the tracked test binaries are macOS builds), merge the three legal-move enumerators, share the end-of-game chain.
- Rules 1–5: one generator edit each in `piece.c` plus the matching edit in `is_square_attacked`.
- Rule 6: king calls its neighbours' generators from its own square and merges with dedup. Attack detection and the "two-file king move means castling" shortcut in `ffi.c` both break and need care.
- Rules 8–9: "eliminated" (no kings, no pawns) and "royal" (no pawns, one king) helpers; check only applies to a royal side; new `SC_ELIMINATION` outcome.
- Six rule readings need your decision before coding (report section 4).
- Baseline: standard perft numbers correct via a scratch harness, draw tests 6 of 6, Python 31 passed.
