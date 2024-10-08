#ifndef ANIRA_BENCHMARK_PROCESSBLOCKFIXTURE_H
#define ANIRA_BENCHMARK_PROCESSBLOCKFIXTURE_H

#include <benchmark/benchmark.h>
#include "../anira.h"
#include "helperFunctions.h"

namespace anira {
namespace benchmark {

class ANIRA_API ProcessBlockFixture : public ::benchmark::Fixture {
public:

    ProcessBlockFixture();
    ~ProcessBlockFixture();

    void initializeIteration();
    void initializeRepetition(const InferenceConfig& inferenceConfig, const HostAudioConfig& hostAudioConfig, const InferenceBackend& inferenceBackend, bool sleep_after_repetition = true);
    bool bufferHasBeenProcessed();
    void pushRandomSamplesInBuffer(anira::HostAudioConfig hostAudioConfig);
    int getBufferSize();
    int getRepetition();

#if defined(_WIN32) || defined(__APPLE__)
        void interationStep(const std::chrono::steady_clock::time_point& start, const std::chrono::steady_clock::time_point& end, ::benchmark::State& state);
#else
        void interationStep(const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end, ::benchmark::State& state);
#endif

    void repetitionStep();

    inline static std::unique_ptr<anira::InferenceHandler> m_inferenceHandler = nullptr;
    inline static std::unique_ptr<anira::AudioBuffer<float>> m_buffer = nullptr;

private:
    int m_bufferSize = 0;
    int m_repetition = 0;
    bool m_sleep_after_repetition = true;
    int m_iteration = 0;
    std::chrono::duration<double, std::milli> m_runtime_last_repetition = std::chrono::duration<double, std::milli>(0);
    int m_prev_num_received_samples = 0;
    std::string m_model_name;
    std::string m_inference_backend_name;
    InferenceBackend m_inferenceBackend;
    InferenceConfig m_inferenceConfig;
    HostAudioConfig m_hostAudioConfig;

    void SetUp(const ::benchmark::State& state);
    void TearDown(const ::benchmark::State& state);
};

} // namespace benchmark
} // namespace anira

#endif // ANIRA_BENCHMARK_PROCESSBLOCKFIXTURE_H