"""Pythonic wrapper over the C engine's FFI facade (engine/include/ffi.h)."""

from __future__ import annotations

import ctypes
from enum import IntEnum

from ._native import FfiMove, FfiSquare, SC_MAX_LEGAL_MOVES, lib


class Color(IntEnum):
    WHITE = 0
    BLACK = 1


class PieceType(IntEnum):
    PAWN = 0
    KNIGHT = 1
    BISHOP = 2
    ROOK = 3
    QUEEN = 4
    KING = 5
    EMPTY = 6


class Outcome(IntEnum):
    ONGOING = 0
    CHECKMATE = 1
    STALEMATE = 2
    DRAW = 3  # insufficient mating material


class MoveResult(IntEnum):
    OK = 0
    ILLEGAL = 1
    PROMOTION_REQUIRED = 2
    GAME_OVER = 3


class SquareState:
    __slots__ = ("type", "color", "hasmoved", "passable")

    def __init__(self, ffi_square: FfiSquare):
        self.type = PieceType(ffi_square.type)
        self.color = Color(ffi_square.color)
        self.hasmoved = bool(ffi_square.hasmoved)
        self.passable = bool(ffi_square.passable)

    def __repr__(self):
        if self.type == PieceType.EMPTY:
            return "SquareState(EMPTY)"
        return f"SquareState({self.color.name} {self.type.name})"


def square(name: str) -> int:
    """"e4" -> flat index (rank*8+file), mirroring board.c's str_to_idx."""
    if len(name) != 2:
        raise ValueError(f"invalid square: {name!r}")
    file = ord(name[0]) - ord("a")
    rank = ord(name[1]) - ord("1")
    if not (0 <= file <= 7 and 0 <= rank <= 7):
        raise ValueError(f"invalid square: {name!r}")
    return rank * 8 + file


class Engine:
    """Owns one native Game handle. Use as a context manager or call close() explicitly."""

    def __init__(self):
        self._handle = lib.sc_new_game()
        if not self._handle:
            raise MemoryError("sc_new_game returned NULL")

    def close(self) -> None:
        if getattr(self, "_handle", None):
            lib.sc_free_game(self._handle)
            self._handle = None

    def __enter__(self) -> "Engine":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def __del__(self):
        self.close()

    def reset(self) -> None:
        lib.sc_reset_game(self._handle)

    def turn(self) -> Color:
        return Color(lib.sc_turn(self._handle))

    def outcome(self) -> Outcome:
        return Outcome(lib.sc_outcome(self._handle))

    def winner(self) -> Color | None:
        if self.outcome() != Outcome.CHECKMATE:
            return None
        return Color(lib.sc_winner(self._handle))

    def halfmove_clock(self) -> int:
        return lib.sc_halfmove_clock(self._handle)

    def in_check(self) -> bool:
        return bool(lib.sc_in_check(self._handle))

    def legal_moves(self) -> list[tuple[int, int, str | None]]:
        buf = (FfiMove * SC_MAX_LEGAL_MOVES)()
        count = lib.sc_legal_moves(self._handle, buf, SC_MAX_LEGAL_MOVES)
        if count > SC_MAX_LEGAL_MOVES:
            raise OverflowError(f"{count} legal moves exceeds SC_MAX_LEGAL_MOVES buffer")
        moves = []
        for i in range(count):
            m = buf[i]
            special = m.special.decode() if m.special != b"\x00" else None
            moves.append((m.from_sq, m.to_sq, special))
        return moves

    def make_move(self, from_sq: int, to_sq: int, promo: str | None = None) -> MoveResult:
        promo_char = promo.encode() if promo else b"\x00"
        result = lib.sc_make_move(self._handle, from_sq, to_sq, promo_char)
        return MoveResult(result)

    def board_array(self) -> list[SquareState]:
        buf = (FfiSquare * 64)()
        lib.sc_board_array(self._handle, buf)
        return [SquareState(sq) for sq in buf]

    def clone(self) -> "Engine":
        """Independent copy of this game, for MCTS to explore without mutating the original."""
        cloned = object.__new__(Engine)
        cloned._handle = lib.sc_clone_game(self._handle)
        if not cloned._handle:
            raise MemoryError("sc_clone_game returned NULL")
        return cloned
