/*
  ==============================================================================

    DynamicRangeAnalyzer.cpp
    Implementation of DR14, PLR, and dynamic range analysis

  ==============================================================================
*/

#include "DynamicRangeAnalyzer.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace omega {
namespace Analysis {

// ============================================================================
// DynamicRangeAnalyzer Implementation
// ============================================================================

DynamicRangeAnalyzer::DynamicRangeAnalyzer() {
    initialize(48000.0);
}

void DynamicRangeAnalyzer::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
    blockSize_ = static_cast<int>(sampleRate * 0.1); // 100ms blocks
    reset();
}

void DynamicRangeAnalyzer::setSampleRate(double newSampleRate) {
    sampleRate_ = newSampleRate;
    blockSize_ = static_cast<int>(sampleRate_ * 0.1);
}

void DynamicRangeAnalyzer::reset() {
    currentResult_ = DynamicRangeResult();
    rmsBlockValues_.clear();
    accumulatedSamples_ = 0;
}

void DynamicRangeAnalyzer::setHistogramBins(int bins) {
    currentResult_.histogramBins = bins;
    currentResult_.histogram.resize(bins, 0);
}

DynamicRangeResult DynamicRangeAnalyzer::analyze(const juce::AudioBuffer<float>& buffer) {
    reset();
    
    // Calculate peak
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
    }
    currentResult_.peakLevel = peak > 0.0f ? 20.0f * std::log10(peak) : -100.0f;
    
    // Calculate RMS
    double sumSquares = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
        }
    }
    double rms = std::sqrt(sumSquares / (buffer.getNumChannels() * buffer.getNumSamples()));
    currentResult_.rmsLevel = rms > 0.0 ? 20.0f * std::log10(static_cast<float>(rms)) : -100.0f;
    
    // Calculate LUFS
    currentResult_.lufs = calculateLUFS(buffer);
    
    // Analyze in blocks for DR14
    const int blockSamples = blockSize_;
    for (int startSample = 0; startSample < buffer.getNumSamples(); startSample += blockSamples) {
        int samplesToProcess = std::min(blockSamples, buffer.getNumSamples() - startSample);
        
        // Create sub-buffer for this block
        juce::AudioBuffer<float> block(buffer.getNumChannels(), samplesToProcess);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            block.copyFrom(ch, 0, buffer, ch, startSample, samplesToProcess);
        }
        
        analyzeBlockForDR14(block);
    }
    
    // Calculate DR14
    currentResult_.dr14 = calculateDR14(rmsBlockValues_);
    
    // Calculate PLR
    currentResult_.plr = calculatePLR(currentResult_.peakLevel, currentResult_.lufs);
    
    // Calculate Crest Factor
    currentResult_.crestFactor = calculateCrestFactor(currentResult_.peakLevel, currentResult_.rmsLevel);
    
    // Update histogram
    updateHistogram(buffer);
    
    return currentResult_;
}

void DynamicRangeAnalyzer::analyzeBlockForDR14(const juce::AudioBuffer<float>& buffer) {
    // Calculate RMS for this block
    double sumSquares = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
        }
    }
    
    double rms = std::sqrt(sumSquares / (buffer.getNumChannels() * buffer.getNumSamples()));
    float rmsDb = rms > 0.0 ? 20.0f * std::log10(static_cast<float>(rms)) : -100.0f;
    
    rmsBlockValues_.push_back(rmsDb);
}

float DynamicRangeAnalyzer::calculateDR14(const std::vector<float>& rmsValues) {
    if (rmsValues.empty()) return 0.0f;
    
    // Sort RMS values
    std::vector<float> sorted = rmsValues;
    std::sort(sorted.begin(), sorted.end(), std::greater<float>());
    
    // Take top 20% (loudest blocks)
    size_t topCount = std::max(size_t(1), sorted.size() / 5);
    float sumTop = 0.0f;
    for (size_t i = 0; i < topCount; ++i) {
        sumTop += sorted[i];
    }
    float peak20 = sumTop / topCount;
    
    // Take average of all blocks
    float sumAll = std::accumulate(sorted.begin(), sorted.end(), 0.0f);
    float average = sumAll / sorted.size();
    
    // DR14 = difference between peak 20% and average
    float dr = peak20 - average;
    
    return std::max(0.0f, dr);
}

float DynamicRangeAnalyzer::calculatePLR(float peakDb, float lufsDb) {
    // PLR = True Peak (dBTP) - Integrated Loudness (LUFS)
    return peakDb - lufsDb;
}

float DynamicRangeAnalyzer::calculateCrestFactor(float peakDb, float rmsDb) {
    // Crest Factor = Peak - RMS (in dB)
    return peakDb - rmsDb;
}

float DynamicRangeAnalyzer::calculateLUFS(const juce::AudioBuffer<float>& buffer) {
    // Simplified LUFS calculation
    double sumSquares = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
        }
    }
    
    double rms = std::sqrt(sumSquares / (buffer.getNumChannels() * buffer.getNumSamples()));
    float lufs = -23.0f + 20.0f * std::log10(static_cast<float>(rms) + 1e-10f);
    
    return juce::jlimit(-70.0f, 0.0f, lufs);
}

