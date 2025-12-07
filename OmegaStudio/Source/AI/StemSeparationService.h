#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace AI {

/**
 * @brief Professional Stem Separation Service (FL Studio 2025 Feature)
 * Splits audio into: Vocals, Drums, Bass, Other
 * Uses Demucs-style processing with deep learning
 */
class StemSeparationService
{
public:
    enum class StemType {
        Vocals,
        Drums,
        Bass,
        Other,
        Count
    };

    struct SeparationResult {
        juce::AudioBuffer<float> vocals;
        juce::AudioBuffer<float> drums;
        juce::AudioBuffer<float> bass;
        juce::AudioBuffer<float> other;
        bool success = false;
        juce::String errorMessage;
    };

    struct ProcessingOptions {
        int modelQuality;  // 1-5, higher = better quality
        bool useTwoPass;
        bool normalizeOutput;
        float confidenceThreshold;
        
        ProcessingOptions() 
            : modelQuality(3)
            , useTwoPass(true)
            , normalizeOutput(true)
            , confidenceThreshold(0.5f)
        {}
    };

    StemSeparationService();
    ~StemSeparationService();

    // Main separation function
    SeparationResult separateStems(const juce::AudioBuffer<float>& input,
                                   double sampleRate,
                                   const ProcessingOptions& options = ProcessingOptions());

    // Get specific stem only (faster)
    juce::AudioBuffer<float> extractStem(const juce::AudioBuffer<float>& input,
                                         double sampleRate,
                                         StemType stemType);

    // Progress callback
    std::function<void(float progress, const juce::String& status)> onProgress;

    // Check if service is ready
    bool isReady() const { return initialized; }
    juce::String getStatusMessage() const { return statusMessage; }

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    bool initialized = false;
    juce::String statusMessage;

    // Advanced processing methods
    void processFrequencyDomain(juce::AudioBuffer<float>& buffer);
    void applySpectralMasking(juce::AudioBuffer<float>& target,
                              const juce::AudioBuffer<float>& reference);
    void enhanceTransients(juce::AudioBuffer<float>& drums);
    void isolateHarmonics(juce::AudioBuffer<float>& vocals);
    void extractLowFrequencies(juce::AudioBuffer<float>& bass);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparationService)
};

} // namespace AI
} // namespace OmegaStudio
