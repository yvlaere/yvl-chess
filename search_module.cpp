#include "search_module.h"

std::string index_to_chess(int index) {
    if (index < 0 || index >= 64) {
        return ""; // or handle the error as appropriate
    }
    char file = 'a' + (index % 8);      // Files: a-h
    int rank = (index / 8) + 1;           // Ranks: 1-8
    return std::string(1, file) + std::to_string(rank);
}

// to remove visualization, only for debugging purposes
void visualize_game_state(const game_state& state) {
    const char* piece_symbols = "PNBRQKpnbrqk";

    char board[8][8] = {};

    // Initialize board with empty spaces
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            board[r][c] = '.';

    // Fill the board with pieces
    for (int i = 0; i < 12; ++i) {
        U64 bitboard = state.piece_bitboards[i];
        for (int square = 0; square < 64; ++square) {
            if (bitboard & (1ULL << square)) {
                int row = 7 - (square / 8);
                int col = square % 8;
                board[row][col] = piece_symbols[i];
            }
        }
    }

    // Mark en passant squares
    for (int i = 0; i < 2; ++i) {
        U64 ep_board = state.en_passant_bitboards[i];
        for (int square = 0; square < 64; ++square) {
            if (ep_board & (1ULL << square)) {
                int row = 7 - (square / 8);
                int col = square % 8;
                board[row][col] = '*';  // En passant target square
            }
        }
    }

    // Print the board
    std::cout << "  a b c d e f g h\n";
    std::cout << "  ----------------\n";
    for (int r = 0; r < 8; ++r) {
        std::cout << (8 - r) << "| ";
        for (int c = 0; c < 8; ++c) {
            std::cout << board[r][c] << ' ';
        }
        std::cout << "|\n";
    }
    std::cout << "  ----------------\n";

    // Print castling rights
    std::cout << "Castling rights: "
              << (state.w_long_castle ? "Q" : "-")
              << (state.w_short_castle ? "K" : "-")
              << (state.b_long_castle ? "q" : "-")
              << (state.b_short_castle ? "k" : "-") << "\n";
}

