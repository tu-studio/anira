/* ==========================================================================

Minimal LibTorch example from https://pytorch.org/tutorials/advanced/cpp_export.html
Licence: modified BSD

========================================================================== */

#include <torch/script.h>
#include <torch/torch.h>
#include <iostream>
#include <memory>

#include "../../../../extras/desktop/models/stateful-rnn/StatefulRNNConfig.h"
#include "../../../../extras/desktop/models/hybrid-nn/HybridNNConfig.h"
#include "../../../../extras/desktop/models/cnn/CNNConfig.h"

void minimal_inference(anira::InferenceConfig config) {
    std::cout << "Minimal LibTorch example:" << std::endl;
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Using model: " << config.m_model_path_torch << std::endl;

    std::string omp_num_threads = "OMP_NUM_THREADS=1";
    std::string mkl_num_threads = "MKL_NUM_THREADS=1";

#if WIN32
    _putenv(omp_num_threads.data());
    _putenv(mkl_num_threads.data());
#else
    putenv(omp_num_threads.data());
    putenv(mkl_num_threads.data());
#endif

    // Load model
    torch::jit::Module module;
    try {
        module = torch::jit::load(config.m_model_path_torch);
    }
    catch (const c10::Error& e) {
        std::cerr << "error loading the model\n";
        std::cout << e.what() << std::endl;
    }

    // Fill input tensor with data
    std::vector<float> inputData;
    for (int i = 0; i < config.m_new_model_input_size; i++) {
        inputData.push_back(i * 0.000001f);
    }

    // Create input tensor object from input data values and reshape
    std::vector<int64_t> shape = config.m_model_input_shape_torch;
    torch::Tensor inputTensor = torch::from_blob(inputData.data(), shape);

    for (int i = 0; i < inputTensor.sizes().size(); ++i) {
        std::cout << "Input shape " << i << ": " << inputTensor.sizes()[i] << '\n';
    }

    // Create IValue vector for input of interpreter
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(inputTensor);

    // Execute inference
    torch::Tensor outputTensor = module.forward(inputs).toTensor();

    for (int i = 0; i < outputTensor.sizes().size(); ++i) {
        std::cout << "Output shape " << i << ": " << outputTensor.sizes()[i] << '\n';
    }

    // Flatten the output tensor
    outputTensor = outputTensor.view({-1});

    std::vector<float> outputData;

    // Copy the data to the outputData vector
    for (int i = 0; i < config.m_new_model_output_size; ++i) {
        outputData.push_back(outputTensor[i].item().toFloat());
        std::cout << "Output data [" << i << "]: " << outputData[i] << std::endl;
    }
}

int main(int argc, const char* argv[]) {

    std::vector<anira::InferenceConfig> modelsToInference = {hybridNNConfig, cnnConfig, statefulRNNConfig};

    for (int i = 0; i < modelsToInference.size(); ++i) {
        minimal_inference(modelsToInference[i]);
    }

    return 0;
}