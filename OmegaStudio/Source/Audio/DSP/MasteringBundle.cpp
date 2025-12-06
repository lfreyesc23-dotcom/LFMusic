/**
 * @file MasteringBundle.cpp
 */

#include "MasteringBundle.h"
#include <cmath>

namespace omega {

//==============================================================================
// MultibandCompressor - Implementation in ProfessionalEffects.cpp
//==============================================================================

//==============================================================================
// TransientDesigner Implementation
//==============================================================================

TransientDesigner::TransientDesigner() {
}

void TransientDesigner::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_envelopeBuffer.setSize(1, maxBlockSize);
}

void TransientDesigner::process(float* buffer, int numSamples) {
    // Detect envelope
    float* envelope = m_envelopeBuffer.getWritePointer(0);
    
    for (int i = 0; i < numSamples; ++i) {
        float input = std::abs(buffer[i]);
        
        // Simple envelope follower
        float attack = 0.001f + m_speed * 0.01f;
        float release = 0.01f + (1.0f - m_speed) * 0.1f;
        
        float coeff = (input > m_lastEnvelope) ? attack : release;
        m_lastEnvelope += coeff * (input - m_lastEnvelope);
        envelope[i] = m_lastEnvelope;
    }
    
    // Apply attack/sustain shaping
    for (int i = 0; i < numSamples; ++i) {
        float env = envelope[i];
        float delta = (i > 0) ? env - envelope[i-1] : 0.0f;
        
        // Transient detection (positive derivative = attack)
        bool isTransient = delta > 0.001f;
        
        float gain = 1.0f;
        
        if (isTransient) {
            // Attack phase
            gain *= juce::Decibels::decibelsToGain(m_attack);
        } else {
            // Sustain phase
            gain *= juce::Decibels::decibelsToGain(m_sustain);
        }
        
        buffer[i] *= gain;
        
        // Apply clipping
        if (m_clip > 0.0f) {
            float threshold = 1.0f - m_clip;
            buffer[i] = juce::jlimit(-threshold, threshold, buffer[i]);
        }
    }
}

void TransientDesigner::reset() {
    m_lastEnvelope = 0.0f;
    m_envelopeBuffer.clear();
}

float TransientDesigner::detectTransient(const float* buffer, int start, int length) {
    float maxDelta = 0.0f;
    
    for (int i = start + 1; i < start + length; ++i) {
        float delta = std::abs(buffer[i]) - std::abs(buffer[i-1]);
        maxDelta = juce::jmax(maxDelta, delta);
    }
    
    return maxDelta;
}

//==============================================================================
// StereoImager Implementation
//==============================================================================

StereoImager::StereoImager() {
}

void StereoImager::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_tempBuffer.setSize(2, maxBlockSize);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maxBlockSize;
    spec.numChannels = 1;
    
    m_lowPassLeft.prepare(spec);
    m_lowPassRight.prepare(spec);
    m_highPassLeft.prepare(spec);
    m_highPassRight.prepare(spec);
    
    updateFilters();
}

void StereoImager::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2)
        return;
    
    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    
    // Calculate correlation
    m_correlation = calculateCorrelation(left, right, numSamples);
    
    // Apply width
    for (int i = 0; i < numSamples; ++i) {
        float mid = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;
        
        // Apply width to side signal
        side *= m_width;
        
        // Safe bass (mono low end)
        if (m_safeBass) {
            // This would need proper filtering
            // For now, reduce width at low frequencies
            side *= 0.5f;  // Simplified
        }
        
        // Decode back to L/R
        left[i] = mid + side;
        right[i] = mid - side;
    }
}

void StereoImager::reset() {
    m_lowPassLeft.reset();
    m_lowPassRight.reset();
    m_highPassLeft.reset();
    m_highPassRight.reset();
}

