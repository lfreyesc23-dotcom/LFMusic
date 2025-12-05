/**
 * @file VocalEnhancer.cpp
 * @brief Implementation of AI-powered vocal enhancement
 */

#include "VocalEnhancer.h"
#include <algorithm>
#include <cmath>

namespace omega {

// ============================================================================
// FormantDetector Implementation
// ============================================================================

FormantDetector::FormantDetector() {
    m_analysisBuffer.resize(kFFTSize, 0.0f);
    m_spectrum.resize(kFFTSize, 0.0f);
}

void FormantDetector::initialize(double sampleRate) {
    m_sampleRate = sampleRate;
    m_fft = std::make_unique<juce::dsp::FFT>(kFFTOrder);
}

void FormantDetector::analyze(const float* buffer, int numSamples) {
    // Copy samples to analysis buffer
    int copySize = std::min(numSamples, kFFTSize);
    std::copy(buffer, buffer + copySize, m_analysisBuffer.begin());

    // Apply Hamming window
    for (int i = 0; i < kFFTSize; ++i) {
        float window = 0.54f - 0.46f * std::cos(2.0f * juce::MathConstants<float>::pi * i / kFFTSize);
        m_analysisBuffer[i] *= window;
    }

    // Perform FFT
    m_fft->performRealOnlyForwardTransform(m_analysisBuffer.data());

    // Calculate magnitude spectrum
    for (int i = 0; i < kFFTSize / 2; ++i) {
        float real = m_analysisBuffer[i * 2];
        float imag = m_analysisBuffer[i * 2 + 1];
        m_spectrum[i] = std::sqrt(real * real + imag * imag);
    }

    // Extract formants
    extractFormants();

    // Check for voice presence (simple energy threshold)
    float energy = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        energy += buffer[i] * buffer[i];
    }
    energy /= numSamples;

    m_voiceDetected = energy > 0.001f; // Simple threshold
    m_confidence = m_voiceDetected ? 0.8f : 0.0f;
}

void FormantDetector::extractFormants() {
    // Simplified formant extraction using spectral peaks
    // Real implementation would use LPC or cepstral analysis

    const float binToHz = static_cast<float>(m_sampleRate) / kFFTSize;

    // Find peaks in typical formant regions
    std::array<int, 3> formantBins = {0, 0, 0};
    
    // F1: 300-1000 Hz (male), 350-1100 Hz (female)
    int f1Start = static_cast<int>(300.0f / binToHz);
    int f1End = static_cast<int>(1100.0f / binToHz);
    formantBins[0] = findPeakInRange(f1Start, f1End);

    // F2: 850-2300 Hz (male), 1000-2700 Hz (female)
    int f2Start = static_cast<int>(850.0f / binToHz);
    int f2End = static_cast<int>(2700.0f / binToHz);
    formantBins[1] = findPeakInRange(f2Start, f2End);

    // F3: 1700-3200 Hz (male), 2000-3500 Hz (female)
    int f3Start = static_cast<int>(1700.0f / binToHz);
    int f3End = static_cast<int>(3500.0f / binToHz);
    formantBins[2] = findPeakInRange(f3Start, f3End);

    // Convert bins to frequencies
    for (int i = 0; i < 3; ++i) {
        m_formants[i] = formantBins[i] * binToHz;
    }
}

int FormantDetector::findPeakInRange(int startBin, int endBin) const {
    int peakBin = startBin;
    float peakValue = 0.0f;

    for (int i = startBin; i < endBin && i < static_cast<int>(m_spectrum.size()); ++i) {
        if (m_spectrum[i] > peakValue) {
            peakValue = m_spectrum[i];
            peakBin = i;
        }
    }

    return peakBin;
}

void FormantDetector::performLPC() {
    // Linear Predictive Coding implementation would go here
    // This is a complex algorithm - using simplified peak detection instead
}

// ============================================================================
// DeEsser Implementation
// ============================================================================

DeEsser::DeEsser() = default;

void DeEsser::initialize(double sampleRate) {
    m_sampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 2048;
    spec.numChannels = 1;

    // Initialize bandpass filter for sibilance detection
    m_bandpassFilter.prepare(spec);
    setFrequencyRange(m_lowFreq, m_highFreq);

    // Initialize compressor for reduction
    m_compressor.prepare(spec);
    m_compressor.setRatio(4.0f);
    m_compressor.setAttack(1.0f);
    m_compressor.setRelease(50.0f);
}

