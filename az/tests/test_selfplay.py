"""Self-play smoke tests: real engine, real (tiny, untrained) network, real worker pool."""

from __future__ import annotations

import numpy as np

from az.encoding import POLICY_SIZE
from az.nn.model import SuperChessNet
from az.selfplay import generate_games


def test_generate_games_end_to_end():
    state_dict = SuperChessNet(num_blocks=1, channels=8).state_dict()

    examples, stats = generate_games(
        state_dict,
        num_games=2,
        num_workers=1,
        device="cpu",
        num_blocks=1,
        channels=8,
        mcts_simulations=10,
        c_puct=1.5,
        dirichlet_alpha=0.3,
        dirichlet_epsilon=0.25,
        temperature_moves=5,
        max_plies=40,
    )

    assert stats["games"] == 2
    assert sum(stats["result_counts"].values()) == 2
    assert len(examples) == stats["examples"]
    assert len(examples) > 0

    for tensor, policy, value in examples:
        assert tensor.shape == (14, 8, 8)
        assert policy.shape == (POLICY_SIZE,)
        assert np.isclose(policy.sum(), 1.0, atol=1e-6)
        assert value in (-1.0, 0.0, 1.0)