void StereoImager::updateFilters() {
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        m_sampleRate, m_lowCrossover, 0.707);
    
    *m_lowPassLeft.coefficients = *lpCoeffs;
    *m_lowPassRight.coefficients = *lpCoeffs;
    
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        m_sampleRate, m_lowCrossover, 0.707);
    
    *m_highPassLeft.coefficients = *hpCoeffs;
    *m_highPassRight.coefficients = *hpCoeffs;
}

float StereoImager::calculateCorrelation(const float* left, const float* right, int numSamples) {
    double sum = 0.0;
    double sumL = 0.0;
    double sumR = 0.0;
    
    for (int i = 0; i < numSamples; ++i) {
        sum += left[i] * right[i];
        sumL += left[i] * left[i];
        sumR += right[i] * right[i];
    }
    
    double denom = std::sqrt(sumL * sumR);
    return (denom > 0.0) ? static_cast<float>(sum / denom) : 0.0f;
}

//==============================================================================
// MidSideProcessor Implementation
//==============================================================================

MidSideProcessor::MidSideProcessor() {
}

void MidSideProcessor::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_midBuffer.setSize(1, maxBlockSize);
    m_sideBuffer.setSize(1, maxBlockSize);
}

void MidSideProcessor::process(juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2)
        return;
    
    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    float* mid = m_midBuffer.getWritePointer(0);
    float* side = m_sideBuffer.getWritePointer(0);
    
    // Encode to Mid/Side
    encodeToMidSide(left, right, numSamples);
    
    // Copy to mid/side buffers
    juce::FloatVectorOperations::copy(mid, left, numSamples);
    juce::FloatVectorOperations::copy(side, right, numSamples);
    
    // Apply gains
    juce::FloatVectorOperations::multiply(mid, m_midGain, numSamples);
    juce::FloatVectorOperations::multiply(side, m_sideGain, numSamples);
    
    // Process EQ and compression (placeholder)
    
    // Monitor mode
    switch (m_monitorMode) {
        case MonitorMode::MidOnly:
            juce::FloatVectorOperations::clear(side, numSamples);
            break;
        case MonitorMode::SideOnly:
            juce::FloatVectorOperations::clear(mid, numSamples);
            break;
        case MonitorMode::Stereo:
        default:
            break;
    }
    
    // Decode back to L/R
    decodeToLeftRight(mid, side, numSamples);
    
    juce::FloatVectorOperations::copy(left, mid, numSamples);
    juce::FloatVectorOperations::copy(right, side, numSamples);
}

void MidSideProcessor::reset() {
    m_midBuffer.clear();
    m_sideBuffer.clear();
}

void MidSideProcessor::encodeToMidSide(float* left, float* right, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float l = left[i];
        float r = right[i];
        left[i] = (l + r) * 0.5f;   // Mid
        right[i] = (l - r) * 0.5f;  // Side
    }
}

void MidSideProcessor::decodeToLeftRight(float* mid, float* side, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float m = mid[i];
        float s = side[i];
        mid[i] = m + s;   // Left
        side[i] = m - s;  // Right
    }
}

void MidSideProcessor::setMidEQ(int band, const EQBand& settings) {
    if (band >= 0 && band < 3) {
        m_midEQ[band] = settings;
    }
}

void MidSideProcessor::setSideEQ(int band, const EQBand& settings) {
    if (band >= 0 && band < 3) {
        m_sideEQ[band] = settings;
    }
}

void MidSideProcessor::setMidCompression(bool enabled, float threshold, float ratio) {
    m_midCompEnabled = enabled;
    m_midCompThreshold = threshold;
    m_midCompRatio = ratio;
}

void MidSideProcessor::setSideCompression(bool enabled, float threshold, float ratio) {
    m_sideCompEnabled = enabled;
    m_sideCompThreshold = threshold;
    m_sideCompRatio = ratio;
}

//==============================================================================
// AdvancedMetering Implementation
//==============================================================================

