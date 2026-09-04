"""ReplayBuffer add/sample/save/load round-trip and capacity eviction."""

from __future__ import annotations

import numpy as np

from az.train.replay_buffer import ReplayBuffer


def _fake_examples(n: int, offset: int = 0):
    return [
        (np.full((14, 8, 8), i, dtype=np.float32), np.full(24576, 1.0 / 24576, dtype=np.float32), float(i % 2))
        for i in range(offset, offset + n)
    ]


def test_add_and_sample():
    buf = ReplayBuffer(capacity=100)
    buf.add_game(_fake_examples(10))
    assert len(buf) == 10

    tensors, policies, values = buf.sample(4)
    assert tensors.shape == (4, 14, 8, 8)
    assert policies.shape == (4, 24576)
    assert values.shape == (4,)


def test_sample_more_than_available_returns_all():
    buf = ReplayBuffer(capacity=100)
    buf.add_game(_fake_examples(3))
    tensors, _, _ = buf.sample(10)
    assert tensors.shape[0] == 3


def test_capacity_eviction():
    buf = ReplayBuffer(capacity=5)
    buf.add_game(_fake_examples(10))
    assert len(buf) == 5


def test_save_and_load_round_trip(tmp_path):
    buf = ReplayBuffer(capacity=100)
    buf.add_game(_fake_examples(7))
    path = tmp_path / "buffer.pkl"
    buf.save(str(path))

    loaded = ReplayBuffer(capacity=100)
    loaded.load(str(path))
    assert len(loaded) == 7

    tensors, _, _ = loaded.sample(7)
    assert tensors.shape == (7, 14, 8, 8)
