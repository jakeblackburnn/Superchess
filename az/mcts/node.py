"""A single node in the MCTS tree, keyed by policy-index action."""

from __future__ import annotations


class Node:
    __slots__ = ("prior", "visit_count", "value_sum", "children")

    def __init__(self, prior: float):
        self.prior = prior
        self.visit_count = 0
        self.value_sum = 0.0
        self.children: dict[int, "Node"] = {}

    @property
    def expanded(self) -> bool:
        return bool(self.children)

    @property
    def value(self) -> float:
        """Mean backed-up value, from this node's own side-to-move's perspective."""
        if self.visit_count == 0:
            return 0.0
        return self.value_sum / self.visit_count
