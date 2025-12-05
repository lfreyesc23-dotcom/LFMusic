/*
  ==============================================================================

    TonalBalance.cpp
    Implementation of tonal balance analyzer

  ==============================================================================
*/

#include "TonalBalance.h"
#include <cmath>
#include <algorithm>

namespace omega {
namespace Analysis {

// ============================================================================
// TonalBalanceAnalyzer Implementation
// ============================================================================

TonalBalanceAnalyzer::TonalBalanceAnalyzer() : fft_(13), 
    window_(8192, juce::dsp::WindowingFunction<float>::hann) {
    initialize(48000.0, 8192);
}

void TonalBalanceAnalyzer::initialize(double sampleRate, int fftSize) {
    sampleRate_ = sampleRate;
    setFFTSize(fftSize);
    bands_ = getStandardOctaveBands();
    setTargetCurve(TargetCurve::Flat);
}

void TonalBalanceAnalyzer::setSampleRate(double newSampleRate) {
    sampleRate_ = newSampleRate;
}

void TonalBalanceAnalyzer::setFFTSize(int size) {
    fftSize_ = size;
    int order = static_cast<int>(std::log2(size));
    fft_ = juce::dsp::FFT(order);
    window_.~WindowingFunction();
    new (&window_) juce::dsp::WindowingFunction<float>(size, juce::dsp::WindowingFunction<float>::hann);
    fftData_.resize(size * 2, 0.0f);
}

std::vector<OctaveBand> TonalBalanceAnalyzer::getStandardOctaveBands() {
    // ISO 266 standard 1/3 octave center frequencies
    const float centers[] = {
        31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f, 200.0f, 250.0f,
        315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f, 1250.0f, 1600.0f, 2000.0f,
        2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f
    };
    
    std::vector<OctaveBand> bands;
    for (float center : centers) {
        bands.push_back(OctaveBand(center));
    }
    return bands;
}

std::vector<OctaveBand> TonalBalanceAnalyzer::getExtendedOctaveBands() {
    const float centers[] = {
        20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f,
        200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f, 1250.0f,
        1600.0f, 2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f, 6300.0f, 8000.0f,
        10000.0f, 12500.0f, 16000.0f, 20000.0f
    };
    
    std::vector<OctaveBand> bands;
    for (float center : centers) {
        bands.push_back(OctaveBand(center));
    }
    return bands;
}

TonalBalanceResult TonalBalanceAnalyzer::analyze(const juce::AudioBuffer<float>& buffer) {
    currentResult_ = TonalBalanceResult();
    
    performOctaveBandAnalysis(buffer);
    calculateEnergyDistribution();
    
    // Get target curve
    currentResult_.targetCurve = getTargetCurveData(targetCurveType_);
    
    // Calculate difference
    currentResult_.difference.resize(currentResult_.octaveBandLevels.size());
    for (size_t i = 0; i < currentResult_.octaveBandLevels.size(); ++i) {
        currentResult_.difference[i] = currentResult_.octaveBandLevels[i] - currentResult_.targetCurve[i];
    }
    
    // Calculate overall score
    currentResult_.overallScore = calculateDifferenceScore();
    
    return currentResult_;
}

void TonalBalanceAnalyzer::performOctaveBandAnalysis(const juce::AudioBuffer<float>& buffer) {
    // Perform FFT
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    
    // Mix to mono
    const int numSamples = std::min(buffer.getNumSamples(), fftSize_);
    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            sample += buffer.getSample(ch, i);
        }
        fftData_[i] = sample / buffer.getNumChannels();
    }
    
    // Apply window and FFT
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize_);
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Analyze each octave band
    currentResult_.octaveBandLevels.resize(bands_.size());
    
    for (size_t i = 0; i < bands_.size(); ++i) {
        const auto& band = bands_[i];
        
        // Find FFT bins in this band
        int lowerBin = static_cast<int>(band.lowerFreq * fftSize_ / sampleRate_);
        int upperBin = static_cast<int>(band.upperFreq * fftSize_ / sampleRate_);
        lowerBin = juce::jlimit(0, fftSize_ / 2, lowerBin);
        upperBin = juce::jlimit(0, fftSize_ / 2, upperBin);
        
        // Average magnitude in this band
        float sum = 0.0f;
        int count = 0;
        for (int bin = lowerBin; bin <= upperBin; ++bin) {
            sum += fftData_[bin];
            count++;
        }
        
        float avgMagnitude = count > 0 ? sum / count : 0.0f;
        currentResult_.octaveBandLevels[i] = avgMagnitude > 0.0f ? 
            20.0f * std::log10(avgMagnitude) : -100.0f;
    }
}

