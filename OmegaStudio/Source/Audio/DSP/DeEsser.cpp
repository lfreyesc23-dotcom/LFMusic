/**
 * @file DeEsser.cpp
 */

#include "DeEsser.h"
#include <cmath>

namespace omega {

ProDeEsser::ProDeEsser() {
}

void ProDeEsser::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_sibilanceBuffer.setSize(1, maxBlockSize);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maxBlockSize;
    spec.numChannels = 1;
    
    m_highPassFilter.prepare(spec);
    m_bandPassFilter.prepare(spec);
    m_lowPassFilter.prepare(spec);
    
    updateFilters();
}

void ProDeEsser::process(float* buffer, int numSamples) {
    if (!buffer || numSamples == 0)
        return;
    
    // Extract sibilance using band-pass filter
    juce::FloatVectorOperations::copy(m_sibilanceBuffer.getWritePointer(0), buffer, numSamples);
    
    juce::dsp::AudioBlock<float> block(m_sibilanceBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    m_bandPassFilter.process(context);
    
    const float* sibilanceData = m_sibilanceBuffer.getReadPointer(0);
    
    // Detect sibilance level
    float sibilanceLevel = detectSibilance(sibilanceData, numSamples);
    
    // Calculate compression
    float compression = calculateCompression(sibilanceLevel);
    m_gainReduction = compression;
    
    if (m_listenMode) {
        // Output only sibilance band
        juce::FloatVectorOperations::copy(buffer, sibilanceData, numSamples);
    } else {
        // Apply compression to sibilance band and subtract from original
        for (int i = 0; i < numSamples; ++i) {
            float original = buffer[i];
            float sibilance = sibilanceData[i];
            float reducedSibilance = sibilance * compression;
            buffer[i] = original - sibilance + reducedSibilance;
        }
    }
}

void ProDeEsser::processStereo(float* leftBuffer, float* rightBuffer, int numSamples) {
    process(leftBuffer, numSamples);
    process(rightBuffer, numSamples);
}

void ProDeEsser::reset() {
    m_highPassFilter.reset();
    m_bandPassFilter.reset();
    m_lowPassFilter.reset();
    m_gainReduction = 0.0f;
}

void ProDeEsser::updateFilters() {
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        m_sampleRate, m_frequency, 2.0);
    
    *m_bandPassFilter.coefficients = *coeffs;
}

float ProDeEsser::detectSibilance(const float* buffer, int numSamples) {
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        peak = juce::jmax(peak, std::abs(buffer[i]));
    }
    
    return juce::Decibels::gainToDecibels(peak);
}

float ProDeEsser::calculateCompression(float level) {
    if (level < m_threshold)
        return 1.0f;  // No compression
    
    float excess = level - m_threshold;
    float reduction = excess * (1.0f - 1.0f / m_ratio);
    reduction = juce::jmin(reduction, m_range);
    
    return juce::Decibels::decibelsToGain(-reduction);
}

} // namespace omega