void DynamicRangeAnalyzer::updateHistogram(const juce::AudioBuffer<float>& buffer) {
    const int numBins = currentResult_.histogramBins;
    const float minDb = -80.0f;
    const float maxDb = 0.0f;
    const float dbRange = maxDb - minDb;
    
    currentResult_.histogram.clear();
    currentResult_.histogram.resize(numBins, 0);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(data[i]);
            float db = sample > 0.0f ? 20.0f * std::log10(sample) : minDb;
            
            int bin = static_cast<int>((db - minDb) / dbRange * (numBins - 1));
            bin = juce::jlimit(0, numBins - 1, bin);
            
            currentResult_.histogram[bin]++;
        }
    }
}

DynamicRangeResult DynamicRangeAnalyzer::analyzeFile(const juce::File& audioFile) {
    if (!audioFile.existsAsFile()) return DynamicRangeResult();
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto* reader = formatManager.createReaderFor(audioFile);
    if (reader == nullptr) return DynamicRangeResult();
    
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels), 
                                   static_cast<int>(reader->lengthInSamples));
    reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    
    delete reader;
    
    return analyze(buffer);
}

void DynamicRangeAnalyzer::processBlock(const juce::AudioBuffer<float>& buffer) {
    // Accumulate samples for batch analysis
    if (accumulatedBuffer_.getNumSamples() == 0) {
        accumulatedBuffer_.setSize(buffer.getNumChannels(), blockSize_ * 100);
    }
    
    int samplesToAdd = std::min(buffer.getNumSamples(), 
                                accumulatedBuffer_.getNumSamples() - accumulatedSamples_);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        accumulatedBuffer_.copyFrom(ch, accumulatedSamples_, buffer, ch, 0, samplesToAdd);
    }
    
    accumulatedSamples_ += samplesToAdd;
    
    // Analyze when we have enough data
    if (accumulatedSamples_ >= blockSize_) {
        juce::AudioBuffer<float> analysisBuffer(accumulatedBuffer_.getNumChannels(), accumulatedSamples_);
        for (int ch = 0; ch < accumulatedBuffer_.getNumChannels(); ++ch) {
            analysisBuffer.copyFrom(ch, 0, accumulatedBuffer_, ch, 0, accumulatedSamples_);
        }
        
        currentResult_ = analyze(analysisBuffer);
        accumulatedSamples_ = 0;
    }
}

// ============================================================================
// PLRMeter Implementation
// ============================================================================

PLRMeter::PLRMeter() {
    initialize(48000.0);
}

void PLRMeter::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
    reset();
}

void PLRMeter::reset() {
    plr_ = 0.0f;
    truePeak_ = -100.0f;
    lufs_ = -23.0f;
}

void PLRMeter::process(const juce::AudioBuffer<float>& buffer) {
    // Calculate true peak
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
    }
    truePeak_ = peak > 0.0f ? 20.0f * std::log10(peak) : -100.0f;
    
    // Calculate LUFS (simplified)
    double sumSquares = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
        }
    }
    double rms = std::sqrt(sumSquares / (buffer.getNumChannels() * buffer.getNumSamples()));
    lufs_ = -23.0f + 20.0f * std::log10(static_cast<float>(rms) + 1e-10f);
    lufs_ = juce::jlimit(-70.0f, 0.0f, lufs_);
    
    // Calculate PLR
    plr_ = truePeak_ - lufs_;
}

float PLRMeter::getTargetPLR(const juce::String& useCase) {
    if (useCase == "streaming") return 12.0f;  // Spotify, Apple Music
    if (useCase == "broadcast") return 10.0f;  // TV/Radio
    if (useCase == "cd") return 8.0f;          // CD mastering
    if (useCase == "vinyl") return 14.0f;      // Vinyl cutting
    return 10.0f; // Default
}

// ============================================================================
// CrestFactorAnalyzer Implementation
// ============================================================================

CrestFactorAnalyzer::CrestFactorAnalyzer() {
    initialize(48000.0);
}

void CrestFactorAnalyzer::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
    setWindowSize(100.0f); // 100ms default
}

void CrestFactorAnalyzer::reset() {
    crestFactor_ = 0.0f;
    peak_ = 0.0f;
    rms_ = 0.0f;
    std::fill(peakHistory_.begin(), peakHistory_.end(), 0.0f);
    std::fill(rmsHistory_.begin(), rmsHistory_.end(), 0.0f);
    historyIndex_ = 0;
}

void CrestFactorAnalyzer::setWindowSize(float ms) {
    windowSamples_ = static_cast<int>(sampleRate_ * ms / 1000.0f);
    int historySize = std::max(10, windowSamples_ / 100);
    peakHistory_.resize(historySize, 0.0f);
    rmsHistory_.resize(historySize, 0.0f);
}