void DeEsser::process(float* buffer, int numSamples) {
    if (m_amount <= 0.0f) {
        return; // Bypass
    }

    // Create JUCE audio block
    juce::dsp::AudioBlock<float> block(&buffer, 1, numSamples);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Copy for sidechain detection
    std::vector<float> sidechain(buffer, buffer + numSamples);
    juce::dsp::AudioBlock<float> sidechainBlock(&sidechain[0], 1, numSamples);
    juce::dsp::ProcessContextReplacing<float> sidechainContext(sidechainBlock);

    // Filter sidechain to isolate sibilance
    m_bandpassFilter.process(sidechainContext);

    // Compress based on filtered signal
    m_compressor.setThreshold(m_threshold);
    
    // Apply compression with amount control
    for (int i = 0; i < numSamples; ++i) {
        float sibilanceLevel = std::abs(sidechain[i]);
        
        if (sibilanceLevel > juce::Decibels::decibelsToGain(m_threshold)) {
            float reduction = 1.0f - (m_amount * 0.5f);
            buffer[i] *= reduction;
        }
    }
}

void DeEsser::reset() {
    m_bandpassFilter.reset();
    m_compressor.reset();
}

void DeEsser::setFrequencyRange(float lowFreq, float highFreq) {
    m_lowFreq = lowFreq;
    m_highFreq = highFreq;

    // Create bandpass filter
    auto centerFreq = std::sqrt(lowFreq * highFreq);
    auto q = centerFreq / (highFreq - lowFreq);

    *m_bandpassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass(
        m_sampleRate, centerFreq, q
    );
}

// ============================================================================
// BreathRemover Implementation
// ============================================================================

BreathRemover::BreathRemover() = default;

void BreathRemover::initialize(double sampleRate) {
    m_sampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 2048;
    spec.numChannels = 1;

    m_highpassFilter.prepare(spec);
    *m_highpassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        sampleRate, kBreathFreqLow
    );
}

void BreathRemover::process(float* buffer, int numSamples) {
    if (m_sensitivity <= 0.0f) {
        return; // Bypass
    }

    // Calculate energy
    float energy = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        energy += buffer[i] * buffer[i];
    }
    energy = std::sqrt(energy / numSamples);

    // Calculate spectral centroid
    float centroid = calculateSpectralCentroid(buffer, numSamples);

    // Detect breath
    bool isBreath = detectBreath(energy, centroid);

    // Apply gate
    float targetGain = isBreath ? juce::Decibels::decibelsToGain(m_reduction) : 1.0f;
    
    // Smooth gain changes
    const float smoothing = 0.01f;
    for (int i = 0; i < numSamples; ++i) {
        m_gateState += (targetGain - m_gateState) * smoothing;
        buffer[i] *= m_gateState;
    }
}

void BreathRemover::reset() {
    m_highpassFilter.reset();
    m_gateState = 1.0f;
}

bool BreathRemover::detectBreath(float energy, float spectralCentroid) {
    // Breath sounds are typically:
    // - Low energy
    // - Low spectral centroid (but not too low)
    // - In specific frequency range

    float energyThreshold = 0.01f * (1.0f - m_sensitivity);
    float centroidLow = 500.0f;
    float centroidHigh = 2000.0f;

    return (energy < energyThreshold &&
            centroid > centroidLow &&
            centroid < centroidHigh);
}

float BreathRemover::calculateSpectralCentroid(const float* buffer, int numSamples) {
    // Simplified spectral centroid calculation
    float weightedSum = 0.0f;
    float sum = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        float magnitude = std::abs(buffer[i]);
        weightedSum += magnitude * i;
        sum += magnitude;
    }

    if (sum > 0.0f) {
        return (weightedSum / sum) * static_cast<float>(m_sampleRate) / numSamples;
    }

    return 0.0f;
}

// ============================================================================
// VocalEQ Implementation
// ============================================================================

VocalEQ::VocalEQ() = default;

void VocalEQ::initialize(double sampleRate) {
    m_sampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 2048;
    spec.numChannels = 1;

    m_lowCutFilter.prepare(spec);
    m_lowShelfFilter.prepare(spec);
    m_midPeakFilter.prepare(spec);
    m_presenceFilter.prepare(spec);
    m_brightnessFilter.prepare(spec);

    // Set default filter coefficients
    setLowCut(kLowCutFreq);
    setLowShelf(kLowShelfFreq, 0.0f);
    setMidPeak(kMidPeakFreq, 0.0f, 1.0f);
    setPresence(0.0f);
    setBrightness(0.0f);
}

void VocalEQ::process(float* buffer, int numSamples) {
    juce::dsp::AudioBlock<float> block(&buffer, 1, numSamples);
    juce::dsp::ProcessContextReplacing<float> context(block);

    m_lowCutFilter.process(context);
    m_lowShelfFilter.process(context);
    m_midPeakFilter.process(context);
    m_presenceFilter.process(context);
    m_brightnessFilter.process(context);
}

