#include <iostream>
#include <array>
#include <random>
#include <algorithm>
#include <vector>

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

constexpr size_t INPUT_SIZE = 768;
constexpr size_t HIDDEN1_SIZE = 1024;
constexpr size_t HIDDEN2_SIZE = 8;
constexpr size_t HIDDEN3_SIZE = 32;
constexpr size_t OUTPUT_SIZE = 1;

// define NNUE components

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

// input functions
void game_state_to_input(const std::array<int, 64>& piece_on_square, std::vector<int>& active_features);
void refresh_accumulator(const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1, NNUE_accumulator& accumulator, const std::vector<int>& active_features, bool color);
void update_accumulator(const linear_layer<INPUT_SIZE, HIDDEN1_SIZE>& layer1, NNUE_accumulator& accumulator, const std::vector<int>& removed_features, const std::vector<int>& added_features, bool color);

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

// activation function
float* cReLu(int size, float* output, const float* input);

// actual evaluation
float nnue_evaluation(NNUE_accumulator& accumulator, const linear_layer<HIDDEN1_SIZE, HIDDEN2_SIZE>& layer2, const linear_layer<HIDDEN2_SIZE, HIDDEN3_SIZE>& layer3, const linear_layer<HIDDEN3_SIZE, OUTPUT_SIZE>& layer4);