void CrestFactorAnalyzer::process(const juce::AudioBuffer<float>& buffer) {
    // Calculate peak
    float blockPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            blockPeak = std::max(blockPeak, std::abs(data[i]));
        }
    }
    
    // Calculate RMS
    double sumSquares = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            sumSquares += data[i] * data[i];
        }
    }
    float blockRms = static_cast<float>(std::sqrt(sumSquares / (buffer.getNumChannels() * buffer.getNumSamples())));
    
    // Update history
    peakHistory_[historyIndex_] = blockPeak;
    rmsHistory_[historyIndex_] = blockRms;
    historyIndex_ = (historyIndex_ + 1) % static_cast<int>(peakHistory_.size());
    
    // Calculate averages
    float avgPeak = std::accumulate(peakHistory_.begin(), peakHistory_.end(), 0.0f) / peakHistory_.size();
    float avgRms = std::accumulate(rmsHistory_.begin(), rmsHistory_.end(), 0.0f) / rmsHistory_.size();
    
    peak_ = avgPeak > 0.0f ? 20.0f * std::log10(avgPeak) : -100.0f;
    rms_ = avgRms > 0.0f ? 20.0f * std::log10(avgRms) : -100.0f;
    crestFactor_ = peak_ - rms_;
}

// ============================================================================
// LevelHistogram Implementation
// ============================================================================

LevelHistogram::LevelHistogram() {
    initialize();
}

void LevelHistogram::initialize(int bins, float minDb, float maxDb) {
    numBins_ = bins;
    minDb_ = minDb;
    maxDb_ = maxDb;
    bins_.resize(bins, 0);
    totalSamples_ = 0;
}

void LevelHistogram::clear() {
    std::fill(bins_.begin(), bins_.end(), 0);
    totalSamples_ = 0;
}

void LevelHistogram::process(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = std::abs(data[i]);
            float db = sample > 0.0f ? 20.0f * std::log10(sample) : minDb_;
            addSample(db);
        }
    }
}

void LevelHistogram::addSample(float levelDb) {
    int bin = getBinForDb(levelDb);
    if (bin >= 0 && bin < numBins_) {
        bins_[bin]++;
        totalSamples_++;
    }
}

int LevelHistogram::getBinForDb(float db) const {
    float normalized = (db - minDb_) / (maxDb_ - minDb_);
    int bin = static_cast<int>(normalized * (numBins_ - 1));
    return juce::jlimit(0, numBins_ - 1, bin);
}

float LevelHistogram::getDbForBin(int bin) const {
    float normalized = static_cast<float>(bin) / (numBins_ - 1);
    return minDb_ + normalized * (maxDb_ - minDb_);
}

float LevelHistogram::getPercentile(float percentile) const {
    if (totalSamples_ == 0) return minDb_;
    
    int targetSamples = static_cast<int>(totalSamples_ * percentile / 100.0f);
    int accumulated = 0;
    
    for (int i = 0; i < numBins_; ++i) {
        accumulated += bins_[i];
        if (accumulated >= targetSamples) {
            return getDbForBin(i);
        }
    }
    
    return maxDb_;
}

float LevelHistogram::getMean() const {
    if (totalSamples_ == 0) return minDb_;
    
    double sum = 0.0;
    for (int i = 0; i < numBins_; ++i) {
        sum += getDbForBin(i) * bins_[i];
    }
    
    return static_cast<float>(sum / totalSamples_);
}

float LevelHistogram::getMode() const {
    auto maxIt = std::max_element(bins_.begin(), bins_.end());
    int maxBin = static_cast<int>(std::distance(bins_.begin(), maxIt));
    return getDbForBin(maxBin);
}

// ============================================================================
// DynamicRangeSuite Implementation
// ============================================================================

DynamicRangeSuite::DynamicRangeSuite() {
    initialize(48000.0);
}

void DynamicRangeSuite::initialize(double sampleRate) {
    drAnalyzer_.initialize(sampleRate);
    plrMeter_.initialize(sampleRate);
    crestAnalyzer_.initialize(sampleRate);
    histogram_.initialize(100, -80.0f, 0.0f);
}

void DynamicRangeSuite::process(const juce::AudioBuffer<float>& buffer) {
    drAnalyzer_.processBlock(buffer);
    plrMeter_.process(buffer);
    crestAnalyzer_.process(buffer);
    histogram_.process(buffer);
}

void DynamicRangeSuite::reset() {
    drAnalyzer_.reset();
    plrMeter_.reset();
    crestAnalyzer_.reset();
    histogram_.clear();
}

juce::String DynamicRangeSuite::getDynamicRangeAssessment() const {
    float dr = getDR14();
    
    if (dr < 5.0f) return "Heavily compressed / Brickwalled";
    if (dr < 8.0f) return "Compressed";
    if (dr < 14.0f) return "Well-mastered";
    if (dr < 20.0f) return "Dynamic";
    return "Very dynamic";
}

bool DynamicRangeSuite::isBrickwalled() const {
    return getDR14() < 5.0f && getCrestFactor() < 6.0f;
}

bool DynamicRangeSuite::isWellMastered() const {
    float dr = getDR14();
    float plr = getPLR();
    return (dr >= 8.0f && dr <= 14.0f) && (plr >= 8.0f && plr <= 14.0f);
}

} // namespace Analysis
} // namespace omega
