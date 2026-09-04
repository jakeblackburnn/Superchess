"""Fixed-capacity replay buffer of (board_tensor, policy_target, value_target) examples."""

from __future__ import annotations

import pickle
import random
from collections import deque

import numpy as np


class ReplayBuffer:
    def __init__(self, capacity: int):
        self.capacity = capacity
        self._buffer: deque = deque(maxlen=capacity)

    def __len__(self) -> int:
        return len(self._buffer)

    def add_game(self, examples: list[tuple[np.ndarray, np.ndarray, float]]) -> None:
        self._buffer.extend(examples)

    def sample(self, batch_size: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        if len(self._buffer) == 0:
            raise ValueError("cannot sample from an empty replay buffer")
        n = min(batch_size, len(self._buffer))
        batch = random.sample(self._buffer, n)
        tensors = np.stack([b[0] for b in batch]).astype(np.float32)
        policies = np.stack([b[1] for b in batch]).astype(np.float32)
        values = np.array([b[2] for b in batch], dtype=np.float32)
        return tensors, policies, values

    def save(self, path: str) -> None:
        with open(path, "wb") as f:
            pickle.dump(list(self._buffer), f)

    def load(self, path: str) -> None:
        with open(path, "rb") as f:
            items = pickle.load(f)
        self._buffer = deque(items, maxlen=self.capacity)
