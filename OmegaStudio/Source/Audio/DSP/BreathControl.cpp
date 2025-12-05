/**
 * @file BreathControl.cpp
 */

#include "BreathControl.h"
#include <cmath>

namespace omega {

BreathControl::BreathControl() {
}

void BreathControl::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    updateEnvelope();
}

void BreathControl::process(float* buffer, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float inputLevel = std::abs(buffer[i]);
        float inputDB = juce::Decibels::gainToDecibels(inputLevel + 0.0001f);
        
        // Detect if current sample is breath/noise
        bool isBelowThreshold = inputDB < m_threshold;
        
        // Update envelope
        float targetGain = isBelowThreshold ? 
            juce::Decibels::decibelsToGain(-m_reduction * m_noiseReduction) : 1.0f;
        
        float coeff = (targetGain < m_envelopeGain) ? m_attackCoeff : m_releaseCoeff;
        m_envelopeGain += coeff * (targetGain - m_envelopeGain);
        
        // Apply gain
        buffer[i] *= m_envelopeGain;
    }
}

void BreathControl::reset() {
    m_envelopeGain = 1.0f;
}

void BreathControl::updateEnvelope() {
    m_attackCoeff = 1.0f - std::exp(-1.0f / (m_attack * 0.001f * m_sampleRate));
    m_releaseCoeff = 1.0f - std::exp(-1.0f / (m_release * 0.001f * m_sampleRate));
}

bool BreathControl::isBreath(const float* buffer, int numSamples) {
    float rms = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        rms += buffer[i] * buffer[i];
    }
    rms = std::sqrt(rms / numSamples);
    
    float rmsDB = juce::Decibels::gainToDecibels(rms + 0.0001f);
    return rmsDB < m_threshold;
}

} // namespace omega
