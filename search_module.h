#include "move_generation.h"
#include <limits>
#include <string>
#include <algorithm>
#include <chrono>

// negamax with alpha-beta pruning, transposition table, move ordering and iterative deepening

//global constants
constexpr int INF = std::numeric_limits<int>::max() / 2;
constexpr size_t TT_SIZE = 1 << 20;
constexpr int MAX_DEPTH = 256;

// piece values in centipawns
constexpr int pawn_value = 100;
constexpr int knight_value = 320;
constexpr int bishop_value = 330;
constexpr int rook_value = 500;
constexpr int queen_value = 900;
constexpr int king_value = 20000;

constexpr std::array<int, 6> piece_values = {pawn_value, knight_value, bishop_value, rook_value, queen_value, king_value};

// transposition tables
struct transposition_table_entry {
    U64 hash;
    int depth;
    int score;
    int flag; // 0: exact, 1: alpha, 2: beta
    move best_move;
};

//useful functions
std::string index_to_chess(int index);
void visualize_game_state(const game_state& state);

//evaluation
int evaluation(game_state &state);

// search algorithm

// fast negamax search with alpha-beta pruning.
int negamax(game_state &state, int depth, int alpha, int beta, bool color, 
    lookup_tables_wrap& lookup_tables, const U64& occupancy_bitboard, int current_depth,
    zobrist_randoms& zobrist, U64& zobrist_hash,
    std::array<std::array<move, 256>, 256>& moves_stack, 
    std::array<move_undo, 256>& undo_stack,
    std::vector<transposition_table_entry>& transposition_table,
    std::array<int, 64>& piece_on_square,
    std::array<move, MAX_DEPTH>& pv, int& pv_length);

move iterative_deepening(game_state& state, int max_depth, bool color,
    lookup_tables_wrap& lookup_tables, U64& occupancy_bitboard,
    zobrist_randoms& zobrist, U64& zobrist_hash,
    std::array<std::array<move, 256>, 256>& moves_stack, 
    std::array<move_undo, 256>& undo_stack, 
    std::vector<transposition_table_entry>& transposition_table,
    std::array<int, 64> piece_on_square);