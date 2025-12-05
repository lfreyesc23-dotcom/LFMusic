/*
  ==============================================================================

    AIVocalTuning.h
    Intelligent vocal pitch correction with vibrato preservation

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace omega {
namespace AI {

class AIVocalTuner {
public:
    AIVocalTuner();
    
    void initialize(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer);
    
    void setCorrection(float amount) { correction_ = amount; } // 0-1
    void setPreserveVibrato(bool preserve) { preserveVibrato_ = preserve; }
    void setScale(const std::vector<int>& scale) { scale_ = scale; }
    
    float getDetectedPitch() const { return detectedPitch_; }
    
private:
    float detectPitch(const juce::AudioBuffer<float>& buffer);
    float quantizeToScale(float pitch);
    
    double sampleRate_ = 48000.0;
    float correction_ = 1.0f;
    bool preserveVibrato_ = true;
    std::vector<int> scale_ = {0, 2, 4, 5, 7, 9, 11}; // Major
    float detectedPitch_ = 440.0f;
};

} // namespace AI
} // namespace omega
