"""Generates self-play games in parallel across a worker-process pool.

Uses the "spawn" start method (not "fork"): each worker needs its own ctypes
handle to libsuperchess and its own torch state, and fork-inherited state
(open library handles, CUDA contexts) doesn't play well with that.
"""

from __future__ import annotations

import multiprocessing
import time
from functools import partial

import numpy as np

import az.selfplay.worker as worker
from az.nn.inference import NetworkWrapper


def _init_worker(state_dict: dict, device: str, num_blocks: int, channels: int) -> None:
    network = NetworkWrapper.from_state_dict(
        state_dict, device=device, num_blocks=num_blocks, channels=channels
    )
    worker.set_network(network)


def _run_one_game(_ignored_index: int, **play_game_kwargs):
    return worker.play_game(**play_game_kwargs)


def generate_games(
    state_dict: dict,
    num_games: int,
    num_workers: int,
    device: str = "cpu",
    num_blocks: int = 5,
    channels: int = 64,
    **play_game_kwargs,
) -> tuple[list[tuple[np.ndarray, np.ndarray, float]], dict]:
    cpu_state_dict = {k: v.cpu() for k, v in state_dict.items()}

    start = time.monotonic()
    ctx = multiprocessing.get_context("spawn")
    with ctx.Pool(
        num_workers,
        initializer=_init_worker,
        initargs=(cpu_state_dict, device, num_blocks, channels),
    ) as pool:
        results = pool.map(partial(_run_one_game, **play_game_kwargs), range(num_games))
    total_wall_time = time.monotonic() - start

    examples: list[tuple[np.ndarray, np.ndarray, float]] = []
    plies = []
    result_counts = {"white": 0, "black": 0, "draw": 0, "truncated": 0}
    games_wall_time = 0.0

    for game_examples, meta in results:
        examples.extend(game_examples)
        plies.append(meta["plies"])
        result_counts[meta["result"]] += 1
        games_wall_time += meta["wall_time"]

    stats = {
        "games": num_games,
        "avg_plies": float(np.mean(plies)) if plies else 0.0,
        "min_plies": int(np.min(plies)) if plies else 0,
        "max_plies": int(np.max(plies)) if plies else 0,
        "result_counts": result_counts,
        "total_wall_time": total_wall_time,
        "games_per_sec": num_games / total_wall_time if total_wall_time > 0 else 0.0,
        "examples": len(examples),
    }
    return examples, stats
