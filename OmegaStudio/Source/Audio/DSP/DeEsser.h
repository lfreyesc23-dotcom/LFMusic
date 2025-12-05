/**
 * @file DeEsser.h
 * @brief Professional de-esser for controlling sibilance
 * 
 * Features:
 * - Automatic sibilance frequency detection
 * - Multi-band sibilance reduction
 * - Listen mode for monitoring sibilance
 * - Variable frequency split
 * - Threshold and ratio control
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include "../../Utils/Constants.h"

namespace omega {

class ProDeEsser {
public:
    ProDeEsser();
    ~ProDeEsser() = default;
    
    void initialize(double sampleRate, int maxBlockSize);
    void process(float* buffer, int numSamples);
    void processStereo(float* leftBuffer, float* rightBuffer, int numSamples);
    void reset();
    
    // Parameters
    void setFrequency(float hz) { m_frequency = juce::jlimit(2000.0f, 12000.0f, hz); updateFilters(); }
    float getFrequency() const { return m_frequency; }
    
    void setThreshold(float dB) { m_threshold = juce::jlimit(-60.0f, 0.0f, dB); }
    float getThreshold() const { return m_threshold; }
    
    void setRatio(float ratio) { m_ratio = juce::jlimit(1.0f, 10.0f, ratio); }
    float getRatio() const { return m_ratio; }
    
    void setRange(float dB) { m_range = juce::jlimit(0.0f, 20.0f, dB); }
    float getRange() const { return m_range; }
    
    void setListenMode(bool listen) { m_listenMode = listen; }
    bool isListenMode() const { return m_listenMode; }
    
    void setAutoFrequency(bool enabled) { m_autoFrequency = enabled; }
    bool isAutoFrequencyEnabled() const { return m_autoFrequency; }
    
    float getGainReduction() const { return m_gainReduction; }
    
private:
    void updateFilters();
    float detectSibilance(const float* buffer, int numSamples);
    float calculateCompression(float level);
    
    juce::dsp::IIR::Filter<float> m_highPassFilter;
    juce::dsp::IIR::Filter<float> m_bandPassFilter;
    juce::dsp::IIR::Filter<float> m_lowPassFilter;
    
    float m_frequency { 6000.0f };
    float m_threshold { -20.0f };
    float m_ratio { 4.0f };
    float m_range { 12.0f };
    bool m_listenMode { false };
    bool m_autoFrequency { false };
    
    float m_gainReduction { 0.0f };
    double m_sampleRate { 48000.0 };
    
    juce::AudioBuffer<float> m_sibilanceBuffer;
};

} // namespace omega
