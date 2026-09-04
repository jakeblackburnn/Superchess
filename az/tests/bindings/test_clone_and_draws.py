"""End-to-end smoke tests against the real compiled libsuperchess (no mocks).

Move sequences mirror engine/tests/draw_test.c's hand-verified checks (same
flat square indices), so both the C-level and FFI-level tests exercise the
identical, already-verified move sequences.
"""

from az.bindings import Color, Engine, MoveResult, Outcome


def _apply(eng: Engine, from_sq: int, to_sq: int) -> None:
    result = eng.make_move(from_sq, to_sq)
    assert result == MoveResult.OK, f"({from_sq},{to_sq}) rejected: {result}"


def test_threefold_repetition_draw():
    with Engine() as eng:
        seq = [
            (1, 18), (57, 42), (18, 1), (42, 57),
            (1, 18), (57, 42), (18, 1), (42, 57),
        ]
        for frm, to in seq[:7]:
            _apply(eng, frm, to)
        assert eng.outcome() == Outcome.ONGOING

        _apply(eng, *seq[7])
        assert eng.outcome() == Outcome.DRAW


def test_fifty_move_rule_draw():
    with Engine() as eng:
        seq = [
            (12, 28), (52, 36), (11, 27), (51, 35), (14, 22), (54, 46), (9, 17), (49, 41),
            (1, 11), (57, 51), (0, 1), (51, 57), (1, 9), (57, 51), (3, 12), (51, 57),
            (4, 3), (57, 51), (3, 4), (51, 57), (5, 14), (57, 51), (4, 3), (51, 57),
            (6, 23), (57, 51), (3, 4), (51, 57), (4, 5), (57, 51), (5, 6), (51, 57),
            (9, 1), (57, 51), (1, 0), (51, 57), (2, 9), (57, 51), (0, 1), (51, 57),
            (1, 2), (57, 51), (2, 3), (51, 57), (3, 4), (57, 51), (4, 5), (51, 57),
            (5, 3), (57, 51), (3, 4), (51, 57), (4, 5), (57, 51), (5, 2), (51, 57),
            (2, 1), (57, 51), (1, 0), (51, 57), (6, 5), (57, 51), (0, 1), (51, 57),
            (1, 2), (57, 51), (2, 3), (51, 57), (3, 4), (57, 51), (4, 2), (51, 57),
            (2, 3), (57, 51), (3, 4), (51, 57), (4, 1), (57, 51), (1, 0), (51, 57),
            (5, 4), (57, 51), (0, 1), (51, 57), (1, 2), (57, 51), (2, 3), (51, 57),
            (3, 1), (57, 51), (1, 2), (51, 57), (2, 3), (57, 51), (3, 0), (51, 57),
            (4, 3), (57, 51), (0, 1), (51, 57), (1, 2), (57, 51), (2, 0), (51, 57),
            (0, 1), (57, 51), (1, 2), (51, 57),
        ]
        assert len(seq) == 108

        for frm, to in seq[:8]:
            _apply(eng, frm, to)
        assert eng.outcome() == Outcome.ONGOING

        for frm, to in seq[8:107]:
            _apply(eng, frm, to)
        assert eng.outcome() == Outcome.ONGOING

        _apply(eng, *seq[107])
        assert eng.outcome() == Outcome.DRAW


def test_clone_is_independent():
    with Engine() as eng:
        _apply(eng, 12, 28)  # e2-e4

        clone = eng.clone()
        try:
            _apply(clone, 52, 36)  # e7-e5, on the clone only
            assert eng.turn() != clone.turn()
            assert eng.board_array()[36].type.name == "EMPTY"
            assert clone.board_array()[36].type.name == "PAWN"
        finally:
            clone.close()


def test_clone_survives_original_close():
    eng = Engine()
    _apply(eng, 12, 28)  # e2-e4
    clone = eng.clone()
    eng.close()

    assert clone.turn() == Color.BLACK
    clone.close()
