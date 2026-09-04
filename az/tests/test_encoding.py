"""Move-index and board-tensor encoding tests, driven against the real engine
(no mocks) via az.bindings.Engine."""

import numpy as np

from az.bindings import Color, square
from az.bindings.engine import Engine
from az.encoding import (
    NUM_PLANES,
    POLICY_SIZE,
    board_to_tensor,
    index_for_raw_move,
    index_to_move,
    legal_move_mask,
    masked_softmax,
    move_to_index,
    raw_move_from_index,
)


def test_move_to_index_round_trips_over_all_promo_slots():
    for from_sq in (0, 12, 63):
        for to_sq in (5, 40, 63):
            for promo in (None, "q", "b", "r", "n", "k"):
                idx = move_to_index(from_sq, to_sq, promo)
                assert 0 <= idx < POLICY_SIZE
                assert index_to_move(idx) == (from_sq, to_sq, promo)


def test_index_for_raw_move_round_trips_start_position():
    with Engine() as eng:
        turn = eng.turn()
        for from_sq, to_sq, promo in eng.legal_moves():
            idx = index_for_raw_move(from_sq, to_sq, promo, turn)
            assert raw_move_from_index(idx, turn) == (from_sq, to_sq, promo)


def test_index_for_raw_move_round_trips_black_to_move():
    with Engine() as eng:
        assert eng.make_move(square("e2"), square("e4")) is not None
        assert eng.turn() == Color.BLACK
        for from_sq, to_sq, promo in eng.legal_moves():
            idx = index_for_raw_move(from_sq, to_sq, promo, Color.BLACK)
            assert raw_move_from_index(idx, Color.BLACK) == (from_sq, to_sq, promo)


def test_round_trip_through_a_promotion_position():
    # Same march used in the bindings smoke test: white ends one capture away
    # from promoting on b8, with several legal promotion choices.
    with Engine() as eng:
        moves = [
            ("b2", "b4"), ("h7", "h5"),
            ("b4", "b5"), ("h5", "h4"),
            ("b5", "b6"), ("h4", "h3"),
            ("b6", "a7"), ("g7", "g6"),
        ]
        for frm, to in moves:
            eng.make_move(square(frm), square(to))

        turn = eng.turn()
        legal = eng.legal_moves()
        promo_moves = [m for m in legal if m[2] is not None]
        assert promo_moves, "expected promotion choices to be legal here"

        for from_sq, to_sq, promo in legal:
            idx = index_for_raw_move(from_sq, to_sq, promo, turn)
            assert raw_move_from_index(idx, turn) == (from_sq, to_sq, promo)

        # Distinct promotion choices for the same from/to must get distinct indices.
        indices = {index_for_raw_move(f, t, p, turn) for f, t, p in promo_moves}
        assert len(indices) == len(promo_moves)


def test_castling_move_encodes_and_round_trips():
    # Clear White's kingside (knight and bishop out) so O-O is legal, without
    # ever moving the king or the h1 rook.
    with Engine() as eng:
        moves = [
            ("g1", "f3"), ("g8", "f6"),
            ("g2", "g3"), ("g7", "g6"),
            ("f1", "g2"), ("f8", "g7"),
        ]
        for frm, to in moves:
            eng.make_move(square(frm), square(to))

        turn = eng.turn()
        legal = eng.legal_moves()
        castle_moves = [m for m in legal if m[2] == "c"]
        assert castle_moves, "expected White kingside castling to be legal here"

        # Every legal move here (including castling's special='c') must encode
        # without raising — this is a regression check: move_to_index only
        # recognizes promotion chars, and 'c' used to blow it up.
        for from_sq, to_sq, promo in legal:
            idx = index_for_raw_move(from_sq, to_sq, promo, turn)
            assert 0 <= idx < POLICY_SIZE

        from_sq, to_sq, promo = castle_moves[0]
        idx = index_for_raw_move(from_sq, to_sq, promo, turn)
        decoded = raw_move_from_index(idx, turn)
        # 'c' normalizes to "no promotion" — (from, to) alone already
        # disambiguates castling from every other legal move, and make_move
        # auto-detects castling regardless of what's passed as promo.
        assert decoded == (from_sq, to_sq, None)

        result = eng.make_move(*decoded)
        assert result.name == "OK"
        board = eng.board_array()
        assert board[square("g1")].type.name == "KING"
        assert board[square("f1")].type.name == "ROOK"


def test_perspective_flip_is_symmetric_start_vs_after_one_move():
    # White's e2-e4 seen from White's perspective should land on the same
    # relative squares as Black's mirror-image reply from Black's perspective.
    with Engine() as eng:
        white_idx = index_for_raw_move(square("e2"), square("e4"), None, Color.WHITE)
        eng.make_move(square("e2"), square("e4"))
        # Black's symmetric reply, e7-e5, viewed from Black's own perspective,
        # must encode to the exact same policy index as White's e2-e4 did.
        black_idx = index_for_raw_move(square("e7"), square("e5"), None, Color.BLACK)
        assert white_idx == black_idx


def test_board_to_tensor_shape_and_perspective():
    with Engine() as eng:
        tensor = board_to_tensor(eng.board_array(), eng.turn(), halfmove_clock=0)
        assert tensor.shape == (NUM_PLANES, 8, 8)
        # White to move: white pawns are "mine" (planes 0-5), occupying rank
        # index 1 (rank 2) in perspective space, unflipped for White.
        assert tensor[0, 1, :].sum() == 8  # all 8 pawns on their home rank
        assert tensor[0 + 6, 6, :].sum() == 8  # opponent pawns, plane 6 offset

        eng.make_move(square("e2"), square("e4"))
        tensor_black = board_to_tensor(eng.board_array(), eng.turn(), halfmove_clock=0)
        # Black to move now: from Black's perspective, Black's own pawns are
        # "mine" and sit on perspective rank 1, mirroring the White case above.
        assert tensor_black[0, 1, :].sum() == 8
        assert tensor_black[6, 6, :].sum() == 7  # one white pawn has moved off rank 2


def test_legal_move_mask_matches_legal_move_count():
    with Engine() as eng:
        mask, raw_moves = legal_move_mask(eng)
        assert mask.dtype == bool
        assert mask.shape == (POLICY_SIZE,)
        assert int(mask.sum()) == len(raw_moves) == 20


def test_masked_softmax_sums_to_one_over_mask():
    rng = np.random.default_rng(0)
    logits = rng.normal(size=POLICY_SIZE)
    mask = np.zeros(POLICY_SIZE, dtype=bool)
    mask[[3, 100, 5000]] = True
    probs = masked_softmax(logits, mask)
    assert np.isclose(probs.sum(), 1.0)
    assert np.all(probs[~mask] == 0.0)
