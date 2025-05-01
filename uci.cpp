#include "search_module.h"
#include <string>

// make my engine UCI compliant

//move format is long algebraic notation
std::string move_to_long_algebraic(move m) {
    char from_file = 'a' + (m.from_position % 8);
    char from_rank = '1' + (m.from_position / 8);
    char to_file = 'a' + (m.to_position % 8);
    char to_rank = '1' + (m.to_position / 8);

    return std::string(1, from_file) + std::string(1, from_rank) +
           std::string(1, to_file) + std::string(1, to_rank);
}

int main() {
    // initial game state
    // convention: least significant bit (rightmost bit) is A1

    U64 w_pawn = 65280;
    U64 w_knight = 66;
    U64 w_bishop = 36;
    U64 w_rook = 129;
    U64 w_queen = 8;
    U64 w_king = 16;
    U64 b_pawn = 71776119061217280;
    U64 b_knight = 4755801206503243776;
    U64 b_bishop = 2594073385365405696;
    U64 b_rook = 9295429630892703744ULL;
    U64 b_queen = 576460752303423488;
    U64 b_king = 1152921504606846976;

    U64 w_en_passant = 0;
    U64 b_en_passant = 0;

    bool w_long_castle = true;
    bool w_short_castle = true;
    bool b_long_castle = true;
    bool b_short_castle = true;

    // initialize game state
    std::array<U64, 12> piece_bitboards = {w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king, b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king};
    std::array<U64, 2> en_passant_bitboards = {w_en_passant, b_en_passant};
    game_state initial_game_state(piece_bitboards, en_passant_bitboards, w_long_castle, w_short_castle, b_long_castle, b_short_castle);

    // create lookup tables
    lookup_tables_wrap lookup_tables;
    generate_lookup_tables(lookup_tables);

    // create zobrist randoms
    zobrist_randoms zobrist;

    // create move object array
    //untill depth 256
    std::array<std::array<move, 256>, MAX_DEPTH> moves_stack;

    // create move undo object array
    std::array<move_undo, 256> undo_stack;

    // create transposition table
    // get transposition table index like this: hash & (TT_SIZE - 1)
    // vector to put it on the heap, otherwize we get a segfault
    std::vector<transposition_table_entry> transposition_table;
    transposition_table.resize(TT_SIZE);

    // initialize
    game_state state = initial_game_state;
    std::array<int, 64> piece_on_square;
    U64 zobrist_hash = init_zobrist_hashing_mailbox(state, zobrist, false, piece_on_square);
    U64 occupancy_bitboard = get_occupancy(state.piece_bitboards);
    int negamax_depth = 6;
    bool color = false;

    // visualize initial game state
    std::cout << "Initial game state:" << std::endl;
    visualize_game_state(state);

    // game loop
    while (true) {
        
        auto start = std::chrono::high_resolution_clock::now();

        iterative_deepening(state, negamax_depth, color, lookup_tables, occupancy_bitboard, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square);
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        std::cout << "Time taken: " << duration.count() << " ms" << std::endl;

        // other player plays
        color = !color;
        // generate pseudo-legal moves
        std::array<move, 256> moves2;
        int move_count2 = pseudo_legal_move_generator(
            moves2, state, color, lookup_tables, occupancy_bitboard);

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count2; i++) {

            apply_move(state, moves2[i], zobrist_hash, zobrist, undo_stack[0], piece_on_square);
            U64 new_occupancy = get_occupancy(state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(state, !color, lookup_tables, new_occupancy)) {
                std::cout << "from: " << index_to_chess(moves2[i].from_position) << " to: " << index_to_chess(moves2[i].to_position) << " move index: " << i << std::endl;
            }

            // Undo the move
            undo_move(state, moves2[i], zobrist_hash, zobrist, undo_stack[0], piece_on_square);
        }

        // ask for move
        int player_move;
        std::cout << "Move: ";
        std::cin >> player_move;

        apply_move(state, moves2[player_move], zobrist_hash, zobrist, undo_stack[0], piece_on_square);
        visualize_game_state(state);

        // update state
        color = !color;
        occupancy_bitboard = get_occupancy(state.piece_bitboards);

    }

    return 0;
}