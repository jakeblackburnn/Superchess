"""PUCT search over az.bindings.Engine, guided by an az.nn NetworkWrapper.

Each simulation clones the engine once (Engine.clone(), a cheap POD copy — see
az/bindings/engine.py) and mutates that single clone while descending the tree,
so the real game state is never touched. Values are always stored on a Node
from that node's own side-to-move's perspective; backing up negates at every
ply (two-player negamax), matching how az.encoding.board_to_tensor always
presents the board from the mover's perspective.
"""

from __future__ import annotations

import math

import numpy as np

from az.bindings import MoveResult, Outcome
from az.bindings.engine import Engine
from az.encoding import POLICY_SIZE, board_to_tensor, legal_move_mask, masked_softmax, raw_move_from_index
from az.nn.inference import NetworkWrapper

from .node import Node


def _terminal_value(outcome: Outcome) -> float:
    """Value from the perspective of the side to move at a terminal position.

    Checkmate means the side to move has no legal moves and is in check —
    i.e. they just lost — so this is always -1 for checkmate. Stalemate and
    draws are 0.
    """
    if outcome == Outcome.CHECKMATE:
        return -1.0
    return 0.0


class MCTS:
    def __init__(
        self,
        network: NetworkWrapper,
        c_puct: float = 1.5,
        dirichlet_alpha: float = 0.3,
        dirichlet_epsilon: float = 0.25,
    ):
        self.network = network
        self.c_puct = c_puct
        self.dirichlet_alpha = dirichlet_alpha
        self.dirichlet_epsilon = dirichlet_epsilon

    def run(self, engine: Engine, num_simulations: int, add_noise: bool = True) -> np.ndarray:
        """Returns a (POLICY_SIZE,) visit-count distribution, normalized, over
        the root's legal moves. All mass is on legal moves; everything else is 0.
        """
        root = Node(prior=1.0)
        self._expand(root, engine)
        if add_noise:
            self._add_dirichlet_noise(root)

        for _ in range(num_simulations):
            self._simulate(root, engine)

        # float32: this ends up stored per-example in the replay buffer, and
        # POLICY_SIZE=24576 floats/example adds up fast at float64.
        visit_counts = np.zeros(POLICY_SIZE, dtype=np.float32)
        for action, child in root.children.items():
            visit_counts[action] = child.visit_count
        total = visit_counts.sum()
        if total > 0:
            return visit_counts / total

        # num_simulations == 0 (or every child somehow unvisited): fall back
        # to the raw priors so callers always get a usable distribution.
        priors = np.zeros(POLICY_SIZE, dtype=np.float32)
        for action, child in root.children.items():
            priors[action] = child.prior
        priors_total = priors.sum()
        return priors / priors_total if priors_total > 0 else priors

    def _simulate(self, root: Node, engine: Engine) -> None:
        sim_engine = engine.clone()
        try:
            node = root
            path = [node]

            while node.expanded:
                action, node = self._select_child(node)
                from_sq, to_sq, promo = raw_move_from_index(action, sim_engine.turn())
                result = sim_engine.make_move(from_sq, to_sq, promo)
                if result != MoveResult.OK:
                    raise RuntimeError(
                        f"MCTS selected an illegal move ({from_sq},{to_sq},{promo}): "
                        f"result={result} — indicates a mask/perspective bug"
                    )
                path.append(node)

            outcome = sim_engine.outcome()
            if outcome != Outcome.ONGOING:
                value = _terminal_value(outcome)
            else:
                value = self._expand(node, sim_engine)

            self._backup(path, value)
        finally:
            sim_engine.close()

    def _select_child(self, node: Node) -> tuple[int, Node]:
        sqrt_parent = math.sqrt(max(node.visit_count, 1))
        best_score = -math.inf
        best_action = None
        best_child = None
        for action, child in node.children.items():
            q = -child.value  # child.value is from the opponent's perspective
            u = self.c_puct * child.prior * sqrt_parent / (1 + child.visit_count)
            score = q + u
            if score > best_score:
                best_score = score
                best_action = action
                best_child = child
        return best_action, best_child

    def _expand(self, node: Node, engine: Engine) -> float:
        """Evaluates `engine`'s current (non-terminal) position, populates
        `node`'s children with network priors over legal moves, and returns
        the value from the side-to-move's perspective."""
        mask, _raw_moves = legal_move_mask(engine)
        tensor = board_to_tensor(engine.board_array(), engine.turn(), engine.halfmove_clock())
        policy_logits, values = self.network.predict(tensor[None, ...])
        priors = masked_softmax(policy_logits[0], mask)

        for action in np.flatnonzero(mask):
            node.children[int(action)] = Node(prior=float(priors[action]))

        return float(values[0])

    def _add_dirichlet_noise(self, root: Node) -> None:
        if not root.children:
            return
        actions = list(root.children.keys())
        noise = np.random.dirichlet([self.dirichlet_alpha] * len(actions))
        eps = self.dirichlet_epsilon
        for action, n in zip(actions, noise):
            child = root.children[action]
            child.prior = (1 - eps) * child.prior + eps * n

    @staticmethod
    def _backup(path: list[Node], value: float) -> None:
        """`value` is from the perspective of the side to move at the leaf.
        Each node up the path represents the opponent's turn relative to the
        one below it, so the sign flips at every step."""
        for node in reversed(path):
            node.visit_count += 1
            node.value_sum += value
            value = -value


def select_action(visit_policy: np.ndarray, temperature: float) -> int:
    """Sample a policy index from a visit-count distribution. temperature=0
    is greedy (argmax); otherwise samples from policy**(1/temperature),
    renormalized."""
    nonzero = np.flatnonzero(visit_policy)
    if len(nonzero) == 0:
        raise ValueError("visit_policy has no nonzero entries")

    if temperature <= 0:
        return int(nonzero[np.argmax(visit_policy[nonzero])])

    weights = visit_policy[nonzero] ** (1.0 / temperature)
    weights = weights / weights.sum()
    return int(np.random.choice(nonzero, p=weights))
