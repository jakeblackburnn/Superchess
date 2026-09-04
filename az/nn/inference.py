"""Thin device-placement + batched-inference wrapper over SuperChessNet."""

from __future__ import annotations

import numpy as np
import torch

from az.nn.model import SuperChessNet


class NetworkWrapper:
    """Owns one model instance on one device; predict() is the only entry point
    MCTS/self-play need."""

    def __init__(self, model: SuperChessNet, device: str = "cpu"):
        self.device = torch.device(device)
        self.model = model.to(self.device)
        self.model.eval()

    @classmethod
    def from_state_dict(
        cls, state_dict: dict, device: str = "cpu", num_blocks: int = 5, channels: int = 64
    ) -> "NetworkWrapper":
        model = SuperChessNet(num_blocks=num_blocks, channels=channels)
        model.load_state_dict(state_dict)
        return cls(model, device=device)

    @torch.no_grad()
    def predict(self, tensor_batch: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        """tensor_batch: (N, 14, 8, 8) float32 -> (policy_logits (N, POLICY_SIZE),
        values (N,)), both numpy, on CPU."""
        x = torch.from_numpy(tensor_batch).to(self.device, dtype=torch.float32)
        policy_logits, value = self.model(x)
        return (
            policy_logits.detach().cpu().numpy(),
            value.detach().cpu().numpy().reshape(-1),
        )