void TonalBalanceAnalyzer::calculateEnergyDistribution() {
    float lowSum = 0.0f, midSum = 0.0f, highSum = 0.0f;
    int lowCount = 0, midCount = 0, highCount = 0;
    
    for (size_t i = 0; i < bands_.size(); ++i) {
        float freq = bands_[i].centerFreq;
        float level = currentResult_.octaveBandLevels[i];
        
        if (freq < 200.0f) {
            lowSum += level;
            lowCount++;
        } else if (freq < 5000.0f) {
            midSum += level;
            midCount++;
        } else {
            highSum += level;
            highCount++;
        }
    }
    
    currentResult_.lowEnergy = lowCount > 0 ? lowSum / lowCount : -100.0f;
    currentResult_.midEnergy = midCount > 0 ? midSum / midCount : -100.0f;
    currentResult_.highEnergy = highCount > 0 ? highSum / highCount : -100.0f;
}

void TonalBalanceAnalyzer::setTargetCurve(TargetCurve curve) {
    targetCurveType_ = curve;
    currentResult_.targetCurve = getTargetCurveData(curve);
}

void TonalBalanceAnalyzer::setCustomCurve(const std::vector<float>& curve) {
    targetCurveType_ = TargetCurve::Custom;
    currentResult_.targetCurve = curve;
}

std::vector<float> TonalBalanceAnalyzer::getTargetCurveData(TargetCurve curve) const {
    switch (curve) {
        case TargetCurve::Harman:
            return TargetCurveLibrary::getHarmanCurve();
        case TargetCurve::Studio:
            return TargetCurveLibrary::getStudioCurve();
        case TargetCurve::Broadcast:
            return TargetCurveLibrary::getBroadcastCurve();
        case TargetCurve::Vinyl:
            return TargetCurveLibrary::getVinylCurve();
        case TargetCurve::Streaming:
            return TargetCurveLibrary::getStreamingCurve();
        case TargetCurve::Flat:
        case TargetCurve::Custom:
        default:
            return TargetCurveLibrary::getFlatCurve();
    }
}

float TonalBalanceAnalyzer::calculateDifferenceScore() const {
    if (currentResult_.difference.empty()) return 0.0f;
    
    // Calculate RMS difference
    float sumSquares = 0.0f;
    for (float diff : currentResult_.difference) {
        sumSquares += diff * diff;
    }
    float rmsDiff = std::sqrt(sumSquares / currentResult_.difference.size());
    
    // Convert to 0-100 scale (0dB diff = 100, 10dB diff = 0)
    float score = 100.0f - (rmsDiff * 10.0f);
    return juce::jlimit(0.0f, 100.0f, score);
}

juce::String TonalBalanceAnalyzer::getTonalBalanceAssessment() const {
    float score = currentResult_.overallScore;
    
    if (score >= 90.0f) return "Excellent balance";
    if (score >= 75.0f) return "Good balance";
    if (score >= 60.0f) return "Fair balance";
    if (score >= 40.0f) return "Needs adjustment";
    return "Poor balance";
}

void TonalBalanceAnalyzer::processBlock(const juce::AudioBuffer<float>& buffer) {
    sampleCounter_ += buffer.getNumSamples();
    
    if (sampleCounter_ >= samplesPerUpdate_) {
        sampleCounter_ = 0;
        currentResult_ = analyze(buffer);
    }
}

// ============================================================================
// TonalBalanceEQSuggester Implementation
// ============================================================================

TonalBalanceEQSuggester::TonalBalanceEQSuggester() {}

std::vector<TonalBalanceEQSuggester::EQBand> TonalBalanceEQSuggester::suggestEQ(
    const TonalBalanceResult& result, float amount) {
    
    return generateMatchingEQ(result.octaveBandLevels, result.targetCurve, amount);
}

