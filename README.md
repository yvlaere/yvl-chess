# chess_engine
UCI-compliant chess engine written in C++.

## Move generation
Moves are generated using bitboard attack lookup tables and magic bitboards. Moves are made and unmade. Details in jupyter notebooks.

## Move search
### Negamax
Minimax algorithm for a two-player zero-sum game. A recursive depth first search is performed to a predefined depth (or a terminal node) where nodes (game states) get evaluated. The best score is passed to the parental node. The score gets negated every other layer to simulate each player choosing the best possible move.
### Alpha-beta pruning
Method to reduce the search space. If a move is encountered that makes a branch useless, the branch is no longer explored.
### Evaluation
Game states are evaluated by summing piece values, modified by piece square tables. The values for both colors are subtracted from each other.
#### Piece square tables
Arrays that represent the positional value of a square for a certain piece type. Each piece type has a piece square table that has 64 values, one for each square. The values get added or subtracted from the value of that piece.
### Transposition table
A Transposition Table is a database that stores results of previously performed searches. It is a way to greatly reduce the search space of  the game tree. During their search, chess engines encounter the same position multiple times, transposition tables prevent the need to search these positions multiple times.
### Move ordering
By ordering moves in the recursive search function, alpha-beta cutoffs are reached faster if more promising moves are searched earlier. Current move ordering prioritizes the best move according to the transposition table, followed by MVV-LVA captures.
#### MVV-LVA
Most valuable victim - least valuable attacker. Moves are sorted to evaluate captures where the victim is most valuable and the attacker is least valuable.
### Iterative deepening
The search function is used with increasing depth. The best move of the previous iteration is used for ordering moves in the next iteration, increasing branch pruning by causing earlier beta cutoffs.

## Strength
The chess engine was evaluated using cutechess-cli in a gauntlet against multiple instances of stockfish, with their elo set at 1400, 1600, 1700 and 1800.
Rank Name                          Elo     +/-   Games   Score    Draw
   0 yvl-bot                       -26      74      80   46.3%    7.5%
   1 SF1800                        338     nan      20   87.5%    5.0%
   2 SF1700                         89     167      20   62.5%    5.0%
   3 SF1600                         35     155      20   55.0%   10.0%
   4 SF1400                       -382     nan      20   10.0%   10.0%

The chess engine is available on lichess as yvl-bot.

## Compilation
Compile the engine as follows:
```g++ uci.cpp search_module.cpp move_generation.cpp -O3 -o yvl-bot```