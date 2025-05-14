#include <iostream>
#include <array>
#include <random>
#include <algorithm>
#include <vector>

constexpr size_t INPUT_SIZE = 768;
constexpr size_t HIDDEN1_SIZE = 1024;
constexpr size_t HIDDEN2_SIZE = 8;
constexpr size_t HIDDEN3_SIZE = 32;
constexpr size_t OUTPUT_SIZE = 1;

void game_state_to_input(const std::array<int, 64>& piece_on_square, std::vector<int>& active_features) {
    // convert the game state to a NN input

    // iterate over all pieces
    for (int i = 0; i < 64; i++) {
        if (piece_on_square[i] != 0) {
            active_features.push_back(piece_on_square[i]*i);
        }
    }
}

// start with a very basic NNUE implementation
// this is a simple feedforward neural network with 4 layers
// Layer 1: 768 → 1024
// Layer 2: 1024 → 8
// Layer 3: 8 → 32
// Layer 4: 32 → 1
// The input is a sparse vector of size 768 (0 or 1 values)
// The output is a single value (the evaluation of the position)
// The weights and biases are initialized randomly
// The activation function is clipped ReLU (Rectified Linear Unit)
// The forward pass is implemented as a series of matrix multiplications
// and element-wise operations

// The accumulator is the *output* of the first hidden layer, it is what gets efficiently updated
struct NNUE_accumulator {
    std::array<float, HIDDEN1_SIZE> values;
};

// Linear layers have weights and biases
template <size_t input_size, size_t output_size>
struct linear_layer {
    std::array<std::array<float, output_size>, input_size> weights;
    std::array<float, output_size> biases;
};

// Compute accumulator from scratch
void refresh_accumulator(const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1, NNUE_accumulator& accumulator, const std::vector<int>& active_features, bool color) {
    
    // add the biases to the accumulator
    for (int i = 0; i < HIDDEN1_SIZE; i++) {
        accumulator.values[i] += layer1.biases[i];
    }

    // add the weights of active features to the accumulator
    // because features are binary, we can just add the weights of the active features without multiplying
    for (int af : active_features) {
        for (int i = 0; i < HIDDEN1_SIZE; i++) {
            accumulator.values[i] += layer1.weights[af][i];
        }
    }
}

// Update the accumulator
void update_accumulator(const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer, NNUE_accumulator& accumulator, const std::vector<int>& removed_features, const std::vector<int>& added_features, bool color) {

    // remove the weights of removed features to the accumulator
    for (int rf : removed_features) {
        for (int i = 0; i < HIDDEN1_SIZE; i++) {
            accumulator.values[i] -= layer.weights[rf][i];
        }
    }
    
    // add the weights of active features to the accumulator
    for (int af : added_features) {
        for (int i = 0; i < HIDDEN1_SIZE; i++) {
            accumulator.values[i] += layer.weights[af][i];
        }
    }
}

// linear layer forward pass
template <size_t input_size, size_t output_size>
float* linear_layer_forward(const linear_layer<input_size, output_size>& layer, float* output, const float* input) {
    
    // copy biases to output
    for (int i = 0; i < output_size; i++) {
        output[i] = layer.biases[i];
    }

    // multiply input by weights and add to output
    for (int i = 0; i < input_size; i++) {
        for (int j = 0; j < output_size; j++) {
            output[i] += layer.weights[i][j] * input[i];
        }
    }

    return output + output_size;
}

float* cReLu(int size, float* output, const float* input) {
    
    // apply clipped ReLU activation function
    for (int i = 0; i < size; i++) {
        output[i] = std::min(std::max(0.0f, input[i]), 1.0f);
    }

    return output + size;
}

float nnue_evaluation(NNUE_accumulator& accumulator, const linear_layer<HIDDEN1_SIZE, HIDDEN2_SIZE>& layer2, const linear_layer<HIDDEN2_SIZE, HIDDEN3_SIZE>& layer3, const linear_layer<HIDDEN3_SIZE, OUTPUT_SIZE>& layer4) {

    // create a buffer that can fit the largest layer twice
    // so it needs to be max(HIDDEN1_SIZE, HIDDEN2_SIZE, HIDDEN3_SIZE, OUTPUT_SIZE) × 2
    // this is faster than std::vector because it presents dynamic allocation overhead
    float buffer[2048];
    float* curr_output = buffer;
    float* curr_input = accumulator.values.data();
    float* next_output;

    // cReLu after accumulator
    next_output = cReLu(HIDDEN1_SIZE, curr_output, curr_input);
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
    // linear layer 3
    next_output = linear_layer_forward(layer3, curr_output, curr_input);
    curr_input = curr_output;
    curr_output = next_output;
    // cReLu after layer 3
    next_output = cReLu(HIDDEN3_SIZE, curr_output, curr_input);
    curr_input = curr_output;
    curr_output = next_output;
    // linear layer 4
    next_output = linear_layer_forward(layer4, curr_output, curr_input);
    curr_input = curr_output;
    curr_output = next_output;


    return *curr_output;

}

int main() {

    // Example game state (piece on square)
    std::array<int, 64> piece_on_square = {0}; // Initialize with zeros
    piece_on_square[0] = 1; // Example: a piece on square 0

    // random linear layer weights and biases
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    linear_layer<INPUT_SIZE, HIDDEN1_SIZE> layer1;
    for (int i = 0; i < INPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN1_SIZE; j++) {
            layer1.weights[i][j] = dis(gen);
        }
    }
    for (int i = 0; i < HIDDEN1_SIZE; i++) {
        layer1.biases[i] = dis(gen);
    }
    linear_layer<HIDDEN1_SIZE, HIDDEN2_SIZE> layer2;
    for (int i = 0; i < HIDDEN1_SIZE; i++) {
        for (int j = 0; j < HIDDEN2_SIZE; j++) {
            layer2.weights[i][j] = dis(gen);
        }
    }
    for (int i = 0; i < HIDDEN2_SIZE; i++) {
        layer2.biases[i] = dis(gen);
    }
    linear_layer<HIDDEN2_SIZE, HIDDEN3_SIZE> layer3;
    for (int i = 0; i < HIDDEN2_SIZE; i++) {
        for (int j = 0; j < HIDDEN3_SIZE; j++) {
            layer3.weights[i][j] = dis(gen);
        }
    }
    for (int i = 0; i < HIDDEN3_SIZE; i++) {
        layer3.biases[i] = dis(gen);
    }
    linear_layer<HIDDEN3_SIZE, OUTPUT_SIZE> layer4;
    for (int i = 0; i < HIDDEN3_SIZE; i++) {
        for (int j = 0; j < OUTPUT_SIZE; j++) {
            layer4.weights[i][j] = dis(gen);
        }
    }
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        layer4.biases[i] = dis(gen);
    }

    // input to accumulator
    NNUE_accumulator accumulator;
    std::vector<int> active_features;
    game_state_to_input(piece_on_square, active_features);
    refresh_accumulator(layer1, accumulator, active_features, true);

    float eval = nnue_evaluation(accumulator, layer2, layer3, layer4);
    
    std::cout << "NNUE output: " << eval << std::endl;

    return 0;
}