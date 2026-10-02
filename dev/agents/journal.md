# Journal · 2026-10-02 12:19 · main · 4732b07
## Start
**State:** no `dev/` yet, no project `CLAUDE.md`. C engine in `engine/`, AlphaZero Python in `az/` (bindings, nn, mcts, selfplay, train). `checkpoints/` and `logs/` untracked.
**Next:** this task.
**Traps:** none known.
**Pointers:** `README.md`, `superchess.md` (rules, roadmap). Sibling layout to copy: `~/Code/main/koth/dev/`.

## Task: Init dev/ folder structure · 12:19
**Goal:** give superchess the same `dev/` layout as koth/ctensor so future sessions have ground-truth docs and an agent journal.
**Now:**
- No `dev/`. Project intent lives in `README.md` and `superchess.md`.
- koth layout: `dev/main.md`, `dev/journal.md` (owner), `dev/agents/journal.md`, `dev/agents/project/notes.md`.
**Scope:** in: create `dev/agents/journal.md` and `dev/agents/project/notes.md` (build/test/train commands, layout of `engine/` and `az/`, traps). Out: owner files at `dev/` root, `CLAUDE.md`, `.gitignore`, any code.
**Constraints:** assumption: owner files are yours to write, so I don't create even empty stubs. No commit (`/devlog` does that).
**Approach:**
- Journal written (this file).
- Read `engine/Makefile`, `az/tests`, `az/train` for real commands, then write `notes.md` with only verified facts.
- Leave other `dev/agents/` files to be created when first needed.
**Risks:** notes drift from the code; keep it to commands I ran.
**Done when:** `dev/agents/journal.md` and `dev/agents/project/notes.md` exist; every command in notes.md was run, with results stated; `git status` shows only `dev/` as new besides the existing untracked dirs.
**Open:**
1. Should I also seed `dev/main.md` from `README.md`/`superchess.md`? Recommend no, you write it.
2. Add `checkpoints/` and `logs/` to `.gitignore`? Out of scope unless you say so.

## Close · 2026-10-02 12:21 · 4732b07..50d84ff
- **changed:** `superchess.md` → `dev/main.md` (git mv, your request); `dev/agents/journal.md` created
- **why:** start the `dev/` structure with your rules/roadmap doc as ground truth
- **verified:** `git status --short` → only `checkpoints/`, `logs/`, `dev/agents/` untracked; rename committed. Nothing else run.
- **by:** claude
- `50d84ff` notes: move superchess.md to dev/main.md
- 1 file, +0 −0
- `dev/agents/project/notes.md` was framed but not written; no `dev/journal.md` exists, so no Results appended.

### Next
1. Write `dev/agents/project/notes.md`: read `engine/Makefile`, `az/tests`, `az/train`, run the build and tests, record only verified commands.
2. Decide: `.gitignore` for `checkpoints/` and `logs/`; project `CLAUDE.md`.
### Pointers
`dev/main.md` (rules, roadmap; README.md still describes the project), sibling layout `~/Code/main/koth/dev/`.
