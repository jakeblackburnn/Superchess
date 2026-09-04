"""Small ResNet policy/value network, sized for a single-GPU MVP self-play loop.

Deliberately tiny relative to real AlphaZero (default ~5 blocks x 64 channels):
VRAM is not the constraint at this scale, self-play throughput through the C
engine's ctypes FFI is. num_blocks/channels are CLI-tunable (see az.train.cli).
"""

from __future__ import annotations

import torch
from torch import nn

from az.encoding import BOARD_DIM, NUM_PLANES, NUM_PROMO_SLOTS, NUM_SQUARES, POLICY_SIZE

# Per from-square move-logit count: (to_square, promotion) combinations. Matches
# az.encoding's index scheme (from_sq * MOVES_PER_SQUARE + to_sq * NUM_PROMO_SLOTS
# + promo_slot) so the policy head can be a small per-square conv instead of a
# giant flatten->Linear (channels*64 -> POLICY_SIZE=24576 would be ~50M params
# on its own — dwarfing the rest of the network and dominating both checkpoint
# size and CPU self-play inference cost).
MOVES_PER_SQUARE = NUM_SQUARES * NUM_PROMO_SLOTS


class ResidualBlock(nn.Module):
    def __init__(self, channels: int):
        super().__init__()
        self.conv1 = nn.Conv2d(channels, channels, 3, padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(channels)
        self.conv2 = nn.Conv2d(channels, channels, 3, padding=1, bias=False)
        self.bn2 = nn.BatchNorm2d(channels)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        residual = x
        out = torch.relu(self.bn1(self.conv1(x)))
        out = self.bn2(self.conv2(out))
        return torch.relu(out + residual)


class SuperChessNet(nn.Module):
    def __init__(self, num_blocks: int = 5, channels: int = 64):
        super().__init__()
        self.stem = nn.Sequential(
            nn.Conv2d(NUM_PLANES, channels, 3, padding=1, bias=False),
            nn.BatchNorm2d(channels),
            nn.ReLU(inplace=True),
        )
        self.blocks = nn.Sequential(*[ResidualBlock(channels) for _ in range(num_blocks)])

        policy_hidden = 32
        self.policy_hidden = nn.Sequential(
            nn.Conv2d(channels, policy_hidden, 1, bias=False),
            nn.BatchNorm2d(policy_hidden),
            nn.ReLU(inplace=True),
        )
        # 1x1 conv, not a flatten->Linear: output channel c at spatial (rank,
        # file) is the logit for from_sq=(rank*8+file), (to_sq, promo) = divmod(c,
        # NUM_PROMO_SLOTS). Keeps the policy head's param count independent of
        # POLICY_SIZE.
        self.policy_out = nn.Conv2d(policy_hidden, MOVES_PER_SQUARE, 1)

        value_channels = 8
        self.value_head = nn.Sequential(
            nn.Conv2d(channels, value_channels, 1, bias=False),
            nn.BatchNorm2d(value_channels),
            nn.ReLU(inplace=True),
            nn.Flatten(),
            nn.Linear(value_channels * BOARD_DIM * BOARD_DIM, 64),
            nn.ReLU(inplace=True),
            nn.Linear(64, 1),
            nn.Tanh(),
        )

    def forward(self, x: torch.Tensor) -> tuple[torch.Tensor, torch.Tensor]:
        features = self.blocks(self.stem(x))

        p = self.policy_out(self.policy_hidden(features))  # (B, MOVES_PER_SQUARE, 8, 8)
        # (rank, file) is the slower-varying axis, channel the faster-varying one,
        # so this flatten's index == rank*8*MOVES_PER_SQUARE + file*MOVES_PER_SQUARE
        # + channel == from_sq * MOVES_PER_SQUARE + (to_sq * NUM_PROMO_SLOTS + promo)
        # == az.encoding.move_to_index's scheme, exactly.
        policy_logits = p.permute(0, 2, 3, 1).reshape(p.shape[0], -1)

        value = self.value_head(features)
        return policy_logits, value
