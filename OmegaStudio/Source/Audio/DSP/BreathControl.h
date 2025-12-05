/**
 * @file BreathControl.h
 * @brief Breath detection and noise reduction for vocals
 */

#pragma once

#include <JuceHeader.h>
#include "../../Utils/Constants.h"

namespace omega {

class BreathControl {
public:
    BreathControl();
    ~BreathControl() = default;
    
    void initialize(double sampleRate, int maxBlockSize);
    void process(float* buffer, int numSamples);
    void reset();
    
    // Parameters
    void setThreshold(float dB) { m_threshold = juce::jlimit(-80.0f, -20.0f, dB); }
    float getThreshold() const { return m_threshold; }
    
    void setReduction(float dB) { m_reduction = juce::jlimit(0.0f, 60.0f, dB); }
    float getReduction() const { return m_reduction; }
    
    void setAttack(float ms) { m_attack = juce::jlimit(0.1f, 100.0f, ms); updateEnvelope(); }
    float getAttack() const { return m_attack; }
    
    void setRelease(float ms) { m_release = juce::jlimit(10.0f, 1000.0f, ms); updateEnvelope(); }
    float getRelease() const { return m_release; }
    
    void setNoiseReductionAmount(float amount) { m_noiseReduction = juce::jlimit(0.0f, 1.0f, amount); }
    float getNoiseReductionAmount() const { return m_noiseReduction; }
    
private:
    void updateEnvelope();
    bool isBreath(const float* buffer, int numSamples);
    
    float m_threshold { -40.0f };
    float m_reduction { 12.0f };
    float m_attack { 5.0f };
    float m_release { 100.0f };
    float m_noiseReduction { 0.5f };
    
    float m_envelopeGain { 1.0f };
    float m_attackCoeff { 0.0f };
    float m_releaseCoeff { 0.0f };
    
    double m_sampleRate { 48000.0 };
};

} // namespace omega
