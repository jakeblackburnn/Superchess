# Project notes (durable facts)

Verified 2026-10-02 at `9d9ee95`.

## Commands
- Engine: `cd engine && make` (CLI binary `engine/superchess`), `make lib` (`engine/build/libsuperchess.so`, needed by `az/bindings`).
- Python tests: from the repo root, `PYTHONPATH=. ~/Code/python_venv_01_main/bin/python -m pytest -q az/tests` → 31 passed.
- C tests: `cd engine/tests && make test` / `make test-draw`. Both fail today, see Traps.

## Traps
- `engine/tests/perft_test` and `engine/tests/draw_test` are tracked macOS arm64 binaries. `make` thinks they are up to date and running them gives `cannot execute binary file`.
- `engine/tests/perft_test.c` does not compile: it calls `perft(depth)` and uses globals `b`, `temp`, `turn`, `create_board()` that no longer exist. Current signature is `perft(Board *, Color, int)` (`engine/include/superchess.h:17`).
- `engine/tests/draw_test.c` compiles and passes 6 of 6 when rebuilt by hand.
- The project `.venv` has no `pytest`; use the fallback venv.
- Run pytest from the repo root with `PYTHONPATH=.`; from inside `az/` the `az` package is not importable.

## Pointers
- `dev/agents/project/superchess-ruleset.md`: engine map and the plan for adding the superchess ruleset.
