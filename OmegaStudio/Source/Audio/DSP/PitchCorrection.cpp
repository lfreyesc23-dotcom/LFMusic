/**
 * @file PitchCorrection.cpp
 * @brief Implementation of real-time pitch correction
 */

#include "PitchCorrection.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace omega {

// ============================================================================
// PitchDetector Implementation
// ============================================================================

PitchDetector::PitchDetector() {
    m_yinBuffer.resize(2048);
}

void PitchDetector::initialize(double sampleRate, int bufferSize) {
    m_sampleRate = sampleRate;
    m_bufferSize = bufferSize;
    m_yinBuffer.resize(bufferSize / 2);
    m_confidence = 0.0f;
}

float PitchDetector::detectPitch(const float* buffer, int numSamples) {
    if (numSamples < m_bufferSize) {
        return 0.0f;
    }

    // YIN algorithm implementation
    calculateDifference(buffer, numSamples);
    cumulativeMeanNormalizedDifference();
    
    int tauEstimate = absoluteThreshold(kDefaultThreshold);
    
    if (tauEstimate == -1) {
        m_confidence = 0.0f;
        return 0.0f;
    }

    float betterTau = parabolicInterpolation(tauEstimate);
    
    // Calculate confidence based on the minimum value
    m_confidence = 1.0f - m_yinBuffer[tauEstimate];
    
    // Convert tau to frequency
    float frequency = static_cast<float>(m_sampleRate) / betterTau;
    
    return frequency;
}

void PitchDetector::calculateDifference(const float* buffer, int numSamples) {
    const int halfBufferSize = m_yinBuffer.size();
    
    for (int tau = 0; tau < halfBufferSize; ++tau) {
        float sum = 0.0f;
        for (int i = 0; i < halfBufferSize; ++i) {
            float delta = buffer[i] - buffer[i + tau];
            sum += delta * delta;
        }
        m_yinBuffer[tau] = sum;
    }
}

void PitchDetector::cumulativeMeanNormalizedDifference() {
    m_yinBuffer[0] = 1.0f;
    float runningSum = 0.0f;
    
    for (int tau = 1; tau < static_cast<int>(m_yinBuffer.size()); ++tau) {
        runningSum += m_yinBuffer[tau];
        m_yinBuffer[tau] *= tau / runningSum;
    }
}

int PitchDetector::absoluteThreshold(float threshold) {
    const int size = static_cast<int>(m_yinBuffer.size());
    
    // Find first minimum below threshold
    for (int tau = 2; tau < size; ++tau) {
        if (m_yinBuffer[tau] < threshold) {
            // Check if this is a local minimum
            while (tau + 1 < size && m_yinBuffer[tau + 1] < m_yinBuffer[tau]) {
                ++tau;
            }
            return tau;
        }
    }
    
    return -1; // No pitch detected
}

