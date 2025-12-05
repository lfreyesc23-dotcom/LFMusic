/*
  ==============================================================================

    AdvancedVisualizers.cpp
    Implementation of Goniometer, Spectrogram, and visualization tools

  ==============================================================================
*/

#include "AdvancedVisualizers.h"
#include <cmath>

namespace omega {
namespace Analysis {

// ============================================================================
// Goniometer Implementation
// ============================================================================

Goniometer::Goniometer() {
    initialize(48000.0);
}

void Goniometer::initialize(double sampleRate, int maxHistoryMs) {
    sampleRate_ = sampleRate;
    maxHistorySamples_ = static_cast<int>(sampleRate * maxHistoryMs / 1000.0);
    historyBuffer_.setSize(2, maxHistorySamples_);
    historyBuffer_.clear();
    points_.reserve(maxPoints_);
}

void Goniometer::setSampleRate(double newSampleRate) {
    sampleRate_ = newSampleRate;
    initialize(sampleRate_, maxHistorySamples_ * 1000 / static_cast<int>(sampleRate_));
}

void Goniometer::process(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;
    
    const int numSamples = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    
    // Calculate correlation
    calculateCorrelation(buffer);
    
    // Generate points for visualization
    points_.clear();
    
    int stride = std::max(1, numSamples / maxPoints_);
    
    for (int i = 0; i < numSamples; i += stride) {
        float l = left[i];
        float r = right[i];
        
        Point p;
        if (displayMode_ == 0) {
            // X-Y mode (L-R)
            p.x = (l + r) * 0.5f;  // Mid
            p.y = (l - r) * 0.5f;  // Side
        } else {
            // M-S mode
            p.x = l;
            p.y = r;
        }
        
        points_.push_back(p);
    }
}

void Goniometer::calculateCorrelation(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) {
        correlation_ = 1.0f;
        return;
    }
    
    const int numSamples = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    
    double sumLR = 0.0;
    double sumLL = 0.0;
    double sumRR = 0.0;
    double sumL = 0.0;
    double sumR = 0.0;
    
    for (int i = 0; i < numSamples; ++i) {
        float l = left[i];
        float r = right[i];
        
        sumLR += l * r;
        sumLL += l * l;
        sumRR += r * r;
        sumL += std::abs(l);
        sumR += std::abs(r);
    }
    
    // Pearson correlation coefficient
    double denominator = std::sqrt(sumLL * sumRR);
    if (denominator > 1e-10) {
        correlation_ = static_cast<float>(sumLR / denominator);
    } else {
        correlation_ = 0.0f;
    }
    
    correlation_ = juce::jlimit(-1.0f, 1.0f, correlation_);
    
    // Update levels
    leftLevel_ = static_cast<float>(sumL / numSamples);
    rightLevel_ = static_cast<float>(sumR / numSamples);
}

void Goniometer::clear() {
    points_.clear();
    correlation_ = 0.0f;
    leftLevel_ = 0.0f;
    rightLevel_ = 0.0f;
}

// ============================================================================
// Spectrogram Implementation
// ============================================================================

Spectrogram::Spectrogram() : fft_(11), window_(2048, juce::dsp::WindowingFunction<float>::hann) {
    initialize(48000.0, 2048);
}

void Spectrogram::initialize(double sampleRate, int fftSize) {
    sampleRate_ = sampleRate;
    setFFTSize(fftSize);
}

void Spectrogram::setSampleRate(double newSampleRate) {
    sampleRate_ = newSampleRate;
}

void Spectrogram::setFFTSize(int size) {
    fftSize_ = size;
    int order = static_cast<int>(std::log2(size));
    fft_ = juce::dsp::FFT(order);
    window_.~WindowingFunction();
    new (&window_) juce::dsp::WindowingFunction<float>(size, juce::dsp::WindowingFunction<float>::hann);
    fftData_.resize(size * 2, 0.0f);
    samplesPerLine_ = size / 2; // Overlap
}

void Spectrogram::process(const juce::AudioBuffer<float>& buffer) {
    sampleCounter_ += buffer.getNumSamples();
    
    if (sampleCounter_ >= samplesPerLine_) {
        sampleCounter_ = 0;
        performFFT(buffer);
    }
}

void Spectrogram::performFFT(const juce::AudioBuffer<float>& buffer) {
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    
    // Mix to mono
    const int numSamples = std::min(buffer.getNumSamples(), fftSize_);
    const int numChannels = buffer.getNumChannels();
    
    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            sample += buffer.getSample(ch, i);
        }
        fftData_[i] = sample / static_cast<float>(numChannels);
    }
    
    // Apply window and FFT
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize_);
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Create new line
    SpectrogramLine line;
    line.timestamp = juce::Time::currentTimeMillis();
    line.magnitudes.resize(fftSize_ / 2);
    
    for (size_t i = 0; i < line.magnitudes.size(); ++i) {
        float magnitude = fftData_[i];
        line.magnitudes[i] = magnitude > 0.0f ? 20.0f * std::log10(magnitude) : minDb_;
    }
    
    lines_.push_back(line);
    
    // Limit history
    while (static_cast<int>(lines_.size()) > maxLines_) {
        lines_.pop_front();
    }
}

float Spectrogram::getFrequencyForBin(int bin) const {
    return static_cast<float>(bin * sampleRate_ / fftSize_);
}

