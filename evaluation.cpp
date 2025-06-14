#include "evaluation.h"

void game_state_to_input(const std::array<int, 64>& piece_on_square, std::vector<int>& active_features_w, std::vector<int>& active_features_b) {
    // convert the game state to a NN input

    // iterate over all pieces
    for (int i = 0; i < 64; i++) {
        if (piece_on_square[i] != -1) {
            int alternative_position = 63 - (i ^ 7);
            int alternative_piece = piece_on_square[i] < 6 ? piece_on_square[i] + 6 : piece_on_square[i] - 6;
            active_features_w.push_back(piece_on_square[i]*64 + i);
            active_features_b.push_back(alternative_piece*64 + alternative_position);
        }
    }
}

// Compute accumulator from scratch
void refresh_accumulator(const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1, NNUE_accumulator& accumulator, const std::vector<int>& active_features, bool color) {
    
    // add the biases to the accumulator
    for (int i = 0; i < HIDDEN1_SIZE; i++) {
        accumulator.values[color][i] = layer1.biases[i];
    }

    // add the weights of active features to the accumulator
    // because features are binary, we can just add the weights of the active features without multiplying
    for (int af : active_features) {
        for (int i = 0; i < HIDDEN1_SIZE; i++) {
            accumulator.values[color][i] += layer1.weights[af][i];
        }
    }
}

// Update the accumulator
void update_accumulator(const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1, NNUE_accumulator& accumulator, const std::vector<int>& removed_features, const std::vector<int>& added_features, bool color) {

    // remove the weights of removed features to the accumulator
    for (int rf : removed_features) {
        for (int i = 0; i < HIDDEN1_SIZE; i++) {
            accumulator.values[color][i] -= layer1.weights[rf][i];
        }
    }
    
    // add the weights of active features to the accumulator
    for (int af : added_features) {
        for (int i = 0; i < HIDDEN1_SIZE; i++) {
            accumulator.values[color][i] += layer1.weights[af][i];
        }
    }
}

// clipped ReLU activation function
float* cReLu(int size, float* output, const float* input) {
    
    // apply clipped ReLU activation function
    for (int i = 0; i < size; i++) {
        output[i] = std::min(std::max(0.0f, input[i]), 1.0f);
    }

    return output + size;
}

float nnue_evaluation(NNUE_accumulator& accumulator, const linear_layer<HIDDEN1_SIZE*2, HIDDEN2_SIZE>& layer2, const linear_layer<HIDDEN2_SIZE, OUTPUT_SIZE>& layer3, bool color) {

    // create a buffer that can fit the largest layer twice
    // so it needs to be max(HIDDEN1_SIZE, HIDDEN2_SIZE, OUTPUT_SIZE) × 2
    // this is faster than std::vector because it presents dynamic allocation overhead
    float buffer[1024];

    std::array<float, 2*HIDDEN1_SIZE> input;
    bool stm = color;
    for (int i = 0; i < HIDDEN1_SIZE; ++i) {
        input[i] = accumulator[stm][i];
        input[HIDDEN1_SIZE + i] = accumulator[!stm][i];
    }

    float* curr_output = buffer;
    float* curr_input = input.data();
    float* next_output;

    // cReLu after accumulator
    next_output = cReLu(2*HIDDEN1_SIZE, curr_output, curr_input);
    curr_input = curr_output;
    curr_output = next_output;

    // linear layer 2
    next_output = linear_layer_forward(layer2, curr_output, curr_input);
    curr_input = curr_output;
    curr_output = next_output;

    // cReLu after layer 2
    next_output = cReLu(HIDDEN2_SIZE, curr_output, curr_input);
    curr_input = curr_output;
    curr_output = next_output;

    // linear layer 4
    next_output = linear_layer_forward(layer3, curr_output, curr_input);

    return *curr_output;

}