#pragma once

#include <JuceHeader.h>
#include "AIJobQueue.h"
#include <functional>
#include <atomic>

namespace omega {
namespace AI {

enum class DenoiseQuality {
    Fast,
    Balanced,
    HighQuality
};

struct DenoiseConfig {
    double sampleRate = 44100.0;
    DenoiseQuality quality = DenoiseQuality::Balanced;
    float reductionAmount = 0.8f; // 0.0 - 1.0
    bool preserveTransients = true;
    bool adaptiveMode = true;
    juce::File modelPath;
};

struct DenoiseResult {
    juce::AudioBuffer<float> denoisedAudio;
    float noiseReductionDb = 0.0f;
    float processingTimeMs = 0.0f;
    bool success = false;
    juce::String errorMessage;
};

class DenoiseService {
public:
    DenoiseService();
    ~DenoiseService();

    void setConfig(const DenoiseConfig& config) { config_ = config; }
    void setModelPath(const juce::File& path) { config_.modelPath = path; }
    
    // Synchronous processing
    DenoiseResult processAudio(const juce::AudioBuffer<float>& input);
    
    // Asynchronous processing with callback
    void processAudioAsync(const juce::AudioBuffer<float>& input,
                          std::function<void(DenoiseResult)> callback);
    
    // Cancel ongoing operations
    void cancelAll();
    
    bool isProcessing() const { return isProcessing_.load(); }
    
private:
    DenoiseConfig config_;
    std::atomic<bool> isProcessing_{false};
    std::unique_ptr<AIJobQueue> jobQueue_;
    
    // RNNoise-based denoising algorithm
    DenoiseResult applyRNNoise(const juce::AudioBuffer<float>& input);
    
    // Spectral subtraction fallback
    DenoiseResult applySpectralSubtraction(const juce::AudioBuffer<float>& input);
    
    // Artifact reduction post-processing
    void reduceArtifacts(juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DenoiseService)
};

} // namespace AI
} // namespace omega
