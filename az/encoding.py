"""Move-index and board-tensor encoding shared by az.mcts and az.nn.

Move encoding: from-square x to-square (+ promotion piece), not AlphaZero's
piece-geometry planes — chosen so a future SuperChess move-gen swap (which changes
which piece can make which shape of move) doesn't invalidate the policy head's
output shape. See alphazero-plan.md.

Board tensor: always from the mover's perspective (rank-flip + swap "mine"/"theirs"
when it's Black to move), so the network only ever has to learn one side.
"""

from __future__ import annotations

import numpy as np

from az.bindings import Color, PieceType
from az.bindings.engine import Engine, SquareState

PROMO_ORDER: tuple[str | None, ...] = (None, "q", "b", "r", "n", "k")
NUM_PROMO_SLOTS = len(PROMO_ORDER)  # 6
NUM_SQUARES = 64
POLICY_SIZE = NUM_SQUARES * NUM_SQUARES * NUM_PROMO_SLOTS  # 24576

NUM_PLANES = 14
BOARD_DIM = 8


def move_to_index(from_sq: int, to_sq: int, promo: str | None = None) -> int:
    if not (0 <= from_sq < NUM_SQUARES and 0 <= to_sq < NUM_SQUARES):
        raise ValueError(f"square out of range: from={from_sq} to={to_sq}")
    promo_slot = PROMO_ORDER.index(promo)
    return (from_sq * NUM_SQUARES + to_sq) * NUM_PROMO_SLOTS + promo_slot


def index_to_move(index: int) -> tuple[int, int, str | None]:
    if not (0 <= index < POLICY_SIZE):
        raise ValueError(f"policy index out of range: {index}")
    promo_slot = index % NUM_PROMO_SLOTS
    rest = index // NUM_PROMO_SLOTS
    to_sq = rest % NUM_SQUARES
    from_sq = rest // NUM_SQUARES
    return from_sq, to_sq, PROMO_ORDER[promo_slot]


def _persp_square(sq: int, turn: Color) -> int:
    """Rank-mirror a square when Black is to move. Self-inverse (flip(flip(s)) == s)."""
    if turn == Color.BLACK:
        rank, file = divmod(sq, 8)
        return (7 - rank) * 8 + file
    return sq


def index_for_raw_move(from_sq: int, to_sq: int, promo: str | None, turn: Color) -> int:
    """Perspective-transform a raw engine move, then encode it as a policy index.

    A raw move's `special` is 'c' for castling (see engine/include/ffi.h) — not
    a promotion choice, and never one make_move needs passed back explicitly
    (castling is auto-detected from the two-file king move). (from, to) alone
    already disambiguates castling from every other legal move, so it's encoded
    in the same slot as "no promotion" rather than needing a 7th promo slot.
    """
    if promo == "c":
        promo = None
    return move_to_index(_persp_square(from_sq, turn), _persp_square(to_sq, turn), promo)


def raw_move_from_index(index: int, turn: Color) -> tuple[int, int, str | None]:
    """Inverse of index_for_raw_move: policy index -> a real (from, to, promo) for make_move."""
    from_sq, to_sq, promo = index_to_move(index)
    return _persp_square(from_sq, turn), _persp_square(to_sq, turn), promo


def board_to_tensor(
    board_array: list[SquareState], turn: Color, halfmove_clock: int = 0
) -> np.ndarray:
    """(14, 8, 8) float32 tensor from the mover's perspective.

    Planes 0-5: my {pawn,knight,bishop,rook,queen,king}. Planes 6-11: opponent's
    same. Plane 12: en-passant target squares. Plane 13: no-progress count
    (halfmove_clock / 100), constant-filled.
    """
    tensor = np.zeros((NUM_PLANES, BOARD_DIM, BOARD_DIM), dtype=np.float32)
    for sq, piece in enumerate(board_array):
        ps = _persp_square(sq, turn)
        rank, file = divmod(ps, 8)
        if piece.type != PieceType.EMPTY:
            plane = int(piece.type) + (0 if piece.color == turn else 6)
            tensor[plane, rank, file] = 1.0
        if piece.passable:
            tensor[12, rank, file] = 1.0
    tensor[13].fill(halfmove_clock / 100.0)
    return tensor


def legal_move_mask(engine: Engine) -> tuple[np.ndarray, list[tuple[int, int, str | None]]]:
    """Legal-move mask over POLICY_SIZE (perspective-transformed) plus the raw
    (engine-coordinate) legal moves list, in the order sc_legal_moves returned them.
    """
    turn = engine.turn()
    raw_moves = engine.legal_moves()
    mask = np.zeros(POLICY_SIZE, dtype=bool)
    for from_sq, to_sq, promo in raw_moves:
        mask[index_for_raw_move(from_sq, to_sq, promo, turn)] = True
    return mask, raw_moves


def masked_softmax(logits: np.ndarray, mask: np.ndarray) -> np.ndarray:
    """Softmax over `logits` restricted to `mask` (bool array, same shape); zero elsewhere.

    Returns float32 — this ends up stored per-example in the replay buffer, and
    POLICY_SIZE=24576 floats/example adds up fast (float64 would double it for
    no accuracy benefit at this scale).
    """
    masked_logits = np.where(mask, logits, -np.inf)
    m = np.max(masked_logits)
    if not np.isfinite(m):
        # No legal moves passed in the mask — fall back to uniform over the mask.
        count = max(int(mask.sum()), 1)
        return (mask.astype(np.float32)) / count
    exp = np.where(mask, np.exp(masked_logits - m), 0.0)
    total = exp.sum()
    if total <= 0:
        count = max(int(mask.sum()), 1)
        return (mask.astype(np.float32)) / count
    return (exp / total).astype(np.float32)
