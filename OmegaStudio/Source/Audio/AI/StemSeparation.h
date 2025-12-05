/*
  ==============================================================================

    StemSeparation.h
    AI-powered stem separation (vocals/drums/bass/other)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace omega {
namespace AI {

enum class StemType {
    Vocals,
    Drums,
    Bass,
    Guitar,
    Keys,
    Other
};

enum class QualityMode {
    Fast,      // RT 0.5x
    Balanced,  // RT 2x
    Best       // RT 5x
};

class StemSeparator {
public:
    StemSeparator();
    
    bool separateStems(const juce::AudioBuffer<float>& input, 
                      std::map<StemType, juce::AudioBuffer<float>>& outputs);
    
    void setQualityMode(QualityMode mode) { qualityMode_ = mode; }
    float getProgress() const { return progress_; }
    
private:
    void processFFTSeparation(const juce::AudioBuffer<float>& input,
                             juce::AudioBuffer<float>& output,
                             float lowCutoff, float highCutoff);
    
    QualityMode qualityMode_ = QualityMode::Balanced;
    float progress_ = 0.0f;
};

} // namespace AI
} // namespace omega
