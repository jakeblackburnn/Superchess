"""ctypes loading and struct/signature wiring for libsuperchess, mirroring engine/include/ffi.h."""

import ctypes
import platform
from pathlib import Path

_LIB_NAME = "libsuperchess.dylib" if platform.system() == "Darwin" else "libsuperchess.so"
# az/bindings/_native.py -> az/bindings -> az -> repo root
_LIB_PATH = Path(__file__).resolve().parents[2] / "engine" / "build" / _LIB_NAME

if not _LIB_PATH.exists():
    raise OSError(f"{_LIB_PATH} not found — build it first: cd engine && make lib")

lib = ctypes.CDLL(str(_LIB_PATH))


class GameHandle(ctypes.Structure):
    """Opaque; never instantiated directly, only pointed to."""
    pass


class FfiMove(ctypes.Structure):
    _fields_ = [
        ("from_sq", ctypes.c_int),
        ("to_sq", ctypes.c_int),
        ("special", ctypes.c_char),
    ]


class FfiSquare(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_int),
        ("color", ctypes.c_int),
        ("hasmoved", ctypes.c_int),
        ("passable", ctypes.c_int),
    ]


SC_MAX_LEGAL_MOVES = 256

GamePtr = ctypes.POINTER(GameHandle)

lib.sc_new_game.argtypes = []
lib.sc_new_game.restype = GamePtr

lib.sc_free_game.argtypes = [GamePtr]
lib.sc_free_game.restype = None

lib.sc_reset_game.argtypes = [GamePtr]
lib.sc_reset_game.restype = None

lib.sc_turn.argtypes = [GamePtr]
lib.sc_turn.restype = ctypes.c_int

lib.sc_outcome.argtypes = [GamePtr]
lib.sc_outcome.restype = ctypes.c_int

lib.sc_winner.argtypes = [GamePtr]
lib.sc_winner.restype = ctypes.c_int

lib.sc_in_check.argtypes = [GamePtr]
lib.sc_in_check.restype = ctypes.c_int

lib.sc_legal_moves.argtypes = [GamePtr, ctypes.POINTER(FfiMove), ctypes.c_int]
lib.sc_legal_moves.restype = ctypes.c_int

lib.sc_make_move.argtypes = [GamePtr, ctypes.c_int, ctypes.c_int, ctypes.c_char]
lib.sc_make_move.restype = ctypes.c_int

lib.sc_board_array.argtypes = [GamePtr, ctypes.POINTER(FfiSquare)]
lib.sc_board_array.restype = None
