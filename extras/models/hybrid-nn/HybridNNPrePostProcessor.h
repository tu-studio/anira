#ifndef ANIRA_HYBRIDNNPREPOSTPROCESSOR_H
#define ANIRA_HYBRIDNNPREPOSTPROCESSOR_H

#include "HybridNNConfig.h"
#include <anira/anira.h>

class HybridNNPrePostProcessor : public anira::PrePostProcessor
{
public:
    virtual void preProcess(anira::RingBuffer& input, anira::AudioBufferF& output, [[maybe_unused]] anira::InferenceBackend currentInferenceBackend) override {
        for (size_t batch = 0; batch < config.m_batch_size; batch++) {
            size_t baseIdx = batch * config.m_model_input_size;
            popSamplesFromBuffer(input, output, config.m_model_output_size, config.m_model_input_size-config.m_model_output_size, baseIdx);
        }
    };
    
    anira::InferenceConfig config = hybridNNConfig;
};

#endif //ANIRA_HYBRIDNNPREPOSTPROCESSOR_H