std::vector<TonalBalanceEQSuggester::EQBand> TonalBalanceEQSuggester::generateMatchingEQ(
    const std::vector<float>& current, const std::vector<float>& target, float amount) {
    
    std::vector<EQBand> bands;
    
    if (current.size() != target.size()) return bands;
    
    // Find peaks in difference curve
    std::vector<float> diff(current.size());
    for (size_t i = 0; i < current.size(); ++i) {
        diff[i] = (target[i] - current[i]) * amount;
    }
    
    // Create EQ bands for significant differences
    auto octaveBands = TonalBalanceAnalyzer::getStandardOctaveBands();
    
    for (size_t i = 1; i < diff.size() - 1 && static_cast<int>(bands.size()) < maxBands_; ++i) {
        if (std::abs(diff[i]) > 2.0f) { // >2dB difference
            EQBand band;
            band.frequency = octaveBands[i].centerFreq;
            band.gain = juce::jlimit(minGain_, maxGain_, diff[i]);
            band.Q = 1.0f;
            
            if (i == 0 || i == diff.size() - 1) {
                band.type = "shelf";
            } else {
                band.type = "bell";
            }
            
            bands.push_back(band);
        }
    }
    
    return bands;
}

// ============================================================================
// SmoothedSpectrum Implementation
// ============================================================================

SmoothedSpectrum::SmoothedSpectrum() : fft_(11),
    window_(2048, juce::dsp::WindowingFunction<float>::hann) {
    initialize(2048, 48000.0);
}

void SmoothedSpectrum::initialize(int fftSize, double sampleRate) {
    fftSize_ = fftSize;
    sampleRate_ = sampleRate;
    
    int order = static_cast<int>(std::log2(fftSize));
    fft_ = juce::dsp::FFT(order);
    window_.~WindowingFunction();
    new (&window_) juce::dsp::WindowingFunction<float>(fftSize, juce::dsp::WindowingFunction<float>::hann);
    
    fftData_.resize(fftSize * 2, 0.0f);
    smoothedMagnitudes_.resize(fftSize / 2, 0.0f);
}

void SmoothedSpectrum::process(const juce::AudioBuffer<float>& buffer) {
    performFFT(buffer);
}

void SmoothedSpectrum::performFFT(const juce::AudioBuffer<float>& buffer) {
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    
    const int numSamples = std::min(buffer.getNumSamples(), fftSize_);
    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            sample += buffer.getSample(ch, i);
        }
        fftData_[i] = sample / buffer.getNumChannels();
    }
    
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize_);
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Smooth magnitudes
    for (size_t i = 0; i < smoothedMagnitudes_.size(); ++i) {
        float current = fftData_[i];
        smoothedMagnitudes_[i] = smoothingFactor_ * smoothedMagnitudes_[i] + 
                                (1.0f - smoothingFactor_) * current;
    }
}

float SmoothedSpectrum::getMagnitudeAt(float frequency) const {
    int bin = static_cast<int>(frequency * fftSize_ / sampleRate_);
    bin = juce::jlimit(0, static_cast<int>(smoothedMagnitudes_.size()) - 1, bin);
    return smoothedMagnitudes_[bin];
}

void SmoothedSpectrum::clear() {
    std::fill(smoothedMagnitudes_.begin(), smoothedMagnitudes_.end(), 0.0f);
}

// ============================================================================
// TargetCurveLibrary Implementation
// ============================================================================

std::vector<float> TargetCurveLibrary::interpolateCurve(
    const std::vector<std::pair<float, float>>& points, int numBands) {
    
    std::vector<float> curve(numBands, 0.0f);
    auto bands = TonalBalanceAnalyzer::getStandardOctaveBands();
    
    for (int i = 0; i < numBands && i < static_cast<int>(bands.size()); ++i) {
        float freq = bands[i].centerFreq;
        
        // Find surrounding points
        for (size_t j = 0; j < points.size() - 1; ++j) {
            if (freq >= points[j].first && freq <= points[j + 1].first) {
                float t = (freq - points[j].first) / (points[j + 1].first - points[j].first);
                curve[i] = points[j].second + t * (points[j + 1].second - points[j].second);
                break;
            }
        }
    }
    
    return curve;
}

std::vector<float> TargetCurveLibrary::getFlatCurve() {
    return std::vector<float>(28, 0.0f); // All bands at 0dB
}

