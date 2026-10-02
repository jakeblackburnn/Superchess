# Superchess 
Superchess is a superset of the chess ruleset. I want to try training alphazero-style models 
to play superchess as I learn more about reinforcement learning.

### rules: 
0. normal rules of chess, with additional moves on all the pieces
1. pawns can take one step backward
2. knights can make normal pawn moves (forward, take diagonally)
3. bishops can make normal king moves (step horizontally / vertically) 
4. rooks can make normal king moves like bishops 
5. queens can make normal knight moves 
6. kings assume ALL the moves of ALL its neighboring pieces
7. pawns can promote to king
8. the win condition is now to take all the opponents kings / pawns 
9. checkmate only occurs if the opponent has no pawns left
