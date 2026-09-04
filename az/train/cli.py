"""Entry point: az-train. Argparse + logging setup, then hands off to run_training."""

from __future__ import annotations

import argparse
import logging
import logging.handlers
import os
import random

import numpy as np
import torch

from az.train.loop import run_training


def _default_selfplay_workers() -> int:
    return max(1, (os.cpu_count() or 2) - 1)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Minimum-viable AlphaZero-style training loop for SuperChess "
            "(standard chess for now). Deliberately downscaled: small network, "
            "few MCTS simulations, small game counts — a working pipeline to "
            "iterate on, not strong play."
        )
    )

    loop = parser.add_argument_group("training loop")
    loop.add_argument("--iterations", type=int, default=50)
    loop.add_argument("--games-per-iteration", type=int, default=32)
    loop.add_argument("--selfplay-workers", type=int, default=_default_selfplay_workers())

    mcts = parser.add_argument_group("MCTS")
    mcts.add_argument("--mcts-simulations", type=int, default=100)
    mcts.add_argument("--c-puct", type=float, default=1.5)
    mcts.add_argument("--dirichlet-alpha", type=float, default=0.3)
    mcts.add_argument("--dirichlet-epsilon", type=float, default=0.25)
    mcts.add_argument("--temperature-moves", type=int, default=15)
    mcts.add_argument("--max-plies", type=int, default=300)

    train = parser.add_argument_group("optimization")
    train.add_argument("--replay-buffer-size", type=int, default=20000)
    train.add_argument("--batch-size", type=int, default=256)
    train.add_argument("--train-steps-per-iteration", type=int, default=200)
    train.add_argument("--lr", type=float, default=1e-3)
    train.add_argument("--weight-decay", type=float, default=1e-4)

    devices = parser.add_argument_group("devices")
    devices.add_argument("--selfplay-device", type=str, default="cpu")
    devices.add_argument(
        "--train-device", type=str, default="cuda" if torch.cuda.is_available() else "cpu"
    )

    net = parser.add_argument_group("network")
    net.add_argument("--nn-blocks", type=int, default=5)
    net.add_argument("--nn-channels", type=int, default=64)

    ckpt = parser.add_argument_group("checkpoints")
    ckpt.add_argument("--checkpoint-dir", type=str, default="checkpoints")
    ckpt.add_argument("--resume", type=str, default=None)
    ckpt.add_argument("--init-checkpoint", type=str, default=None)
    ckpt.add_argument(
        "--eval-games", type=int, default=0,
        help="0 disables checkpoint-gating evaluation (default).",
    )

    misc = parser.add_argument_group("misc")
    misc.add_argument("--log-dir", type=str, default="logs")
    misc.add_argument("--log-level", type=str, default="INFO")
    misc.add_argument("--seed", type=int, default=None)

    return parser


def setup_logging(args) -> None:
    os.makedirs(args.log_dir, exist_ok=True)

    root = logging.getLogger()
    root.setLevel(logging.DEBUG)
    root.handlers.clear()

    console = logging.StreamHandler()
    console.setLevel(getattr(logging, args.log_level.upper()))
    console.setFormatter(logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s"))
    root.addHandler(console)

    file_handler = logging.handlers.RotatingFileHandler(
        os.path.join(args.log_dir, "train.log"), maxBytes=10 * 1024 * 1024, backupCount=5
    )
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s"))
    root.addHandler(file_handler)


def main() -> None:
    args = build_parser().parse_args()
    setup_logging(args)

    if args.seed is not None:
        random.seed(args.seed)
        np.random.seed(args.seed)
        torch.manual_seed(args.seed)

    logging.getLogger(__name__).info("starting az-train with args: %s", vars(args))
    run_training(args)


if __name__ == "__main__":
    main()