void VocalEQ::reset() {
    m_lowCutFilter.reset();
    m_lowShelfFilter.reset();
    m_midPeakFilter.reset();
    m_presenceFilter.reset();
    m_brightnessFilter.reset();
}

void VocalEQ::autoAdjust(const std::array<float, 3>& formants) {
    // Adjust EQ based on detected formants
    // Boost around F1 and F2 for clarity
    
    if (formants[0] > 0.0f) {
        setLowShelf(formants[0] * 0.5f, 2.0f);
    }
    
    if (formants[1] > 0.0f) {
        setMidPeak(formants[1], 3.0f, 2.0f);
    }
    
    // Add presence and air
    setPresence(2.0f);
    setBrightness(1.5f);
}

void VocalEQ::setLowCut(float frequency) {
    *m_lowCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        m_sampleRate, frequency
    );
}

void VocalEQ::setLowShelf(float frequency, float gain) {
    *m_lowShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        m_sampleRate, frequency, 0.7f, juce::Decibels::decibelsToGain(gain)
    );
}

void VocalEQ::setMidPeak(float frequency, float gain, float q) {
    *m_midPeakFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        m_sampleRate, frequency, q, juce::Decibels::decibelsToGain(gain)
    );
}

void VocalEQ::setPresence(float gain) {
    *m_presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        m_sampleRate, kPresenceFreq, 0.7f, juce::Decibels::decibelsToGain(gain)
    );
}

void VocalEQ::setBrightness(float gain) {
    *m_brightnessFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        m_sampleRate, kBrightnessFreq, 0.7f, juce::Decibels::decibelsToGain(gain)
    );
}

// ============================================================================
// ProximityCompensator Implementation
// ============================================================================

ProximityCompensator::ProximityCompensator() = default;

void ProximityCompensator::initialize(double sampleRate) {
    m_sampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 2048;
    spec.numChannels = 1;

    m_highpassFilter.prepare(spec);
}

void ProximityCompensator::process(float* buffer, int numSamples) {
    if (m_amount <= 0.0f) {
        return;
    }

    // Compensate for bass buildup from proximity effect
    float cutoffFreq = 80.0f + (m_amount * 120.0f); // 80-200 Hz range

    *m_highpassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
        m_sampleRate, cutoffFreq, 0.7f
    );

    juce::dsp::AudioBlock<float> block(&buffer, 1, numSamples);
    juce::dsp::ProcessContextReplacing<float> context(block);
    m_highpassFilter.process(context);
}

void ProximityCompensator::reset() {
    m_highpassFilter.reset();
}

// ============================================================================
// VocalEnhancer Implementation
// ============================================================================

VocalEnhancer::VocalEnhancer() = default;

void VocalEnhancer::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;

    // Initialize all processors
    m_formantDetector.initialize(sampleRate);
    m_deEsser.initialize(sampleRate);
    m_breathRemover.initialize(sampleRate);
    m_vocalEQ.initialize(sampleRate);
    m_proximityComp.initialize(sampleRate);

    // Initialize dynamics
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maxBlockSize;
    spec.numChannels = 1;

    m_compressor.prepare(spec);
    m_limiter.prepare(spec);

    // Allocate buffers
    m_dryBuffer.resize(maxBlockSize);
    m_wetBuffer.resize(maxBlockSize);

    // Apply default mode
    setMode(m_mode);
}

void VocalEnhancer::process(float* buffer, int numSamples) {
    if (m_amount <= 0.0f) {
        return; // Bypass
    }

    // Measure input level
    m_inputLevel = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        m_inputLevel = std::max(m_inputLevel, std::abs(buffer[i]));
    }

    // Copy dry signal
    std::copy(buffer, buffer + numSamples, m_dryBuffer.begin());

    // Periodic analysis
    m_analysisFrameCounter += numSamples;
    if (m_analysisFrameCounter >= kAnalysisFrames) {
        m_formantDetector.analyze(buffer, numSamples);
        
        if (m_autoEQEnabled && m_formantDetector.isVoiceDetected()) {
            m_vocalEQ.autoAdjust(m_formantDetector.getFormants());
        }
        
        if (m_voiceType == VoiceType::Auto) {
            detectVoiceType(buffer, numSamples);
        }
        
        m_analysisFrameCounter = 0;
    }

    // Apply processing chain
    if (m_proximityCompEnabled) {
        m_proximityComp.process(buffer, numSamples);
    }

    if (m_breathRemovalEnabled) {
        m_breathRemover.process(buffer, numSamples);
    }

    m_vocalEQ.process(buffer, numSamples);

    // Compression
    juce::dsp::AudioBlock<float> block(&buffer, 1, numSamples);
    juce::dsp::ProcessContextReplacing<float> context(block);
    m_compressor.process(context);

    if (m_deEsserEnabled) {
        m_deEsser.process(buffer, numSamples);
    }

    // Limiting
    m_limiter.process(context);

    // Mix dry/wet based on amount
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = m_dryBuffer[i] * (1.0f - m_amount) + buffer[i] * m_amount;
    }

    // Measure output level
    m_outputLevel = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        m_outputLevel = std::max(m_outputLevel, std::abs(buffer[i]));
    }
}