// evaluation
int evaluation(game_state &state) {
    // basic evaluation with piece square tables, based on the simplified evaluation function on chessprogramming.org

    // piece square tables
    std::array<int, 64> pawn_square_table = {0, 0, 0, 0, 0, 0, 0, 0, 5, 10, 10, -20, -20, 10, 10, 5, 5, -5, -10, 0, 0, -10, -5, 5, 0, 0, 0, 20, 20, 0, 0, 0, 5, 5, 10, 25, 25, 10, 5, 5, 10, 10, 20, 30, 30, 20, 10, 10, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int, 64> knight_square_table = {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0, 5, 5, 0, -20, -40, -30, 5, 10, 15, 15, 10, 5, -30, -30, 0, 15, 20, 20, 15, 0, -30, -30, 5, 15, 20, 20, 15, 5, -30, -30, 0, 10, 15, 15, 10, 0, -30, -40, -20, 0, 0, 0, 0, -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};
    std::array<int, 64> bishop_square_table = {-20, -10, -10, -10, -10, -10, -10, -20, -10, 5, 0, 0, 0, 0, 5, -10, -10, 10, 10, 10, 10, 10, 10, -10, -10, 0, 10, 10, 10, 10, 0, -10, -10, 5, 5, 10, 10, 5, 5, -10, -10, 0, 5, 10, 10, 5, 0, -10, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -10, -10, -10, -10, -20};
    std::array<int, 64> rook_square_table = {0, 0, 0, 5, 5, 0, 0, 0, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, -5, 0, 0, 0, 0, 0, 0, -5, 5, 10, 10, 10, 10, 10, 10, 5, 0, 0, 0, 0, 0, 0, 0, 0};
    std::array<int, 64> queen_square_table = {-20, -10, -10, -5, -5, -10, -10, -20, -10, 0, 5, 0, 0, 0, 0, -10, -10, 5, 5, 5, 5, 5, 0, -10, 0, 0, 5, 5, 5, 5, 0, -5, -5, 0, 5, 5, 5, 5, 0, -5, -10, 0, 5, 5, 5, 5, 0, -5, -10, 0, 0, 0, 0, 0, 0, -10, -20, -10, -10, -5, -5, -10, -10, -20};
    std::array<int, 64> king_square_table = {20, 30, 10, 0, 0, 10, 30, 20, 20, 20, 0, 0, 0, 0, 20, 20, -10, -20, -20, -20, -20, -20, -20, -10, -20, -30, -30, -40, -40, -30, -30, -20, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30};
    
    std::array<std::array<int, 64>, 6> piece_square_tables = {pawn_square_table, knight_square_table, bishop_square_table, rook_square_table, queen_square_table, king_square_table};

    std::array<int, 64> endgame_king_square_table = {-50, -30, -30, -30, -30, -30, -30, -50, -30, -30, 0, 0, 0, 0, -30, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 30, 40, 40, 30, -10, -30, -30, -10, 20, 30, 30, 20, -10, -30, -30, -20, -10, 0, 0, -10, -20, -30, -50, -40, -30, -20, -20, -30, -40, -50};
    
    int score = 0;

    for (int piece_index = 0; piece_index < 12; ++piece_index) {
        U64 bb = state.piece_bitboards[piece_index];
        while (bb) {
            int square = __builtin_ctzll(bb);
            
            // black pieces
            if (piece_index >= 6) {
                score -= piece_values[piece_index - 6];
                score -= piece_square_tables[piece_index - 6][63 - square];
            }

            // white pieces
            else {
                score += piece_values[piece_index];
                score += piece_square_tables[piece_index][square];
            }
            
            bb &= bb - 1;
        }
    }

    return score;
}

// search algorithm

// fast negamax search with alpha-beta pruning.
// 'depth' is the remaining search depth, and alpha-beta parameters prune branches.
int negamax(game_state &state, int depth, int alpha, int beta, bool color, 
    lookup_tables_wrap& lookup_tables,
    const U64& occupancy_bitboard, int current_depth,
    zobrist_randoms& zobrist, U64& zobrist_hash,
    std::array<std::array<move, 256>, 256>& moves_stack, 
    std::array<move_undo, 256>& undo_stack,
    std::vector<transposition_table_entry>& transposition_table,
    std::array<int, 64>& piece_on_square,
    std::array<move, MAX_DEPTH>& pv, int& pv_length) {

    if (depth == 0) {
        pv_length = 0;
        int eval = evaluation(state);
        return color ? -eval : eval;
    }

    std::array<move, MAX_DEPTH> child_pv;
    int child_pv_length = 0;

    // check transposition table for pruning
    U64 TT_index = zobrist_hash & (TT_SIZE - 1);
    transposition_table_entry& entry = transposition_table[TT_index];
    move best_move;
    if (entry.hash == zobrist_hash && entry.depth >= depth) {
        if (entry.flag == 0) {
            pv[0] = entry.best_move;
            pv_length = 1;
            return entry.score;
        }
        else if (entry.flag == 1) {
            alpha = std::max(alpha, entry.score);
        }
        else if (entry.flag == 2) {
            beta = std::min(beta, entry.score);
        }
        if (alpha >= beta) {
            pv[0] = entry.best_move;
            pv_length = 1;
            return entry.score;
        }
        best_move = entry.best_move;
    }
    
    // generate moves from the current position.
    // generate pseudo-legal moves
    std::array<move, 256>& moves = moves_stack[current_depth];
    int move_count = pseudo_legal_move_generator(moves, state, color, lookup_tables, occupancy_bitboard);

    std::array<int, 256> move_order;
    std::array<int, 256> scores;

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {

        int score = 0;

        // check for capture
        int victim_index = piece_on_square[moves[i].to_position];
        if (victim_index > 0) {
            int victim_value = piece_values[victim_index%6];
            int attacker_value = piece_values[moves[i].piece_index%6];
            score = victim_value*10 - attacker_value;
        }
        
        // check for best move
        if (moves[i].piece_index == best_move.piece_index &&
            moves[i].from_position == best_move.from_position &&
            moves[i].to_position == best_move.to_position) {
            score += INF;
        }

        scores[i] = score;
        move_order[i] = i; // store the index
    }

    // sort moves based on scores
    std::sort(move_order.begin(), move_order.begin() + move_count, [&](int a, int b) {
        return scores[a] > scores[b];
    });

    int max_score = -INF;
    int best_move_index = -1;
    int legal_moves = 0;
    int original_alpha = alpha;
    int original_beta = beta;

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {
        // get the move index from the sorted order
        int move_index = move_order[i];

        move_undo& undo = undo_stack[current_depth];
        apply_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square);
        U64 new_occupancy = get_occupancy(state.piece_bitboards);

        // ensure move is legal (not putting king in check)
        if (pseudo_to_legal(state, !color, lookup_tables, new_occupancy)) {
            // apply negamax
            int score = -negamax(state, depth - 1, -beta, -alpha, !color, lookup_tables, new_occupancy, current_depth + 1, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square, child_pv, child_pv_length);
            legal_moves++;

            if (score > max_score) {
                max_score = score;
                best_move_index = move_index;

                pv[0] = moves[move_index];
                for (int j = 0; j < child_pv_length; ++j) {
                    pv[j + 1] = child_pv[j];
                }
                pv_length = child_pv_length + 1;
            }
            if (score > alpha) {
                alpha = score;
            }
            if (alpha >= beta) {

                // Undo the move
                undo_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square);
                break;
            }
        }

        // Undo the move
        undo_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square);
    }

    // terminal node: checkmate or stalemate.
    if (legal_moves == 0) {
        // king is attacked: checkmate
        if (pseudo_to_legal(state, !color, lookup_tables, occupancy_bitboard)) {
            // stalemate
            return 0;
        }
        else {
            // checkmate
            return -INF;
        }
    }

    // store the result in the transposition table
    entry.hash = zobrist_hash;
    entry.depth = depth;
    entry.score = max_score;
    entry.best_move = moves[best_move_index];
    entry.flag = 0; // exact score
    if (max_score <= original_alpha) {
        entry.flag = 2; // beta cutoff
    }
    else if (max_score >= original_beta) {
        entry.flag = 1; // alpha cutoff
    }
    
    return max_score;
}