std::vector<float> TargetCurveLibrary::getHarmanCurve() {
    // Harman target: slight low boost, neutral mids, gentle high roll-off
    std::vector<std::pair<float, float>> points = {
        {20.0f, 4.0f}, {100.0f, 2.0f}, {1000.0f, 0.0f}, 
        {5000.0f, 0.0f}, {10000.0f, -2.0f}, {20000.0f, -4.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getStudioCurve() {
    // Studio monitoring: flat with slight high roll-off
    std::vector<std::pair<float, float>> points = {
        {20.0f, 0.0f}, {1000.0f, 0.0f}, {10000.0f, -1.0f}, {20000.0f, -2.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getBroadcastCurve() {
    // Broadcast: gentle low/high roll-off
    std::vector<std::pair<float, float>> points = {
        {20.0f, -2.0f}, {100.0f, 0.0f}, {1000.0f, 0.0f}, 
        {10000.0f, -1.0f}, {20000.0f, -3.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getVinylCurve() {
    // Vinyl: reduced lows, boosted highs (RIAA-inspired)
    std::vector<std::pair<float, float>> points = {
        {20.0f, -6.0f}, {100.0f, -2.0f}, {1000.0f, 0.0f}, 
        {5000.0f, 2.0f}, {10000.0f, 3.0f}, {20000.0f, 2.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getStreamingCurve() {
    // Streaming: slight V-curve for consumer playback
    std::vector<std::pair<float, float>> points = {
        {20.0f, 3.0f}, {100.0f, 1.0f}, {1000.0f, 0.0f}, 
        {5000.0f, 1.0f}, {10000.0f, 2.0f}, {20000.0f, 0.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getHipHopCurve() {
    // Hip-hop: enhanced sub and low end
    std::vector<std::pair<float, float>> points = {
        {20.0f, 6.0f}, {60.0f, 4.0f}, {200.0f, 1.0f}, 
        {1000.0f, 0.0f}, {10000.0f, 0.0f}, {20000.0f, -2.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getRockCurve() {
    // Rock: scooped mids
    std::vector<std::pair<float, float>> points = {
        {20.0f, 2.0f}, {100.0f, 1.0f}, {500.0f, -2.0f}, 
        {2000.0f, -1.0f}, {5000.0f, 2.0f}, {20000.0f, 1.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getClassicalCurve() {
    // Classical: natural, gentle roll-offs
    std::vector<std::pair<float, float>> points = {
        {20.0f, -1.0f}, {100.0f, 0.0f}, {1000.0f, 0.0f}, 
        {10000.0f, -0.5f}, {20000.0f, -1.0f}
    };
    return interpolateCurve(points, 28);
}

std::vector<float> TargetCurveLibrary::getElectronicCurve() {
    // Electronic: extended highs, strong lows
    std::vector<std::pair<float, float>> points = {
        {20.0f, 4.0f}, {100.0f, 2.0f}, {1000.0f, 0.0f}, 
        {5000.0f, 1.0f}, {10000.0f, 2.0f}, {20000.0f, 1.0f}
    };
    return interpolateCurve(points, 28);
}

// ============================================================================
// TonalBalanceComparer Implementation
// ============================================================================

TonalBalanceComparer::TonalBalanceComparer() {
    initialize(48000.0);
}

void TonalBalanceComparer::initialize(double sampleRate) {
    analyzer_.initialize(sampleRate);
}

void TonalBalanceComparer::setReferenceSpectrum(const TonalBalanceResult& result) {
    referenceResult_ = result;
    hasReference_ = true;
}

void TonalBalanceComparer::setReferenceFromBuffer(const juce::AudioBuffer<float>& buffer) {
    referenceResult_ = analyzer_.analyze(buffer);
    hasReference_ = true;
}

std::vector<float> TonalBalanceComparer::compare(const TonalBalanceResult& current) {
    if (!hasReference_) return std::vector<float>();
    
    difference_.resize(current.octaveBandLevels.size());
    for (size_t i = 0; i < difference_.size(); ++i) {
        difference_[i] = current.octaveBandLevels[i] - referenceResult_.octaveBandLevels[i];
    }
    
    return difference_;
}

float TonalBalanceComparer::getSimilarityScore() const {
    if (difference_.empty()) return 0.0f;
    
    float sumSquares = 0.0f;
    for (float diff : difference_) {
        sumSquares += diff * diff;
    }
    float rmsDiff = std::sqrt(sumSquares / difference_.size());
    
    // 0dB diff = 100, 10dB diff = 0
    float score = 100.0f - (rmsDiff * 10.0f);
    return juce::jlimit(0.0f, 100.0f, score);
}

} // namespace Analysis
} // namespace omega
