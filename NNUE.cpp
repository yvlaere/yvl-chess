#include <iostream>
#include <array>
#include <random>
#include <algorithm>

constexpr size_t INPUT_SIZE  = 768;
constexpr size_t HIDDEN1_SIZE  =   8;
constexpr size_t HIDDEN2_SIZE  =   8;
constexpr size_t OUTPUT_SIZE   =   1;

void game_state_to_input(const std::array<int, 64>& piece_on_square, std::array<bool, INPUT_SIZE>& input_layer) {
    // convert the game state to a NN input

    // iterate over all pieces
    for (int i = 0; i < 64; i++) {
        input_layer[i*piece_on_square[i]] = true;
    }
}

// start with a very basic NNUE implementation
// this is a simple feedforward neural network with 3 layers
// Layer 1: 768 → 8
// Layer 2: 8 → 8
// Layer 3: 8 → 1
// The input is a sparse vector of size 768 (0 or 1 values)
// The output is a single value (the evaluation of the position)
// The weights and biases are initialized randomly
// The activation function is ReLU (Rectified Linear Unit)
// The forward pass is implemented as a series of matrix multiplications
// and element-wise operations

struct NNUE {
    // Layer 1: 768 → 8
    std::array<std::array<float, HIDDEN1_SIZE>, INPUT_SIZE> weights_1;
    std::array<float, HIDDEN1_SIZE> biases_1;

    // Layer 2: 8 → 8
    std::array<std::array<float, HIDDEN2_SIZE>, HIDDEN1_SIZE> weights_2;
    std::array<float, HIDDEN2_SIZE> biases_2;

    // Layer 3: 8 → 1
    std::array<float, HIDDEN2_SIZE> weights_3;
    float biases_3;

    // Activation (ReLU)
    static float relu(float x) { return std::max(0.0f, x); }

    // Forward pass: x is a sparse vector of size 768 (0 or 1 values)
    float forward(const std::array<bool, INPUT_SIZE>& input_layer) const {
        
        // Layer1
        std::array<float, HIDDEN1_SIZE> hidden_layer_1;
        for (size_t j = 0; j < HIDDEN1_SIZE; ++j) {
            float sum = biases_1[j];
            for (size_t i = 0; i < INPUT_SIZE; ++i)
                sum += input_layer[i] * weights_1[i][j];
            hidden_layer_1[j] = relu(sum);
        }

        // Layer2
        std::array<float, HIDDEN2_SIZE> hidden_layer_2;
        for (size_t j = 0; j < HIDDEN2_SIZE; ++j) {
            float sum = biases_2[j];
            for (size_t i = 0; i < HIDDEN1_SIZE; ++i)
                sum += hidden_layer_1[i] * weights_2[i][j];
            hidden_layer_2[j] = relu(sum);
        }
        
        // Layer3 → scalar
        float out = biases_3;
        for (size_t i = 0; i < HIDDEN2_SIZE; ++i)
            out += hidden_layer_2[i] * weights_3[i];
        return out;
    }

    // Utility to initialize weights/biases randomly (for testing)
    void randomize() {
        std::mt19937 rng{1234};
        std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
        for (auto& row : weights_1) for (auto& w : row) w = dist(rng);
        for (auto& b : biases_1) b = dist(rng);
        for (auto& row : weights_2) for (auto& w : row) w = dist(rng);
        for (auto& b : biases_2) b = dist(rng);
        for (auto& w : weights_3) w = dist(rng);
        biases_3 = dist(rng);
    }
};

int main() {
    // Initialize the neural network
    NNUE nnue;
    nnue.randomize();

    // Example game state (piece on square)
    std::array<int, 64> piece_on_square = {0}; // Initialize with zeros
    piece_on_square[0] = 1; // Example: a piece on square 0

    // create input layer
    std::array<bool, INPUT_SIZE> input_layer = {false};

    // Convert game state to input
    game_state_to_input(piece_on_square, input_layer);

    // Forward pass
    float output = nnue.forward(input_layer);
    
    std::cout << "NNUE output: " << output << std::endl;

    return 0;
}