"""MCTS smoke tests: real engine, a freshly-initialized (untrained) network."""

from __future__ import annotations

import numpy as np

from az.bindings import Color
from az.bindings.engine import Engine
from az.encoding import POLICY_SIZE, legal_move_mask
from az.mcts import MCTS, select_action
from az.nn.inference import NetworkWrapper
from az.nn.model import SuperChessNet


def _fresh_network() -> NetworkWrapper:
    return NetworkWrapper(SuperChessNet(num_blocks=1, channels=8), device="cpu")


def test_root_policy_is_a_valid_distribution_over_legal_moves_only():
    with Engine() as eng:
        mask, _ = legal_move_mask(eng)
        mcts = MCTS(_fresh_network())
        policy = mcts.run(eng, num_simulations=25)

        assert policy.shape == (POLICY_SIZE,)
        assert np.isclose(policy.sum(), 1.0)
        # No mass on illegal moves.
        assert np.all(policy[~mask] == 0)
        # Every legal move got at least a nonzero prior — some may still end
        # up with zero visits at only 25 sims, so just check total legality.
        assert policy[mask].sum() > 0


def test_run_does_not_mutate_the_original_engine():
    with Engine() as eng:
        turn_before = eng.turn()
        board_before = [(s.type, s.color) for s in eng.board_array()]

        MCTS(_fresh_network()).run(eng, num_simulations=10)

        assert eng.turn() == turn_before
        board_after = [(s.type, s.color) for s in eng.board_array()]
        assert board_after == board_before


def test_finds_mate_in_one():
    # Two moves from Fool's Mate: white already played f3 and g4, so it's
    # black to move with Qh4# available.
    with Engine() as eng:
        moves = [(13, 21), (52, 36), (14, 30)]  # f2-f3, e7-e5, g2-g4
        for frm, to in moves:
            result = eng.make_move(frm, to)
            assert result.name == "OK", f"setup move ({frm},{to}) failed: {result}"

        assert eng.turn() == Color.BLACK
        mask, raw_moves = legal_move_mask(eng)
        # Qd8-h4 is checkmate.
        mate_move = (59, 31, None)
        assert mate_move in raw_moves

        mcts = MCTS(_fresh_network())
        policy = mcts.run(eng, num_simulations=40, add_noise=False)
        best_action = select_action(policy, temperature=0)

        from az.encoding import index_for_raw_move

        assert best_action == index_for_raw_move(*mate_move, eng.turn())
