#include <anira/scheduler/SessionElement.h>

namespace anira {

SessionElement::SessionElement(int newSessionID, PrePostProcessor& ppP, InferenceConfig& config, BackendBase& noneProcessor) :
    sessionID(newSessionID),
    prePostProcessor(ppP),
    inferenceConfig(config),
    noneProcessor(noneProcessor)
{
}

    SessionElement::ThreadSafeStruct::ThreadSafeStruct(size_t model_input_size,
                                                       size_t model_output_size) {
        processedModelInput.initialize(1, model_input_size);
        rawModelOutput.initialize(1, model_output_size);
    }

    void SessionElement::clear() {
        sendBuffer.clearWithPositions();
        receiveBuffer.clearWithPositions();

#ifdef USE_SEMAPHORE
        while (m_session_counter.try_acquire()) {
            // Nothing to do here, just reducing count
        }
#else
        m_session_counter.store(0);
#endif

        timeStamps.clear();
        inferenceQueue.clear();
    }

    void SessionElement::prepare(HostAudioConfig newConfig) {
        sendBuffer.initializeWithPositions(1, (size_t) newConfig.hostSampleRate * 50); // TODO find appropriate size dynamically
        receiveBuffer.initializeWithPositions(1, (size_t) newConfig.hostSampleRate * 50); // TODO find appropriate size dynamically

        size_t max_inference_time_in_samples = (size_t) std::ceil(inferenceConfig.m_max_inference_time * newConfig.hostSampleRate / 1000);

        // We assume that the model_output_size gives us the amount of new samples we can write into the buffer for each bath.
        float structs_per_buffer = std::ceil((float) newConfig.hostBufferSize / (float) inferenceConfig.m_new_model_output_size);
        float structs_per_max_inference_time = std::ceil((float) max_inference_time_in_samples / (float) inferenceConfig.m_new_model_output_size);
        // ceil to full buffers
        structs_per_max_inference_time = std::ceil(structs_per_max_inference_time/structs_per_buffer) * structs_per_buffer;
        // we can have multiple max_inference_times per buffer
        float max_inference_times_per_buffer = std::max(std::floor((float) newConfig.hostBufferSize / (float) (max_inference_time_in_samples)), 1.f);
        // minimum number of structs necessary to keep available inference queues where the ringbuffer can push to if we have n_free_threads > structs_per_buffer
        // int n_structs = (int) (structs_per_buffer + structs_per_max_inference_time);
        // but because we can have multiple instances (sessions) that use the same threadpool, we have to multiply structs_per_max_inference_time with the struct_per_buffer
        // because each struct can take max_inference_time time to process and be free again
        int n_structs = (int) (structs_per_buffer + structs_per_max_inference_time * std::ceil(structs_per_buffer/max_inference_times_per_buffer));

        // factor 4 to encounter the case where we have missing samples because the max_inference_time was calculated not correctly
        n_structs *= 1; // TODO: before deployment we have to change this to 4

        for (int i = 0; i < n_structs; ++i) {
            inferenceQueue.emplace_back(std::make_unique<ThreadSafeStruct>(inferenceConfig.m_new_model_input_size, inferenceConfig.m_new_model_output_size));
        }

        timeStamps.reserve(n_structs);
    }

} // namespace anira