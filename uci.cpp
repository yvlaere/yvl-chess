#include "search_module.h"
#include <string>
#include <sstream>

// make my engine UCI compliant

//move format is long algebraic notation
std::string move_to_long_algebraic(move m) {
    char from_file = 'a' + (m.from_position % 8);
    char from_rank = '1' + (m.from_position / 8);
    char to_file = 'a' + (m.to_position % 8);
    char to_rank = '1' + (m.to_position / 8);

    std::string move_string = std::string(1, from_file) + std::string(1, from_rank) +
           std::string(1, to_file) + std::string(1, to_rank);

    if (m.promotion_piece_index != m.piece_index) {
        char promotion_piece = 'n';
        if (m.promotion_piece_index == 2 || m.promotion_piece_index == 8) {
            promotion_piece = 'b';
        } else if (m.promotion_piece_index == 3 || m.promotion_piece_index == 9) {
            promotion_piece = 'r';
        } else if (m.promotion_piece_index == 4 || m.promotion_piece_index == 10) {
            promotion_piece = 'q';
        }

        move_string += promotion_piece;
    }

    return move_string;
}

game_state fen_to_game_state(const std::string& fen, bool& color) {
    // parse FEN string and update the game state
    
    // create empty game state
    game_state state({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0}, false, false, false, false);

    // parse the FEN string
    std::string sub_fen;
    std::stringstream ss(fen);
    std::vector<std::string> sub_fen_elements;
    while (getline(ss, sub_fen, ' ')) {
        sub_fen_elements.push_back(sub_fen);
    }

    int rank = 7;
    int file = 0;
    // iterate over the FEN string
    for(const char& c : sub_fen_elements[0]) {
        if (c >= '1' && c <= '8') {
            file += c - '0'; // skip squares
        }
        else if (c == '/') {
            rank -= 1;
            file = 0; // reset file
        }
        else {
            int piece_index = -1;
            switch (c) {
                case 'P': piece_index = 0; break; // white pawn
                case 'N': piece_index = 1; break; // white knight
                case 'B': piece_index = 2; break; // white bishop
                case 'R': piece_index = 3; break; // white rook
                case 'Q': piece_index = 4; break; // white queen
                case 'K': piece_index = 5; break; // white king
                case 'p': piece_index = 6; break; // black pawn
                case 'n': piece_index = 7; break; // black knight
                case 'b': piece_index = 8; break; // black bishop
                case 'r': piece_index = 9; break; // black rook
                case 'q': piece_index = 10; break; // black queen
                case 'k': piece_index = 11; break; // black king
            }
            state.piece_bitboards[piece_index] |= (1ULL << (rank*8 + file));
            file += 1; // move to the next file
        }
    }

    // color
    color = (sub_fen_elements[1] == "w") ? true : false;

    // castling rights
    for(const char& c : sub_fen_elements[2]) {
        switch (c) {
            case 'K': state.w_short_castle = true; break;
            case 'Q': state.w_long_castle = true; break;
            case 'k': state.b_short_castle = true; break;
            case 'q': state.b_long_castle = true; break;
        }
    }
    
    // en passant
    if (sub_fen_elements[3] != "-") {
        int file = sub_fen_elements[3][0] - 'a';
        int rank = sub_fen_elements[3][1] - '1';
        state.en_passant_bitboards[color] = (1ULL << (rank*8 + file));
    }

    return state;
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
    int negamax_depth = 5;
    bool color = false;

    // UCI loop
    while (true) {
        
        std::string command;
        std::getline(std::cin, command);

        // split command into subcommands
        std::string subcommand;
        std::stringstream ss(command);
        std::vector<std::string> sub_commands;
        for (std::string sub_command; ss >> sub_command;) {
            sub_commands.push_back(sub_command);
        }
        
        if (sub_commands[0] == "uci") {
            std::cout << "id name yvl-bot" << std::endl;
            std::cout << "id author yvl" << std::endl;
            std::cout << "uciok" << std::endl;
            continue;
        }
        else if (sub_commands[0] == "isready") {
            std::cout << "readyok" << std::endl;
            continue;
        }
        else if (sub_commands[0] == "quit") {
            break;
        }
        else if (sub_commands[0] == "ucinewgame") {
            // reset the game state
            state = initial_game_state;
            zobrist_hash = init_zobrist_hashing_mailbox(state, zobrist, false, piece_on_square);
            occupancy_bitboard = get_occupancy(state.piece_bitboards);
            continue;
        }
        else if (sub_commands[0] == "position") {
            if (sub_commands[1] == "startpos") {
                // reset the game state
                state = initial_game_state;
                zobrist_hash = init_zobrist_hashing_mailbox(state, zobrist, false, piece_on_square);
                occupancy_bitboard = get_occupancy(state.piece_bitboards);
                color = false;
            }
            else if (sub_commands[1] == "fen") {
                // handle FEN string
                
                // construct the FEN string
                std::string fen_string;
                for (int i = 2; i < sub_commands.size(); i++) {
                    std::string fen_part;
                    fen_part = sub_commands[2 + i];
                    if (fen_part == "moves") {
                        break;
                    }
                    else {
                        fen_string += fen_part + " ";
                    }
                }

                state = fen_to_game_state(fen_string, color);
                zobrist_hash = init_zobrist_hashing_mailbox(state, zobrist, color, piece_on_square);
                occupancy_bitboard = get_occupancy(state.piece_bitboards);
            }
            for (int i = 2; i < sub_commands.size(); i++) {
                if (sub_commands[i] == "moves") {
                    // handle moves
                    for (int j = i + 1; j < sub_commands.size(); j++) {
                        std::string move_string = sub_commands[j];

                        // generate all moves
                        std::array<move, 256> moves;
                        U64 occupancy_bitboard = get_occupancy(state.piece_bitboards);
                        int move_count = pseudo_legal_move_generator(moves, state, color, lookup_tables, occupancy_bitboard);
                        for (int k = 0; k < move_count; k++) {
                            std::string move_string = move_to_long_algebraic(moves[k]);
                            if (move_string == sub_commands[j]) {
                                // apply move
                                move_undo undo;
                                apply_move(state, moves[k], zobrist_hash, zobrist, undo, piece_on_square);
                                color = !color;
                                break;
                            }
                        }
                    }
                }
            }
        }
        else if (sub_commands[0] == "go") {
            // start the search

            int max_depth = 6;

            U64 occupancy_bitboard = get_occupancy(state.piece_bitboards);
            move best_move = iterative_deepening(state, max_depth, color, lookup_tables, occupancy_bitboard, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square);
            std::cout << "bestmove " << move_to_long_algebraic(best_move) << std::endl;
        }
    }

    return 0;
}