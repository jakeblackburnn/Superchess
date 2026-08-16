

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


### specs & roadmap

**language**: C core library + Python UI

**prototypes**: made with claude code, prototype versions already working in C and Python to learn about implementation 
and to review / compare design decisions. The real version should probably be simpler than what claude created.

**development phases**: Start by developing simple C code for a CLI based Chess game with a clear boundary 
between the core library and the CLI interface. Then add the superchess ruleset as an option when setting up a game. 
Finailly, with a the core libary solidified as an FFI boundary, create a stylish and straightforward UI in python

**extension / future work**: train a small superchess engine with RL similar to AlphaZero, 
to learn about RL and implement an AlphaZero style RL model