AdvancedMetering::AdvancedMetering() {
    m_peakHistoryL.resize(kHistorySize, 0.0f);
    m_peakHistoryR.resize(kHistorySize, 0.0f);
    m_rmsHistoryL.resize(kHistorySize, 0.0f);
    m_rmsHistoryR.resize(kHistorySize, 0.0f);
}

void AdvancedMetering::initialize(double sampleRate) {
    m_sampleRate = sampleRate;
}

void AdvancedMetering::process(const juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    
    if (buffer.getNumChannels() >= 1) {
        const float* left = buffer.getReadPointer(0);
        m_data.peakLeft = buffer.getMagnitude(0, 0, numSamples);
        calculateRMS(left, numSamples, m_data.rmsLeft);
        calculateTruePeak(left, numSamples, m_data.truePeakLeft);
        
        // Add to history
        m_peakHistoryL.erase(m_peakHistoryL.begin());
        m_peakHistoryL.push_back(m_data.peakLeft);
        m_rmsHistoryL.erase(m_rmsHistoryL.begin());
        m_rmsHistoryL.push_back(m_data.rmsLeft);
    }
    
    if (buffer.getNumChannels() >= 2) {
        const float* right = buffer.getReadPointer(1);
        m_data.peakRight = buffer.getMagnitude(1, 0, numSamples);
        calculateRMS(right, numSamples, m_data.rmsRight);
        calculateTruePeak(right, numSamples, m_data.truePeakRight);
        
        m_peakHistoryR.erase(m_peakHistoryR.begin());
        m_peakHistoryR.push_back(m_data.peakRight);
        m_rmsHistoryR.erase(m_rmsHistoryR.begin());
        m_rmsHistoryR.push_back(m_data.rmsRight);
    }
    
    // Calculate LUFS
    calculateLUFS(buffer);
    
    // Calculate dynamic range
    calculateDynamicRange();
    
    // Check for clipping
    m_data.clipping = (m_data.truePeakLeft >= 1.0f || m_data.truePeakRight >= 1.0f);
}

void AdvancedMetering::reset() {
    m_data = MeteringData();
    std::fill(m_peakHistoryL.begin(), m_peakHistoryL.end(), 0.0f);
    std::fill(m_peakHistoryR.begin(), m_peakHistoryR.end(), 0.0f);
    std::fill(m_rmsHistoryL.begin(), m_rmsHistoryL.end(), 0.0f);
    std::fill(m_rmsHistoryR.begin(), m_rmsHistoryR.end(), 0.0f);
}

void AdvancedMetering::calculateRMS(const float* buffer, int numSamples, float& rms) {
    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        sum += buffer[i] * buffer[i];
    }
    rms = std::sqrt(static_cast<float>(sum / numSamples));
}

void AdvancedMetering::calculateTruePeak(const float* buffer, int numSamples, float& truePeak) {
    // Simplified - production would use 4x oversampling
    truePeak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        truePeak = juce::jmax(truePeak, std::abs(buffer[i]));
    }
}

void AdvancedMetering::calculateLUFS(const juce::AudioBuffer<float>& buffer) {
    // Simplified LUFS calculation (ITU-R BS.1770)
    // Production implementation would need K-weighting filter
    float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    m_data.lufsMomentary = -0.691f + 10.0f * std::log10(rms * rms + 0.0001f);
}

void AdvancedMetering::calculateDynamicRange() {
    float peak = juce::jmax(m_data.peakLeft, m_data.peakRight);
    float rms = (m_data.rmsLeft + m_data.rmsRight) * 0.5f;
    
    if (rms > 0.0001f) {
        m_data.crestFactor = peak / rms;
        m_data.dynamicRange = juce::Decibels::gainToDecibels(m_data.crestFactor);
    }
}

std::vector<float> AdvancedMetering::getPeakHistory(int channel) const {
    return (channel == 0) ? m_peakHistoryL : m_peakHistoryR;
}

std::vector<float> AdvancedMetering::getRMSHistory(int channel) const {
    return (channel == 0) ? m_rmsHistoryL : m_rmsHistoryR;
}

} // namespace omega
