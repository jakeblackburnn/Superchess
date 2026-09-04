"""Self-play game generation: az.mcts + az.nn driven through az.bindings.Engine."""

from .driver import generate_games
from .worker import play_game

__all__ = ["generate_games", "play_game"]
