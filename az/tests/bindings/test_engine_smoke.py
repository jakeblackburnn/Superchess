"""End-to-end smoke tests against the real compiled libsuperchess (no mocks)."""

from az.bindings import Color, Engine, MoveResult, Outcome, PieceType, square


def test_start_position_has_twenty_legal_moves():
    with Engine() as eng:
        assert len(eng.legal_moves()) == 20


def test_turn_flips_after_a_move():
    with Engine() as eng:
        result = eng.make_move(square("e2"), square("e4"))
        assert result == MoveResult.OK
        assert eng.turn() == Color.BLACK


def test_board_array_reflects_applied_move():
    with Engine() as eng:
        eng.make_move(square("e2"), square("e4"))
        board = eng.board_array()
        assert board[square("e4")].type == PieceType.PAWN
        assert board[square("e4")].color == Color.WHITE
        assert board[square("e2")].type == PieceType.EMPTY


def test_illegal_move_is_rejected():
    with Engine() as eng:
        # e2 pawn can't jump to e5 on move one.
        assert eng.make_move(square("e2"), square("e5")) == MoveResult.ILLEGAL
        assert eng.turn() == Color.WHITE


def test_promotion_required_then_succeeds():
    with Engine() as eng:
        # White's b-pawn marches up, capturing black's a7 pawn then the b8
        # knight to reach the back rank; black's moves are just filler that
        # don't touch the a7/b8 squares white is aiming for.
        moves = [
            ("b2", "b4"), ("h7", "h5"),
            ("b4", "b5"), ("h5", "h4"),
            ("b5", "b6"), ("h4", "h3"),
            ("b6", "a7"), ("g7", "g6"),
        ]
        for frm, to in moves:
            result = eng.make_move(square(frm), square(to))
            assert result == MoveResult.OK, f"{frm}{to} failed: {result}"

        # White pawn now on a7, one capture from promoting by taking b8's knight.
        assert eng.make_move(square("a7"), square("b8")) == MoveResult.PROMOTION_REQUIRED
        assert eng.make_move(square("a7"), square("b8"), promo="q") == MoveResult.OK

        board = eng.board_array()
        promoted = board[square("b8")]
        assert promoted.type == PieceType.QUEEN
        assert promoted.color == Color.WHITE


def test_checkmate_outcome_fools_mate():
    with Engine() as eng:
        moves = [("f2", "f3"), ("e7", "e5"), ("g2", "g4"), ("d8", "h4")]
        for frm, to in moves:
            assert eng.make_move(square(frm), square(to)) == MoveResult.OK
        assert eng.outcome() == Outcome.CHECKMATE
        assert eng.winner() == Color.BLACK
        assert eng.in_check()


def test_in_check_reports_correctly():
    with Engine() as eng:
        assert not eng.in_check()
        moves = [("b1", "c3"), ("d7", "d5"), ("c3", "b5"), ("b8", "a6")]
        for frm, to in moves:
            assert eng.make_move(square(frm), square(to)) == MoveResult.OK
        assert not eng.in_check()
        # Knight fork with check: Nxc7+ attacks the black king on e8.
        assert eng.make_move(square("b5"), square("c7")) == MoveResult.OK
        assert eng.in_check()
