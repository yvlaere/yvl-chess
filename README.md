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

During move generation, pseudo-legal moves are generated first. These moves don't take into account whether the king is in check or not. The pseudo-legal moves are then filtered to only keep the legal moves.

### Magic bitboards
Sliding pieces need to take blocking pieces into account. So a lookup table containing all precalculated attack bitboards for all squares, for all possible sets of blocker bitboards (bitboard containing the location of all blocking pieces) are needed. This lookup table needs to map the blocker bitboards to the correct attack set. The blocker bitboards are too big to be used as a key, so it is hashed into a smaller key by multiplying it with a magic number and dropping the least significant bits. These magic numbers are generated using brute force calculation at the startup of the engine. The bishop and rook both have different lookup tables. The queen does not have any lookup tables and instead uses those of the bishop and rook.

Further explanation on bitboards and magic bitboards: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html

### Negamax search and Alpha-beta Pruning
Negamax is an implementation of minimax for two-player zero-sum games. A recursive depth first search is performed to a predefined depth (or until a terminal node is reached) where nodes (game states) get evaluated. The best score is passed to the parental node. The score gets negated every other layer to simulate each player choosing the best possible move.

Alpha-beta pruning is a method to reduce the search space. If a move is encountered that makes a branch useless, the branch is no longer explored (pruned).

Negamax and minimax return the same result. Negamax/minimax with and without alpha-beta pruning also return the same result, alpha-beta pruning just makes the search faster.

During the search, moves are made and unmade on the internal board. This incremental update approach prevents the need to copy the game state millions of times during the search, allowing for faster searches.

A good visual explanation of minimax with alpha-beta pruning can be found here: https://www.youtube.com/watch?v=l-hh51ncgDI

### Iterative Deepening
Alpha-beta pruning is much more efficient when promising moves are searched first, it leads to faster beta cutoffs. One way to search promising moves first is by using iterative deepening. The search function (negamax) is used with increasing depth. The best move of the previous iteration is used for ordering moves in the next iteration. Currently, only the first move of the principal variation (sequence of moves that the engine consider best) is used for move ordering in iterative deepening.

### Transposition Tables
Transposition tables are hash tables that store exact scores, lower bound values or upper bound values for previously encountered states. During a search, the same position is often encountered multiple times. Transposition tables can prevent the need for a re-evaluation of these positions. Entries are indexed by a part of the zobrist hash of the game state. There is not enough space in the transposition table to keep all visited game states. The current replacement paradigm is to always replace.

A zobrist hash is an incrementally updatable hash of a game state. Each element of the game state (piece types, piece locations, castling rights, en passant squares, side to move) has a random value associated with it. As moves are made and unmade during the search, the zobrist hash is incrementally updated by adding or removing the relevant random values using the XOR operation.

### Move Ordering
Move ordering makes alpha-beta pruning more efficient. The current move ordering approach puts the best transposition table move first, followed by captures sorted by MVV-LVA. The rest of the moves gets ordered using killer and history heuristics.

MVV-LVA stands for most valuable victim, least valuable attacker. It is a way to order captures by prioritizing valuable victims and unvaluable attackers.

### Null-Move Pruning
Forward-pruning heuristic that makes a “pass” (null move, forfeiting a turn) and searches at depth-2. If that reduced search causes a beta-cutoff, the full branch is cut off.

### Evaluation Function
During the search, positions need to be evaluated to obtain a score. Positions are evaluated by adding the values of all the pieces on the board together, modified by a position score in their piece-square tables.

## Testing and Benchmarking
- The `perft.cpp` script can be used to verify the correctness of the move generation by comparing the output with known results: https://www.chessprogramming.org/Perft_Results. Currently, the move generation achieves ~19M moves/s.

- The `engine_testing.cpp` script can be used to test new features and contains a simple interface to play chess against the engine.

## Performance
The chess engine was evaluated using cutechess-cli (https://github.com/cutechess/cutechess) in a gauntlet against multiple instances of stockfish (https://stockfishchess.org/), with their elo set at 1600, 1700, 1800 and 1900. Averaging out the elo values for each one of the opponents results in an elo of 1683.25
|Rank | Name | Elo |    +/-  | Games |  Score  |  Draw
| --- | --- | --- | --- | --- | --- | --- |
| 0 | yvl-bot | -35  |    72   |   80 |  45.0%  | 12.5%
| 1 | SF1900  | 168  |   193   |   20 |  72.5%  |  5.0%
| 2 | SF1800  | 147  |   176   |   20 |  70.0%  | 10.0%
| 3 | SF1700  | 53   |   151   |   20 |  57.5%  | 15.0%
| 4 | SF1600  | -241 |   192   |   20 |  20.0%  | 20.0%

The chess engine is available on lichess as yvl-bot: https://lichess.org/@/yvl-bot

## License
This project is licensed under the GPL-3.0 license (https://www.gnu.org/licenses/quick-guide-gplv3.html)
