# yvl-chess
UCI-compliant chess engine written in C++.

## Table of Contents
1. Getting Started
    - Prerequisites
    - Compilation
    - Supported Commands
2. Architecture
    - Bitboard Move Generation
    - Magic Bitboards
    - Negamax Search & Alpha-beta Pruning
    - Iterative Deepening
    - Transposition Tables
    - Move Ordering
    - Evaluation Function
3. Testing and Benchmarking
4. Performance
5. License

## Getting Started
### Prerequisites
- C++ compiler (e.g. g++, clang)

### Compilation
```
# Clone repository
$ git clone https://github/yvlaere/yvl-chess.git
$ cd yvl-chess

# Compile
$ g++ uci.cpp search_module.cpp move_generation.cpp -O3 -o yvl-bot
```

### Running the Engine
```
# Run in UCI mode
$ ./yvl-bot
uci
```

### Supported Commands
The engine supports the following UCI commands
- `uci`: Enter UCI mode
- `isready`: Check if the engine is ready
- `ucinewgame`: Reset the internal board representation and prepare for a new game
- `position`: Provide a position (`startpos` or `fen`) and apply the specified `moves` to update the internal board representation
- `go`: Calculate the best move
- `quit`: Exit the program

More information about the Universal Chess Interface protocol can be found here: https://backscattering.de/chess/uci/

## Architecture
### Bitboard Move Generation
The internal game state representation and move generation are done by using 64-bit integers (bitboards) to store information about the chess board. Bitboards are memory efficient and allow fast bitwise operations to manipulate the board.

Their memory efficiency allows for fast lookup tables, speeding up move generation. These lookup tables are populated at the startup of the engine. For the pawns, a lookup table for normal moves and one for attacks is created. For the knight and king, a lookup table for attacks is created. Sliding pieces (bishop, rook and queen) require a more complex kind of lookup table, a magic lookup table.

### Magic bitboards
Sliding pieces need to take blocking pieces into account. So a lookup table containing all precalculated attack bitboards for all squares, for all possible sets of blocker bitboards (bitboard containing the location of all blocking pieces) are needed. This lookup table needs to map the blocker bitboards to the correct attack set. The blocker bitboards are too big to be used as a key, so it is hashed into a smaller key by multiplying it with a magic number and dropping the least significant bits. These magic numbers are generated using brute force calculation at the startup of the engine.

Further explanation: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html

### Negamax search and ALpha-beta Pruning

## Testing and Benchmarking
- The `perft.cpp` script can be used to verify the correctness of the move generation by comparing the output with known results: https://www.chessprogramming.org/Perft_Results

- The `engine_testing.cpp` script can be used to test new features and contains a simple interface to play chess against the engine.

## Performance
The chess engine was evaluated using cutechess-cli (https://github.com/cutechess/cutechess) in a gauntlet against multiple instances of stockfish (https://stockfishchess.org/), with their elo set at 1600, 1700, 1800 and 1900. Averaging out the elo values for each one of the opponents results in an elo of 1628.25
|Rank | Name | Elo |    +/-  | Games |  Score  |  Draw
| --- | --- | --- | --- | --- | --- | --- |
| 0 | yvl-bot | -57  |    76   |   80 |  41.9%  |  6.3%
| 1 | SF1800  | 215  |   223   |   20 |  77.5%  |  5.0%
| 2 | SF1900  |  168  |   193   |   20 |  72.5%  |  5.0%
| 3 | SF1700  | -35  |   155   |   20 |  45.0%  | 10.0%
| 4 | SF1600  |  -89  |   167   |   20 |  37.5%  | 5.0%

The chess engine is available on lichess as yvl-bot.

## License
This project is licensed under the GPL-3.0 license (https://www.gnu.org/licenses/quick-guide-gplv3.html)





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
The chess engine was evaluated using cutechess-cli in a gauntlet against multiple instances of stockfish, with their elo set at 1600, 1700, 1800 and 1900. Averaging out the elo values for each one of the opponents results in an elo of 1 628.25
|Rank | Name |                         Elo |    +/-  | Games |  Score  |  Draw
| --- | --- | --- | --- | --- | --- | --- |
| 0 | yvl-bot |                      -57  |    76   |   80 |  41.9%  |  6.3%
| 1 | SF1800  |                      215  |   223   |   20 |  77.5%  |  5.0%
| 2 | SF1900  |                       168  |   193   |   20 |  72.5%  |  5.0%
| 3 | SF1700  |                       -35  |   155   |   20 |  45.0%  | 10.0%
| 4 | SF1600  |                     -89  |   167   |   20 |  37.5%  | 5.0%

The chess engine is available on lichess as yvl-bot.

## Compilation
Compile the engine as follows:
```g++ uci.cpp search_module.cpp move_generation.cpp -O3 -o yvl-bot```