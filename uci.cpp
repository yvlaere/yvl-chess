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
            promotion_piece = 'r';
        } else if (m.promotion_piece_index == 3 || m.promotion_piece_index == 9) {
            promotion_piece = 'b';
        } else if (m.promotion_piece_index == 4 || m.promotion_piece_index == 10) {
            promotion_piece = 'q';
        }

        move_string += promotion_piece;
    }

    return move_string;
}

void fen_to_game_state(const std::string& fen, game_state& state) {
    // parse FEN string and update the game state
    
    int rank = 7;
    // iterate over the FEN string
    for(const char& c : fen) {
        if (c == ' ') {
            break; // end of piece placement
        }
        else if (c >= '1' && c <= '8') {
            rank -= c - '0'; // skip squares
        }
        else if (c == '/') {
            continue; // skip rank separator
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
                case 'q': piece_index = 10; break;// black queen
                case 'k': piece_index = 11; break;// black king
            }
            state.piece_bitboards[piece_index] |= (1ULL << (rank * 8 + (c - 'a')));
        }
    }
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



    std::string fen1 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string fen2 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";
    std::string fen3 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string fen4 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1";
    visualize_game_state(fen_to_game_state(fen1));
    visualize_game_state(fen_to_game_state(fen2));
    visualize_game_state(fen_to_game_state(fen3));
    visualize_game_state(fen_to_game_state(fen4));

    // UCI loop
    while (true) {
        
        std::string command;
        std::cin >> command;

        // split command into subcommands
        std::string subcommand;
        std::stringstream ss(command);
        std::vector<std::string> sub_commands;
        while (getline(ss, subcommand, ' ')) {
            sub_commands.push_back(subcommand);
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
            }
            else if (sub_commands[1] == "fen") {
                // handle FEN string
                std::string fen_string = sub_commands[2];
            }
        }
    }
    return 0;
}