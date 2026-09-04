"""The single synchronous self-play -> train -> checkpoint loop."""

from __future__ import annotations

import logging
import os
import time

import numpy as np
import torch

from az.bindings import Color, Outcome
from az.bindings.engine import Engine
from az.encoding import raw_move_from_index
from az.mcts import MCTS, select_action
from az.nn.inference import NetworkWrapper
from az.nn.model import SuperChessNet
from az.selfplay import generate_games
from az.train.checkpoint import load_checkpoint, save_checkpoint
from az.train.loss import compute_loss
from az.train.replay_buffer import ReplayBuffer

logger = logging.getLogger(__name__)


def run_training(args) -> None:
    os.makedirs(args.checkpoint_dir, exist_ok=True)

    model = SuperChessNet(num_blocks=args.nn_blocks, channels=args.nn_channels)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr, weight_decay=args.weight_decay)
    replay_buffer = ReplayBuffer(args.replay_buffer_size)

    start_iteration = 0

    if args.init_checkpoint:
        source_iteration, _extra = load_checkpoint(args.init_checkpoint, model)
        logger.info(
            "loaded initial weights from %s (source iteration %d)",
            args.init_checkpoint, source_iteration,
        )

    if args.resume:
        resumed_iteration, _extra = load_checkpoint(args.resume, model, optimizer)
        start_iteration = resumed_iteration + 1
        buffer_path = args.resume + ".buffer.pkl"
        if os.path.exists(buffer_path):
            replay_buffer.load(buffer_path)
            logger.info(
                "loaded replay buffer (%d examples) from %s", len(replay_buffer), buffer_path
            )
        logger.info("resuming from %s at iteration %d", args.resume, start_iteration)

    for iteration in range(start_iteration, args.iterations):
        iter_start = time.monotonic()

        state_dict = model.state_dict()
        examples, selfplay_stats = generate_games(
            state_dict,
            num_games=args.games_per_iteration,
            num_workers=args.selfplay_workers,
            device=args.selfplay_device,
            num_blocks=args.nn_blocks,
            channels=args.nn_channels,
            mcts_simulations=args.mcts_simulations,
            c_puct=args.c_puct,
            dirichlet_alpha=args.dirichlet_alpha,
            dirichlet_epsilon=args.dirichlet_epsilon,
            temperature_moves=args.temperature_moves,
            max_plies=args.max_plies,
        )
        replay_buffer.add_game(examples)

        logger.info(
            "iter %d self-play: %d games in %.1fs (%.1f games/s), avg %.1f plies "
            "(min=%d max=%d), results=%s, buffer=%d/%d",
            iteration,
            selfplay_stats["games"],
            selfplay_stats["total_wall_time"],
            selfplay_stats["games_per_sec"],
            selfplay_stats["avg_plies"],
            selfplay_stats["min_plies"],
            selfplay_stats["max_plies"],
            selfplay_stats["result_counts"],
            len(replay_buffer),
            args.replay_buffer_size,
        )

        device = torch.device(args.train_device)
        model.to(device)
        model.train()

        policy_losses = []
        value_losses = []
        for _step in range(args.train_steps_per_iteration):
            tensors, policies, values = replay_buffer.sample(args.batch_size)
            tensors_t = torch.from_numpy(tensors).to(device)
            policies_t = torch.from_numpy(policies).to(device)
            values_t = torch.from_numpy(values).to(device)

            optimizer.zero_grad()
            policy_logits, value_pred = model(tensors_t)
            total_loss, policy_loss, value_loss = compute_loss(
                policy_logits, policies_t, value_pred, values_t
            )
            total_loss.backward()
            optimizer.step()

            policy_losses.append(policy_loss.item())
            value_losses.append(value_loss.item())

        model.eval()

        mean_policy_loss = float(np.mean(policy_losses)) if policy_losses else 0.0
        mean_value_loss = float(np.mean(value_losses)) if value_losses else 0.0
        lr = optimizer.param_groups[0]["lr"]
        logger.info(
            "iter %d train: policy_loss=%.4f value_loss=%.4f total=%.4f lr=%g",
            iteration, mean_policy_loss, mean_value_loss,
            mean_policy_loss + mean_value_loss, lr,
        )

        ckpt_path = os.path.join(args.checkpoint_dir, f"iter_{iteration:05d}.pt")
        latest_path = os.path.join(args.checkpoint_dir, "latest.pt")
        save_checkpoint(ckpt_path, model, optimizer, iteration)
        save_checkpoint(latest_path, model, optimizer, iteration)
        replay_buffer.save(latest_path + ".buffer.pkl")
        logger.info(
            "iter %d: saved checkpoint %s (iteration took %.1fs)",
            iteration, ckpt_path, time.monotonic() - iter_start,
        )

        if args.eval_games > 0:
            _maybe_promote(model, args)

    logger.info("training complete: %d iterations", args.iterations)


def _play_eval_game(
    challenger: NetworkWrapper,
    incumbent: NetworkWrapper,
    challenger_is_white: bool,
    mcts_simulations: int,
    c_puct: float,
    max_plies: int,
) -> float:
    """Returns 1.0 if the challenger wins, 0.5 for a draw, 0.0 for a loss."""
    challenger_mcts = MCTS(challenger, c_puct=c_puct)
    incumbent_mcts = MCTS(incumbent, c_puct=c_puct)

    with Engine() as engine:
        ply = 0
        while engine.outcome() == Outcome.ONGOING and ply < max_plies:
            mover = engine.turn()
            is_challenger_turn = (mover == Color.WHITE) == challenger_is_white
            mcts = challenger_mcts if is_challenger_turn else incumbent_mcts
            policy = mcts.run(engine, mcts_simulations, add_noise=False)
            action = select_action(policy, temperature=0.0)
            from_sq, to_sq, promo = raw_move_from_index(action, mover)
            engine.make_move(from_sq, to_sq, promo)
            ply += 1

        outcome = engine.outcome()
        winner = engine.winner()

    if outcome != Outcome.CHECKMATE:
        return 0.5
    challenger_won = (winner == Color.WHITE) == challenger_is_white
    return 1.0 if challenger_won else 0.0


def _maybe_promote(model: SuperChessNet, args) -> None:
    """Optional, off by default (--eval-games 0). Plays a small gauntlet between
    the freshly-trained model and checkpoints/best.pt, promoting only on a
    clear win rate. Kept deliberately simple: CPU-only, sequential games."""
    best_path = os.path.join(args.checkpoint_dir, "best.pt")

    if not os.path.exists(best_path):
        save_checkpoint(best_path, model, optimizer=None, iteration=-1)
        logger.info("eval: no best.pt yet, promoting current model unconditionally")
        return

    incumbent_model = SuperChessNet(num_blocks=args.nn_blocks, channels=args.nn_channels)
    load_checkpoint(best_path, incumbent_model)
    incumbent = NetworkWrapper(incumbent_model, device="cpu")
    challenger = NetworkWrapper(model, device="cpu")

    wins = 0.0
    for i in range(args.eval_games):
        challenger_is_white = i % 2 == 0
        wins += _play_eval_game(
            challenger, incumbent, challenger_is_white,
            args.mcts_simulations, args.c_puct, args.max_plies,
        )

    win_rate = wins / args.eval_games
    logger.info("eval: challenger win rate %.2f over %d games", win_rate, args.eval_games)
    if win_rate > 0.55:
        save_checkpoint(best_path, model, optimizer=None, iteration=-1)
        logger.info("eval: challenger promoted to best.pt")
    else:
        logger.info("eval: challenger not promoted (best.pt unchanged)")