move iterative_deepening(game_state& state, int max_depth, bool color,
    lookup_tables_wrap& lookup_tables, U64& occupancy_bitboard,
    zobrist_randoms& zobrist, U64& zobrist_hash,
    std::array<std::array<move, 256>, 256>& moves_stack, 
    std::array<move_undo, 256>& undo_stack, 
    std::vector<transposition_table_entry>& transposition_table,
    std::array<int, 64> piece_on_square) {

    std::array<move, 256>& moves = moves_stack[0];
    int move_count = pseudo_legal_move_generator(moves, state, color, lookup_tables, occupancy_bitboard);

    std::array<int, 256> move_order;
    std::array<int, 256> scores;

    int root_PV_moves_count = 0;
    std::array<move, MAX_DEPTH> best_PV_moves;

    // iterate over all depths
    for (int negamax_depth = 0; negamax_depth <= max_depth; negamax_depth++) {

        // initialize PV array
        int root_PV_moves_count = 0;
        std::array<move, MAX_DEPTH> root_PV_moves;

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count; i++) {

            int score = 0;

            // check for capture
            int victim_index = piece_on_square[moves[i].to_position];
            if (victim_index > 0) {
                int victim_value = piece_values[victim_index%6];
                int attacker_value = piece_values[moves[i].piece_index%6];
                score = victim_value*10 - attacker_value;
            }
            
            // check for best move if not in the first iteration
            if (negamax_depth > 0 &&
                moves[i].piece_index == best_PV_moves[0].piece_index &&
                moves[i].from_position == best_PV_moves[0].from_position &&
                moves[i].to_position == best_PV_moves[0].to_position) {
                score += INF;
            }

            scores[i] = score;
            move_order[i] = i; // store the index
        }

        // sort moves based on scores
        std::sort(move_order.begin(), move_order.begin() + move_count, [&](int a, int b) {
            return scores[a] > scores[b];
        });

        // apply negamax
        int max_score = -INF;

        // iterate over all pseudo-legal moves
        for (int i = 0; i < move_count; i++) {
            // get the move index from the sorted order
            int move_index = move_order[i];

            move_undo& undo = undo_stack[0];
            apply_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square);
            U64 new_occupancy = get_occupancy(state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(state, !color, lookup_tables, new_occupancy)) {
                
                // apply negamax
                int score = -negamax(state, negamax_depth, -INF, INF, !color, lookup_tables, new_occupancy, 1, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square, root_PV_moves, root_PV_moves_count);

                if (score > max_score) {
                    max_score = score;
                    best_PV_moves = root_PV_moves;
                    best_PV_moves[0] = moves[move_index];
                    for (int j = 0; j < root_PV_moves_count; ++j) {
                        best_PV_moves[j + 1] = root_PV_moves[j];
                    }
                }
            }

            // Undo the move
            undo_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square);
        }
    }

    // update state
    occupancy_bitboard = get_occupancy(state.piece_bitboards);
    apply_move(state, best_PV_moves[0], zobrist_hash, zobrist, undo_stack[0], piece_on_square);
    
    //visualize_game_state(state);  

    return best_PV_moves[0];
}