float PitchDetector::parabolicInterpolation(int tauEstimate) {
    if (tauEstimate < 1 || tauEstimate >= static_cast<int>(m_yinBuffer.size()) - 1) {
        return static_cast<float>(tauEstimate);
    }
    
    float s0 = m_yinBuffer[tauEstimate - 1];
    float s1 = m_yinBuffer[tauEstimate];
    float s2 = m_yinBuffer[tauEstimate + 1];
    
    float adjustment = (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
    
    return static_cast<float>(tauEstimate) + adjustment;
}

// ============================================================================
// PhaseVocoder Implementation
// ============================================================================

PhaseVocoder::PhaseVocoder() = default;

void PhaseVocoder::initialize(double sampleRate, int fftSize, int hopSize) {
    m_sampleRate = sampleRate;
    m_fftSize = fftSize;
    m_hopSize = hopSize;
    
    m_fft = std::make_unique<juce::dsp::FFT>(static_cast<int>(std::log2(fftSize)));
    
    m_inputBuffer.resize(fftSize * 2, 0.0f);
    m_outputBuffer.resize(fftSize * 2, 0.0f);
    m_windowBuffer.resize(fftSize);
    m_fftBuffer.resize(fftSize);
    m_lastPhase.resize(fftSize / 2 + 1, 0.0f);
    m_sumPhase.resize(fftSize / 2 + 1, 0.0f);
    
    // Create Hann window
    for (int i = 0; i < fftSize; ++i) {
        m_windowBuffer[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
    }
    
    reset();
}

void PhaseVocoder::process(const float* inputBuffer, float* outputBuffer,
                          int numSamples, float pitchRatio) {
    // Simple pitch shifting implementation
    // For production, would need more sophisticated overlap-add
    
    for (int i = 0; i < numSamples; ++i) {
        m_inputBuffer[m_inputPos++] = inputBuffer[i];
        
        if (m_inputPos >= m_fftSize) {
            processFrame();
            m_inputPos = m_hopSize;
            
            // Shift buffer
            std::copy(m_inputBuffer.begin() + m_hopSize,
                     m_inputBuffer.end(),
                     m_inputBuffer.begin());
            std::fill(m_inputBuffer.begin() + (m_fftSize - m_hopSize),
                     m_inputBuffer.end(), 0.0f);
        }
        
        // Simple resampling for pitch shift
        float readPos = m_outputPos / pitchRatio;
        int readIndex = static_cast<int>(readPos);
        float frac = readPos - readIndex;
        
        if (readIndex < static_cast<int>(m_outputBuffer.size()) - 1) {
            outputBuffer[i] = m_outputBuffer[readIndex] * (1.0f - frac) +
                             m_outputBuffer[readIndex + 1] * frac;
        } else {
            outputBuffer[i] = 0.0f;
        }
        
        m_outputPos++;
        if (m_outputPos >= m_fftSize) {
            m_outputPos = 0;
        }
    }
}

void PhaseVocoder::processFrame() {
    // Apply window
    std::vector<float> windowedInput(m_fftSize);
    for (int i = 0; i < m_fftSize; ++i) {
        windowedInput[i] = m_inputBuffer[i] * m_windowBuffer[i];
    }
    
    // Perform FFT
    m_fft->performRealOnlyForwardTransform(windowedInput.data());
    
    // Convert to complex for phase manipulation
    for (int i = 0; i < m_fftSize / 2 + 1; ++i) {
        float real = windowedInput[i * 2];
        float imag = windowedInput[i * 2 + 1];
        float magnitude = std::sqrt(real * real + imag * imag);
        float phase = std::atan2(imag, real);
        
        // Phase accumulation
        float deltaPhase = phase - m_lastPhase[i];
        m_lastPhase[i] = phase;
        
        // Wrap phase
        deltaPhase = deltaPhase - 2.0f * juce::MathConstants<float>::pi *
                    std::round(deltaPhase / (2.0f * juce::MathConstants<float>::pi));
        
        m_sumPhase[i] += deltaPhase;
        
        // Reconstruct
        m_fftBuffer[i] = std::polar(magnitude, m_sumPhase[i]);
    }
    
    // Convert back for inverse FFT
    for (int i = 0; i < m_fftSize / 2 + 1; ++i) {
        windowedInput[i * 2] = m_fftBuffer[i].real();
        windowedInput[i * 2 + 1] = m_fftBuffer[i].imag();
    }
    
    // Perform inverse FFT
    m_fft->performRealOnlyInverseTransform(windowedInput.data());
    
    // Apply window and overlap-add
    for (int i = 0; i < m_fftSize; ++i) {
        m_outputBuffer[i] += windowedInput[i] * m_windowBuffer[i];
    }
}

void PhaseVocoder::reset() {
    std::fill(m_inputBuffer.begin(), m_inputBuffer.end(), 0.0f);
    std::fill(m_outputBuffer.begin(), m_outputBuffer.end(), 0.0f);
    std::fill(m_lastPhase.begin(), m_lastPhase.end(), 0.0f);
    std::fill(m_sumPhase.begin(), m_sumPhase.end(), 0.0f);
    m_inputPos = 0;
    m_outputPos = 0;
}

// ============================================================================
// PitchCorrection Implementation
// ============================================================================

PitchCorrection::PitchCorrection() {
    m_scaleNotes.fill(true); // Chromatic by default
}

void PitchCorrection::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    
    // Initialize pitch detector with larger buffer for better accuracy
    m_pitchDetector.initialize(sampleRate, 2048);
    
    // Initialize phase vocoder
    m_phaseVocoder.initialize(sampleRate, 2048, 512);
    
    updateScaleNotes();
}

void PitchCorrection::process(float* buffer, int numSamples) {
    if (m_mode == Mode::Off) {
        return; // Bypass
    }
    
    // Detect pitch
    m_detectedPitch = m_pitchDetector.detectPitch(buffer, numSamples);
    
    if (m_detectedPitch < kMinFrequency || m_detectedPitch > kMaxFrequency) {
        return; // Out of range
    }
    
    // Determine target pitch
    float targetPitch = 0.0f;
    if (m_mode == Mode::Automatic) {
        targetPitch = quantizePitch(m_detectedPitch);
    } else {
        targetPitch = m_targetPitch;
    }
    
    // Smooth target pitch
    m_smoothedPitch = m_smoothedPitch * m_smoothingCoeff +
                      targetPitch * (1.0f - m_smoothingCoeff);
    
    m_correctedPitch = m_smoothedPitch;
    
    // Calculate pitch shift ratio
    float pitchRatio = calculatePitchRatio(m_detectedPitch, m_correctedPitch);
    
    // Apply correction strength
    pitchRatio = 1.0f + (pitchRatio - 1.0f) * m_strength;
    
    // Process with phase vocoder
    std::vector<float> tempBuffer(numSamples);
    m_phaseVocoder.process(buffer, tempBuffer.data(), numSamples, pitchRatio);
    
    // Mix dry/wet based on strength
    for (int i = 0; i < numSamples; ++i) {
        buffer[i] = buffer[i] * (1.0f - m_strength) + tempBuffer[i] * m_strength;
    }
}

void PitchCorrection::processStereo(float* leftBuffer, float* rightBuffer, int numSamples) {
    // Process both channels identically for mono pitch detection
    // Use left channel for detection
    process(leftBuffer, numSamples);
    
    // Apply same correction to right channel
    float pitchRatio = calculatePitchRatio(m_detectedPitch, m_correctedPitch);
    pitchRatio = 1.0f + (pitchRatio - 1.0f) * m_strength;
    
    std::vector<float> tempBuffer(numSamples);
    m_phaseVocoder.process(rightBuffer, tempBuffer.data(), numSamples, pitchRatio);
    
    for (int i = 0; i < numSamples; ++i) {
        rightBuffer[i] = rightBuffer[i] * (1.0f - m_strength) + tempBuffer[i] * m_strength;
    }
}

void PitchCorrection::reset() {
    m_phaseVocoder.reset();
    m_detectedPitch = 0.0f;
    m_correctedPitch = 0.0f;
    m_smoothedPitch = 0.0f;
}

void PitchCorrection::updateScaleNotes() {
    m_scaleNotes.fill(false);
    
    switch (m_scale) {
        case Scale::Chromatic:
            m_scaleNotes.fill(true);
            break;
            
        case Scale::Major:
            // W-W-H-W-W-W-H pattern
            m_scaleNotes[0] = true;  // Root
            m_scaleNotes[2] = true;  // Major 2nd
            m_scaleNotes[4] = true;  // Major 3rd
            m_scaleNotes[5] = true;  // Perfect 4th
            m_scaleNotes[7] = true;  // Perfect 5th
            m_scaleNotes[9] = true;  // Major 6th
            m_scaleNotes[11] = true; // Major 7th
            break;
            
        case Scale::Minor:
            // W-H-W-W-H-W-W pattern
            m_scaleNotes[0] = true;  // Root
            m_scaleNotes[2] = true;  // Major 2nd
            m_scaleNotes[3] = true;  // Minor 3rd
            m_scaleNotes[5] = true;  // Perfect 4th
            m_scaleNotes[7] = true;  // Perfect 5th
            m_scaleNotes[8] = true;  // Minor 6th
            m_scaleNotes[10] = true; // Minor 7th
            break;
            
        case Scale::Pentatonic:
            m_scaleNotes[0] = true;  // Root
            m_scaleNotes[2] = true;  // Major 2nd
            m_scaleNotes[4] = true;  // Major 3rd
            m_scaleNotes[7] = true;  // Perfect 5th
            m_scaleNotes[9] = true;  // Major 6th
            break;
            
        case Scale::Blues:
            m_scaleNotes[0] = true;  // Root
            m_scaleNotes[3] = true;  // Minor 3rd
            m_scaleNotes[5] = true;  // Perfect 4th
            m_scaleNotes[6] = true;  // Tritone
            m_scaleNotes[7] = true;  // Perfect 5th
            m_scaleNotes[10] = true; // Minor 7th
            break;
            
        case Scale::Custom:
            // User will set this manually
            break;
    }
}

float PitchCorrection::quantizePitch(float detectedFreq) {
    // Convert frequency to MIDI note
    float midiNote = frequencyToMidi(detectedFreq);
    int roundedNote = static_cast<int>(std::round(midiNote));
    
    // Find closest scale note
    int closestNote = findClosestScaleNote(roundedNote);
    
    // Convert back to frequency
    return midiToFrequency(static_cast<float>(closestNote));
}

float PitchCorrection::frequencyToMidi(float freq) const noexcept {
    return 69.0f + 12.0f * std::log2(freq / kA4Frequency);
}

float PitchCorrection::midiToFrequency(float midi) const noexcept {
    return kA4Frequency * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

int PitchCorrection::findClosestScaleNote(int midiNote) const {
    int noteInScale = (midiNote - m_rootNote) % 12;
    if (noteInScale < 0) noteInScale += 12;
    
    // If note is in scale, return it
    if (m_scaleNotes[noteInScale]) {
        return midiNote;
    }
    
    // Find closest scale note
    int minDistance = 12;
    int closestNote = midiNote;
    
    for (int i = 0; i < 12; ++i) {
        if (m_scaleNotes[i]) {
            int distance = std::abs(noteInScale - i);
            if (distance > 6) distance = 12 - distance; // Wrap around
            
            if (distance < minDistance) {
                minDistance = distance;
                int offset = i - noteInScale;
                if (offset > 6) offset -= 12;
                if (offset < -6) offset += 12;
                closestNote = midiNote + offset;
            }
        }
    }
    
    return closestNote;
}

float PitchCorrection::calculatePitchRatio(float currentFreq, float targetFreq) const noexcept {
    if (currentFreq <= 0.0f || targetFreq <= 0.0f) {
        return 1.0f;
    }
    return targetFreq / currentFreq;
}

} // namespace omega
