"""Combined policy/value loss: soft-label cross-entropy + MSE."""

from __future__ import annotations

import torch
import torch.nn.functional as F


def compute_loss(
    policy_logits: torch.Tensor,
    policy_targets: torch.Tensor,
    value_pred: torch.Tensor,
    value_targets: torch.Tensor,
) -> tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
    log_probs = F.log_softmax(policy_logits, dim=1)
    policy_loss = -(policy_targets * log_probs).sum(dim=1).mean()

    value_pred = value_pred.squeeze(-1)
    value_loss = F.mse_loss(value_pred, value_targets)

    total = policy_loss + value_loss
    return total, policy_loss, value_loss
