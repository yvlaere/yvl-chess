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
    
    int white_score = 0;
    int black_score = 0;

    // iterate over all pieces, except king
    for (int piece_index = 0; piece_index < 5; ++piece_index) {
        U64 bb_white = state.piece_bitboards[piece_index];
        while (bb_white) {
            int square = __builtin_ctzll(bb_white);
            
            white_score += piece_values[piece_index];
            white_score += piece_square_tables[piece_index][square];
            
            bb_white &= bb_white - 1;
        }

        U64 bb_black = state.piece_bitboards[piece_index + 6];
        while (bb_black) {
            int square = __builtin_ctzll(bb_black);
            black_score += piece_values[piece_index];
            black_score += piece_square_tables[piece_index][63 - square];
            bb_black &= bb_black - 1;
        }
    }

    // king evaluation
    U64 bb_white_king = state.piece_bitboards[5];
    U64 bb_black_king = state.piece_bitboards[11];
    int white_king_square = __builtin_ctzll(bb_white_king);
    int black_king_square = __builtin_ctzll(bb_black_king);

    if (white_score + black_score < 1400) {
        // endgame evaluation
        white_score += endgame_king_square_table[white_king_square];
        black_score += endgame_king_square_table[63 - black_king_square];
    }
    else {
        // middle game evaluation
        white_score += piece_square_tables[5][white_king_square];
        black_score += piece_square_tables[5][63 - black_king_square];
    }

    white_score += piece_values[5];
    black_score += piece_values[5];

    int score = white_score - black_score;
    return score;
}

// search algorithm

