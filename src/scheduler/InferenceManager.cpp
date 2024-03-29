#include <anira/scheduler/InferenceManager.h>

namespace anira {

InferenceManager::InferenceManager(PrePostProcessor& ppP, InferenceConfig& config, BackendBase& noneProcessor) :
    inferenceThreadPool(InferenceThreadPool::getInstance(config)),
    session(inferenceThreadPool->createSession(ppP, config, noneProcessor)),
    inferenceConfig(config)
{
}

InferenceManager::~InferenceManager() {
    inferenceThreadPool->releaseSession(session, inferenceConfig);
}

void InferenceManager::setBackend(InferenceBackend newInferenceBackend) {
    session.currentBackend = newInferenceBackend;
}

InferenceBackend InferenceManager::getBackend() {
    return session.currentBackend;
}

void InferenceManager::prepare(HostAudioConfig newConfig) {
    spec = newConfig;

    inferenceThreadPool->prepare(session, spec);

    inferenceCounter = 0;

    bufferCount = 0;

    size_t max_inference_time_in_samples = (size_t) std::ceil(inferenceConfig.m_max_inference_time * spec.hostSampleRate / 1000);

    float divisor = (float) spec.hostBufferSize / (float) inferenceConfig.m_batch_size * (float) inferenceConfig.m_model_output_size;
    size_t remainder = spec.hostBufferSize % (inferenceConfig.m_batch_size * inferenceConfig.m_model_output_size);

    if (remainder == 0) {
        initSamples = (size_t) divisor * max_inference_time_in_samples + (size_t) divisor * inferenceConfig.m_model_latency;
    } else if (remainder > 0 && remainder < spec.hostBufferSize) {
        initSamples = ((size_t) divisor + 1) * max_inference_time_in_samples + ((size_t) divisor + 1) * inferenceConfig.m_model_latency + spec.hostBufferSize; //TODO not minimum possible
    } else {
        initSamples = max_inference_time_in_samples + (inferenceConfig.m_batch_size * inferenceConfig.m_model_output_size) + inferenceConfig.m_model_latency; //TODO not minimum possible
    }

    if ((float) initSamples < inferenceConfig.m_wait_in_process_block * (float) spec.hostBufferSize) {
        init = false;
    } else {
        init = true;
    }
}

void InferenceManager::process(float ** inputBuffer, size_t inputSamples) {
    processInput(inputBuffer, inputSamples);

    inferenceThreadPool->newDataSubmitted(session);
    double timeInSec = static_cast<double>(inputSamples) / spec.hostSampleRate;
    inferenceThreadPool->newDataRequest(session, timeInSec);

    if (init) {
        bufferCount += inputSamples;
        clearBuffer(inputBuffer, inputSamples);
        if (bufferCount >= initSamples) init = false;
    } else {
        processOutput(inputBuffer, inputSamples);
    }
}

void InferenceManager::processInput(float ** inputBuffer, size_t inputSamples) {
    for (size_t channel = 0; channel < spec.hostChannels; ++channel) {
        for (size_t sample = 0; sample < inputSamples; ++sample) {
            session.sendBuffer.pushSample(0, inputBuffer[channel][sample]);
        }
    }
}

void InferenceManager::processOutput(float ** inputBuffer, size_t inputSamples) {    
    while (inferenceCounter > 0) {
        if (session.receiveBuffer.getAvailableSamples(0) >= 2 * (size_t) inputSamples) {
            for (size_t channel = 0; channel < spec.hostChannels; ++channel) {
                for (size_t sample = 0; sample < inputSamples; ++sample) {
                    session.receiveBuffer.popSample(channel);
                }
            }
            inferenceCounter--;
            std::cout << "##### catch up samples" << std::endl;
        }
        else {
            break;
        }
    }
    if (session.receiveBuffer.getAvailableSamples(0) >= (size_t) inputSamples) {
        for (size_t channel = 0; channel < spec.hostChannels; ++channel) {
            for (size_t sample = 0; sample < inputSamples; ++sample) {
                inputBuffer[channel][sample] = session.receiveBuffer.popSample(channel);
            }
        }
    }
    else {
        clearBuffer(inputBuffer, inputSamples);
        inferenceCounter++;
        std::cout << "##### missing samples" << std::endl;
    }
}

void InferenceManager::clearBuffer(float ** inputBuffer, size_t inputSamples) {
    for (size_t channel = 0; channel < spec.hostChannels; ++channel) {
        for (size_t sample = 0; sample < inputSamples; ++sample) {
            inputBuffer[channel][sample] = 0.f;
        }
    }
}

int InferenceManager::getLatency() const {
    if ((int) initSamples % (int) spec.hostBufferSize == 0) return initSamples;
    else return ((int) ((float) initSamples / (float) spec.hostBufferSize) + 1) * (int) spec.hostBufferSize;
}

InferenceThreadPool& InferenceManager::getInferenceThreadPool() {
    return *inferenceThreadPool;
}

size_t InferenceManager::getNumReceivedSamples() {
    inferenceThreadPool->newDataRequest(session, 0); // TODO: Check if processOutput call is better here
    return session.receiveBuffer.getAvailableSamples(0);
}

bool InferenceManager::isInitializing() const {
    return init;
}

int InferenceManager::getMissingBlocks() {
    return inferenceCounter.load();
}

int InferenceManager::getSessionID() const {
    return session.sessionID;
}

} // namespace anira