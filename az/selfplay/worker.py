"""Plays one self-play game to completion using a process-global network.

The network is set once per worker process (see az.selfplay.driver's pool
initializer) rather than reloaded per game — that's the whole point of using
a persistent worker pool instead of spawning a fresh process per game.
"""

from __future__ import annotations

import time

import numpy as np

from az.bindings import Color, MoveResult, Outcome
from az.bindings.engine import Engine
from az.encoding import board_to_tensor, raw_move_from_index
from az.mcts import MCTS, select_action
from az.nn.inference import NetworkWrapper

# Set by az.selfplay.driver's pool initializer, once per worker process.
_NETWORK: NetworkWrapper | None = None


def set_network(network: NetworkWrapper) -> None:
    global _NETWORK
    _NETWORK = network


def _result_string(outcome: Outcome, winner: Color | None, truncated: bool) -> str:
    if truncated:
        return "truncated"
    if outcome == Outcome.CHECKMATE:
        return "white" if winner == Color.WHITE else "black"
    return "draw"  # stalemate or SC_DRAW (insufficient material / fifty-move / repetition)


def play_game(
    mcts_simulations: int,
    c_puct: float = 1.5,
    dirichlet_alpha: float = 0.3,
    dirichlet_epsilon: float = 0.25,
    temperature_moves: int = 15,
    max_plies: int = 300,
) -> tuple[list[tuple[np.ndarray, np.ndarray, float]], dict]:
    if _NETWORK is None:
        raise RuntimeError("az.selfplay.worker.set_network() was never called in this process")

    start = time.monotonic()
    mcts = MCTS(
        _NETWORK,
        c_puct=c_puct,
        dirichlet_alpha=dirichlet_alpha,
        dirichlet_epsilon=dirichlet_epsilon,
    )

    recorded: list[tuple[np.ndarray, np.ndarray, Color]] = []
    truncated = False

    with Engine() as engine:
        ply = 0
        while engine.outcome() == Outcome.ONGOING:
            if ply >= max_plies:
                truncated = True
                break

            mover = engine.turn()
            policy = mcts.run(engine, mcts_simulations, add_noise=True)
            tensor = board_to_tensor(engine.board_array(), mover, engine.halfmove_clock())
            recorded.append((tensor, policy, mover))

            temperature = 1.0 if ply < temperature_moves else 0.0
            action = select_action(policy, temperature)
            from_sq, to_sq, promo = raw_move_from_index(action, mover)
            result = engine.make_move(from_sq, to_sq, promo)
            if result != MoveResult.OK:
                raise RuntimeError(
                    f"self-play chose an illegal move ({from_sq},{to_sq},{promo}): "
                    f"result={result} — indicates a mask/perspective bug"
                )
            ply += 1

        outcome = engine.outcome()
        winner = engine.winner()

    if truncated or outcome != Outcome.CHECKMATE:
        white_result = 0.0
    else:
        white_result = 1.0 if winner == Color.WHITE else -1.0

    examples = [
        (tensor, policy, white_result if mover == Color.WHITE else -white_result)
        for tensor, policy, mover in recorded
    ]

    meta = {
        "plies": ply,
        "result": _result_string(outcome, winner, truncated),
        "wall_time": time.monotonic() - start,
    }
    return examples, meta