// move ordering
void order_moves(std::array<int, 256>& move_order, 
    std::array<int, 256>& scores, std::array<move, 256>& moves, int& move_count,
    std::array<int, 64>& piece_on_square, int& num_non_quiet,
    move& best_move, int& current_depth,
    std::array<std::array<move, 2>, MAX_DEPTH>& killer_moves,
    std::array<std::array<int, 64>, 64>& history_moves) {

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {

        int score = 0;
        int victim_index = piece_on_square[moves[i].to_position];

        // check for best move
        if (moves[i].piece_index == best_move.piece_index &&
            moves[i].from_position == best_move.from_position &&
            moves[i].to_position == best_move.to_position) {
            //std::cout << "best move" << std::endl;
            score += 10000;
            num_non_quiet++;
        }
        // check for killer move
        else if (killer_moves[current_depth][0].from_position == moves[i].from_position &&
            killer_moves[current_depth][0].to_position == moves[i].to_position) {
            score += 9500;
            num_non_quiet++;
        }
        else if (killer_moves[current_depth][1].from_position == moves[i].from_position &&
            killer_moves[current_depth][1].to_position == moves[i].to_position) {
            score += 9500;
            num_non_quiet++;
        }
        // check for capture
        else if (victim_index > 0) {
            int victim_value = piece_values[victim_index%6];
            int attacker_value = piece_values[moves[i].piece_index%6];
            score = victim_value*10 - attacker_value;
            num_non_quiet++;
        }
        
        // quiet moves
        else {
            score = history_moves[moves[i].from_position][moves[i].to_position];
        }

        scores[i] = score;
        move_order[i] = i; // store the index
    }

    // sort moves based on scores
    std::sort(move_order.begin(), move_order.begin() + move_count, [&](int a, int b) {
        return scores[a] > scores[b];
    });
}

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
    std::array<move, MAX_DEPTH>& pv, int& pv_length,
    std::array<std::array<move, 2>, MAX_DEPTH>& killer_moves,
    std::array<std::array<int, 64>, 64>& history_moves, 
    NNUE_accumulator& accumulator,
    const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1,
    const linear_layer<HIDDEN1_SIZE, HIDDEN2_SIZE>& layer2,
    const linear_layer<HIDDEN2_SIZE, HIDDEN3_SIZE>& layer3,
    const linear_layer<HIDDEN3_SIZE, OUTPUT_SIZE>& layer4) {

    if (depth == 0) {
        pv_length = 0;
        int eval = nnue_evaluation(accumulator, layer2, layer3, layer4);
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

    // null move pruning (needs to be before move generation)
    bool in_check = pseudo_to_legal(state, !color, lookup_tables, occupancy_bitboard);
    if (depth >= 3 && in_check) {
        // null move
        U64 null_zobrist_hash = zobrist_hash ^ zobrist.zobrist_black_to_move;
        int score = -negamax(state, depth - 3, -beta, -beta + 1, !color, lookup_tables, occupancy_bitboard, current_depth + 1, zobrist, null_zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square, child_pv, child_pv_length, killer_moves, history_moves, accumulator, layer1, layer2, layer3, layer4);
        if (score >= beta) {
            return score;
        }
    }
    
    // generate moves from the current position.
    // generate pseudo-legal moves
    std::array<move, 256>& moves = moves_stack[current_depth];
    int move_count = pseudo_legal_move_generator(moves, state, color, lookup_tables, occupancy_bitboard);

    std::array<int, 256> move_order;
    std::array<int, 256> scores;
    int num_non_quiet = 0;

    // order moves
    order_moves(move_order, scores, moves, move_count, piece_on_square, num_non_quiet, best_move, current_depth, killer_moves, history_moves);

    int max_score = -INF;
    int best_move_index = -1;
    int legal_moves = 0;
    int original_alpha = alpha;
    int original_beta = beta;
    bool LMR = false;

    // iterate over all pseudo-legal moves
    for (int i = 0; i < move_count; i++) {
        // get the move index from the sorted order
        int move_index = move_order[i];

        move_undo& undo = undo_stack[current_depth];
        apply_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square, layer1, accumulator);
        U64 new_occupancy = get_occupancy(state.piece_bitboards);

        // ensure move is legal (not putting king in check)
        if (pseudo_to_legal(state, !color, lookup_tables, new_occupancy)) {
            // apply negamax
            int score = -negamax(state, depth - 1 - LMR, -beta, -alpha, !color, lookup_tables, new_occupancy, current_depth + 1, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square, child_pv, child_pv_length, killer_moves, history_moves, accumulator, layer1, layer2, layer3, layer4);
            legal_moves++;

            // late move reductions
            if (!LMR && !in_check && legal_moves > 2 && depth > 3) {
                LMR = true;
            }

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
                // beta cutoff
                // Undo the move
                undo_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square, layer1, accumulator);
                
                // store the killer move
                if (killer_moves[current_depth][0].from_position != moves[move_index].from_position ) {
                    killer_moves[current_depth][1] = killer_moves[current_depth][0];
                    killer_moves[current_depth][0] = moves[move_index];
                }

                // update history heuristic score
                if (i > num_non_quiet) {
                    history_moves[moves[move_index].from_position][moves[move_index].to_position] += depth * depth;
                }

                break;
            }
        }

        // Undo the move
        undo_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square, layer1, accumulator);
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
    std::array<int, 64> piece_on_square,
    std::array<std::array<move, 2>, MAX_DEPTH>& killer_moves, 
    std::array<std::array<int, 64>, 64>& history_moves, 
    NNUE_accumulator& accumulator,
    const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1,
    const linear_layer<HIDDEN1_SIZE, HIDDEN2_SIZE>& layer2,
    const linear_layer<HIDDEN2_SIZE, HIDDEN3_SIZE>& layer3,
    const linear_layer<HIDDEN3_SIZE, OUTPUT_SIZE>& layer4) {

    auto start_time = std::chrono::high_resolution_clock::now();
    int time_limit_ms = 1000000;

    std::array<move, 256>& moves = moves_stack[0];
    int move_count = pseudo_legal_move_generator(moves, state, color, lookup_tables, occupancy_bitboard);

    std::array<int, 256> move_order;
    std::array<int, 256> scores;

    int root_PV_moves_count = 0;
    std::array<move, MAX_DEPTH> best_PV_moves;

    // iterate over all depths
    for (int negamax_depth = 0; negamax_depth <= max_depth; negamax_depth++) {

        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = current_time - start_time;

        if (elapsed.count() > time_limit_ms) {
            break;
        }

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
            apply_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square, layer1, accumulator);
            U64 new_occupancy = get_occupancy(state.piece_bitboards);

            // ensure move is legal (not putting king in check)
            if (pseudo_to_legal(state, !color, lookup_tables, new_occupancy)) {
                
                // apply negamax
                int score = -negamax(state, negamax_depth, -INF, INF, !color, lookup_tables, new_occupancy, 1, zobrist, zobrist_hash, moves_stack, undo_stack, transposition_table, piece_on_square, root_PV_moves, root_PV_moves_count, killer_moves, history_moves, accumulator, layer1, layer2, layer3, layer4);

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
            undo_move(state, moves[move_index], zobrist_hash, zobrist, undo, piece_on_square, layer1, accumulator);
        }

        std::cout << "Depth: " << negamax_depth << ", Score: " << max_score << std::endl;
    }

    // update state
    occupancy_bitboard = get_occupancy(state.piece_bitboards);
    apply_move(state, best_PV_moves[0], zobrist_hash, zobrist, undo_stack[0], piece_on_square, layer1, accumulator);
    
    //visualize_game_state(state);  

    return best_PV_moves[0];
}