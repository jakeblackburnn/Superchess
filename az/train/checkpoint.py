"""Model/optimizer checkpoint save/load."""

from __future__ import annotations

import torch


def save_checkpoint(path: str, model, optimizer, iteration: int, extra: dict | None = None) -> None:
    torch.save(
        {
            "model_state_dict": model.state_dict(),
            "optimizer_state_dict": optimizer.state_dict() if optimizer is not None else None,
            "iteration": iteration,
            "extra": extra or {},
        },
        path,
    )


def load_checkpoint(
    path: str, model, optimizer=None, map_location: str = "cpu"
) -> tuple[int, dict]:
    checkpoint = torch.load(path, map_location=map_location, weights_only=False)
    model.load_state_dict(checkpoint["model_state_dict"])
    if optimizer is not None and checkpoint.get("optimizer_state_dict") is not None:
        optimizer.load_state_dict(checkpoint["optimizer_state_dict"])
    return checkpoint.get("iteration", 0), checkpoint.get("extra", {})
