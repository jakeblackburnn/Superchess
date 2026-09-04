"""Expert-data warm-start hook — not implemented for the MVP.

Deliberately deferred: no Stockfish binary, python-chess, or PGN dataset exists in
this project yet, and building that pipeline (dataset acquisition/parsing, or a
UCI wrapper around an engine) is scope of its own. The training loop's
`--init-checkpoint` flag (az.train.cli) is the intended integration point: point it
at a checkpoint produced by whatever this function eventually becomes, and
az.train.loop will load it as iteration-0 weights instead of a random init.
"""

from __future__ import annotations

from az.nn.model import SuperChessNet


def pretrain_from_pgn(pgn_path: str, **kwargs) -> SuperChessNet:
    """Supervised-pretrain a SuperChessNet's policy head on (board, human move)
    pairs parsed from a PGN dataset. Not implemented — see module docstring."""
    raise NotImplementedError(
        "expert-data warm-start is deferred for the MVP; train from random init, "
        "or supply a checkpoint via --init-checkpoint once this is built"
    )