void VocalEnhancer::processStereo(float* leftBuffer, float* rightBuffer, int numSamples) {
    // Process each channel independently
    process(leftBuffer, numSamples);
    process(rightBuffer, numSamples);
}

void VocalEnhancer::reset() {
    m_deEsser.reset();
    m_breathRemover.reset();
    m_vocalEQ.reset();
    m_proximityComp.reset();
    m_compressor.reset();
    m_limiter.reset();
}

void VocalEnhancer::autoOptimize(const float* buffer, int numSamples) {
    m_formantDetector.analyze(buffer, numSamples);
    
    if (m_formantDetector.isVoiceDetected()) {
        detectVoiceType(buffer, numSamples);
        m_vocalEQ.autoAdjust(m_formantDetector.getFormants());
    }
}

void VocalEnhancer::setMode(Mode mode) {
    m_mode = mode;
    applyModeSettings();
}

void VocalEnhancer::applyModeSettings() {
    switch (m_mode) {
        case Mode::Natural:
            m_deEsser.setAmount(0.3f);
            m_breathRemover.setSensitivity(0.3f);
            m_proximityComp.setAmount(0.3f);
            m_compressor.setThreshold(-20.0f);
            m_compressor.setRatio(2.0f);
            m_vocalEQ.setPresence(1.0f);
            m_vocalEQ.setBrightness(0.5f);
            break;

        case Mode::Podcast:
            m_deEsser.setAmount(0.6f);
            m_breathRemover.setSensitivity(0.7f);
            m_proximityComp.setAmount(0.5f);
            m_compressor.setThreshold(-18.0f);
            m_compressor.setRatio(3.0f);
            m_vocalEQ.setPresence(3.0f);
            m_vocalEQ.setBrightness(2.0f);
            break;

        case Mode::Radio:
            m_deEsser.setAmount(0.7f);
            m_breathRemover.setSensitivity(0.9f);
            m_proximityComp.setAmount(0.7f);
            m_compressor.setThreshold(-15.0f);
            m_compressor.setRatio(4.0f);
            m_vocalEQ.setPresence(4.0f);
            m_vocalEQ.setBrightness(3.0f);
            break;

        case Mode::Studio:
            m_deEsser.setAmount(0.5f);
            m_breathRemover.setSensitivity(0.5f);
            m_proximityComp.setAmount(0.4f);
            m_compressor.setThreshold(-16.0f);
            m_compressor.setRatio(3.5f);
            m_vocalEQ.setPresence(2.5f);
            m_vocalEQ.setBrightness(2.0f);
            break;

        case Mode::Custom:
            // User will set parameters manually
            break;
    }

    // Common settings
    m_compressor.setAttack(5.0f);
    m_compressor.setRelease(100.0f);
    m_limiter.setThreshold(-1.0f);
}

void VocalEnhancer::detectVoiceType(const float* buffer, int numSamples) {
    auto formants = m_formantDetector.getFormants();
    
    // Simple voice type detection based on F1
    if (formants[0] < 500.0f) {
        m_voiceType = VoiceType::Male;
    } else if (formants[0] > 700.0f) {
        m_voiceType = VoiceType::Female;
    } else {
        m_voiceType = VoiceType::Male; // Default
    }
}

void VocalEnhancer::setDeEsserAmount(float amount) {
    m_deEsser.setAmount(amount);
}

void VocalEnhancer::setDeEsserThreshold(float threshold) {
    m_deEsser.setThreshold(threshold);
}

void VocalEnhancer::setBreathRemovalSensitivity(float sensitivity) {
    m_breathRemover.setSensitivity(sensitivity);
}

void VocalEnhancer::setBreathRemovalReduction(float reduction) {
    m_breathRemover.setReduction(reduction);
}

void VocalEnhancer::setProximityCompensation(float amount) {
    m_proximityComp.setAmount(amount);
}

void VocalEnhancer::setLowCut(float frequency) {
    m_vocalEQ.setLowCut(frequency);
}

void VocalEnhancer::setPresence(float gain) {
    m_vocalEQ.setPresence(gain);
}

void VocalEnhancer::setBrightness(float gain) {
    m_vocalEQ.setBrightness(gain);
}

} // namespace omega