juce::Colour Spectrogram::getColorForDb(float db) const {
    // Normalize to 0-1
    float normalized = (db - minDb_) / (maxDb_ - minDb_);
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    
    switch (colorMap_) {
        case 0: // Jet
            if (normalized < 0.25f) {
                return juce::Colour::fromFloatRGBA(0.0f, normalized * 4.0f, 1.0f, 1.0f);
            } else if (normalized < 0.5f) {
                return juce::Colour::fromFloatRGBA(0.0f, 1.0f, 1.0f - (normalized - 0.25f) * 4.0f, 1.0f);
            } else if (normalized < 0.75f) {
                return juce::Colour::fromFloatRGBA((normalized - 0.5f) * 4.0f, 1.0f, 0.0f, 1.0f);
            } else {
                return juce::Colour::fromFloatRGBA(1.0f, 1.0f - (normalized - 0.75f) * 4.0f, 0.0f, 1.0f);
            }
            
        case 1: // Hot
            if (normalized < 0.33f) {
                return juce::Colour::fromFloatRGBA(normalized * 3.0f, 0.0f, 0.0f, 1.0f);
            } else if (normalized < 0.66f) {
                return juce::Colour::fromFloatRGBA(1.0f, (normalized - 0.33f) * 3.0f, 0.0f, 1.0f);
            } else {
                return juce::Colour::fromFloatRGBA(1.0f, 1.0f, (normalized - 0.66f) * 3.0f, 1.0f);
            }
            
        case 2: // Grayscale
        default:
            return juce::Colour::fromFloatRGBA(normalized, normalized, normalized, 1.0f);
    }
}

void Spectrogram::clear() {
    lines_.clear();
}

// ============================================================================
// Vectorscope Implementation
// ============================================================================

Vectorscope::Vectorscope() {
    initialize(48000.0);
}

void Vectorscope::initialize(double /*sampleRate*/) {
    points_.reserve(1000);
}

void Vectorscope::process(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;
    
    points_.clear();
    
    const int numSamples = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    
    int stride = std::max(1, numSamples / 500);
    
    for (int i = 0; i < numSamples; i += stride) {
        float l = left[i];
        float r = right[i];
        
        VectorPoint p;
        p.magnitude = std::sqrt(l * l + r * r);
        p.angle = std::atan2(r, l);
        p.hue = 0.0f; // TODO: Add frequency analysis for coloring
        
        if (p.magnitude > 0.01f) {
            points_.push_back(p);
        }
    }
}

void Vectorscope::clear() {
    points_.clear();
}

// ============================================================================
// VisualizationSuite Implementation
// ============================================================================

VisualizationSuite::VisualizationSuite() {
    initialize(48000.0);
}

void VisualizationSuite::initialize(double sampleRate) {
    goniometer_.initialize(sampleRate);
    spectrogram_.initialize(sampleRate);
    vectorscope_.initialize(sampleRate);
}

void VisualizationSuite::process(const juce::AudioBuffer<float>& buffer) {
    goniometer_.process(buffer);
    spectrogram_.process(buffer);
    vectorscope_.process(buffer);
}

void VisualizationSuite::clear() {
    goniometer_.clear();
    spectrogram_.clear();
    vectorscope_.clear();
}

// ============================================================================
// CorrelationMeter Implementation
// ============================================================================

CorrelationMeter::CorrelationMeter() {
    initialize(48000.0);
}

void CorrelationMeter::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
    shortTermBuffer_.resize(static_cast<size_t>(sampleRate), 0.0f); // 1 second
    reset();
}

void CorrelationMeter::reset() {
    instantaneous_ = 0.0f;
    shortTerm_ = 0.0f;
    integrated_ = 0.0f;
    std::fill(shortTermBuffer_.begin(), shortTermBuffer_.end(), 0.0f);
    shortTermIndex_ = 0;
    integratedSum_ = 0.0;
    integratedSamples_ = 0;
}

void CorrelationMeter::process(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 2) return;
    
    const int numSamples = buffer.getNumSamples();
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    
    for (int i = 0; i < numSamples; ++i) {
        updateCorrelation(left[i], right[i]);
    }
}

void CorrelationMeter::updateCorrelation(float leftSample, float rightSample) {
    // Calculate instantaneous correlation (simplified)
    float correlation = leftSample * rightSample;
    instantaneous_ = correlation;
    
    // Update short-term buffer
    shortTermBuffer_[shortTermIndex_] = correlation;
    shortTermIndex_ = (shortTermIndex_ + 1) % static_cast<int>(shortTermBuffer_.size());
    
    // Calculate short-term average
    float sum = 0.0f;
    for (float val : shortTermBuffer_) {
        sum += val;
    }
    shortTerm_ = sum / shortTermBuffer_.size();
    
    // Update integrated
    integratedSum_ += correlation;
    integratedSamples_++;
    if (integratedSamples_ > 0) {
        integrated_ = static_cast<float>(integratedSum_ / integratedSamples_);
    }
}

float CorrelationMeter::getStereoWidth() const {
    // Estimate stereo width from correlation
    // +1 = mono, 0 = stereo, -1 = out of phase
    return (1.0f - integrated_) * 50.0f; // 0-100 scale
}

} // namespace Analysis
} // namespace omega
