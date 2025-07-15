#include <array>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>

using U64 = unsigned long long;

// game state
struct game_state {
    std::array<U64, 12> piece_bitboards;
    std::array<U64, 2> en_passant_bitboards;
    bool w_long_castle;
    bool w_short_castle;
    bool b_long_castle;
    bool b_short_castle;

    // constructor
    game_state(const std::array<U64, 12>& piece_bb, 
        const std::array<U64, 2>& en_passant_bb,
        bool wlc, bool wsc, bool blc, bool bsc) 
        : piece_bitboards(piece_bb),
        en_passant_bitboards(en_passant_bb),
        w_long_castle(wlc),
        w_short_castle(wsc),
        b_long_castle(blc),
        b_short_castle(bsc) {}
    };

// piece values in centipawns
constexpr int pawn_value = 100;
constexpr int knight_value = 320;
constexpr int bishop_value = 330;
constexpr int rook_value = 500;
constexpr int queen_value = 900;
constexpr int king_value = 20000;

constexpr std::array<int, 6> piece_values = {pawn_value, knight_value, bishop_value, rook_value, queen_value, king_value};

game_state fen_to_game_state(const std::string& fen) {
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
    bool color = (sub_fen_elements[1] == "w") ? true : false;

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

// evaluation
int evaluation(const game_state &state) {
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

float sigmoid(float x) {
    return 1 / (1 + std::exp(-x));
}

int main() {

    // initialize squared and absolute error
    std::vector<float> squared_error;
    std::vector<float> absolute_error;

    std::ifstream file("/home/yvlaere//projects/yvl-chess/NNUE_training/training_data/sf_training_data.csv");
    if (!file.is_open()) {
        std::cerr << "Could not open the file\n";
        return 1;
    }

    int counter = 0;
    std::string line;
    while (std::getline(file, line)) {


        std::vector<std::string> fields;
        std::stringstream ss(line);

        std::string field;
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }

        int pred = evaluation(fen_to_game_state(fields[0]));
        std::string sscore = fields[1];
        if (sscore.front() == '"' && sscore.back() == '"') {
            sscore = sscore.substr(1, sscore.size() - 2);
        }
        int score = std::stoi(sscore);

        // fix 32002 scores
        if (score == 32002) {
            score = 0;
        }

        // cp to wdl
        float wdl_score = sigmoid(score/400.0f);
        float wdl_pred = sigmoid(pred/400.0f);

        //std::cout << "wdl_score: " << wdl_score << " wdl_pred: " << wdl_pred << std::endl;
        //std::cout << "squared_error: " << (wdl_pred - wdl_score) * (wdl_pred - wdl_score) << std::endl;
        //std::cout << "absolute_error: " << std::abs(wdl_pred - wdl_score) << std::endl;

        squared_error.push_back((wdl_pred - wdl_score) * (wdl_pred - wdl_score));
        absolute_error.push_back(std::abs(wdl_pred - wdl_score));

        counter++;
        if (counter % 1000000 == 0) {
            std::cout << "Processed " << counter << " lines" << std::endl;
        }
    }
    
    file.close();

    // calculate mean squared error
    double mse = 0;
    for (const float& se : squared_error) {
        mse += se;
    }
    mse /= squared_error.size();
    std::cout << "Mean Squared Error: " << mse << std::endl;
    // calculate mean absolute error
    double mae = 0;
    for (const float& ae : absolute_error) {
        mae += ae;
    }
    mae /= absolute_error.size();
    std::cout << "Mean Absolute Error: " << mae << std::endl;

    return 0;
}