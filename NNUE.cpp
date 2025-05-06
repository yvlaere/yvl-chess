#include "uci.cpp"

std::array<bool, 768> input_to_nnue = {false};

const size_t input_size = 768;
const size_t hidden_layer_size = 1024;

struct network {
    std::array<std::array<bool, hidden_layer_size>, input_size> accumulator_weights;
    std::array<int, hidden_layer_size> accumulator_biases;
    std::array<int, hidden_layer_size*2> output_weights;
    int output_bias;
};

void game_state_to_input(const std::array<int, 64>& piece_on_square) {
    // convert the game state to a NN input

    // iterate over all pieces
    for (int i = 0; i < 64; i++) {
        input_to_nnue[i*piece_on_square[i]] = true;
    }
}

