/*
  ==============================================================================

    DynamicRangeAnalyzer.h
    DR14, PLR, Crest Factor analysis and histogram visualization

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <algorithm>

namespace omega {
namespace Analysis {

/**
 * @brief Dynamic range analysis result
 */
struct DynamicRangeResult {
    float dr14 = 0.0f;              // DR14 dynamic range (dB)
    float plr = 0.0f;               // Peak to Loudness Ratio (dB)
    float crestFactor = 0.0f;       // Crest factor (dB)
    float peakLevel = -100.0f;      // Peak level (dBFS)
    float rmsLevel = -100.0f;       // RMS level (dBFS)
    float lufs = -23.0f;            // Integrated LUFS
    
    // Histogram data
    std::vector<int> histogram;     // Level distribution (bins)
    int histogramBins = 100;
    
    DynamicRangeResult() {
        histogram.resize(100, 0);
    }
};

/**
 * @brief DR14 (Dynamic Range) analyzer
 * Implements ITU-R BS.1770 loudness measurement and DR14 standard
 */
class DynamicRangeAnalyzer {
public:
    DynamicRangeAnalyzer();
    ~DynamicRangeAnalyzer() = default;
    
    void initialize(double sampleRate);
    void setSampleRate(double newSampleRate);
    
    // Analysis
    DynamicRangeResult analyze(const juce::AudioBuffer<float>& buffer);
    DynamicRangeResult analyzeFile(const juce::File& audioFile);
    void reset();
    
    // Real-time analysis
    void processBlock(const juce::AudioBuffer<float>& buffer);
    DynamicRangeResult getCurrentResult() const { return currentResult_; }
    
    // Settings
    void setBlockSize(int samples) { blockSize_ = samples; }
    void setHistogramBins(int bins);
    void setAnalysisMode(int mode) { analysisMode_ = mode; } // 0=DR14, 1=PLR, 2=Crest
    
    // DR14 calculation
    static float calculateDR14(const std::vector<float>& rmsValues);
    static float calculatePLR(float peakDb, float lufsDb);
    static float calculateCrestFactor(float peakDb, float rmsDb);
    
private:
    void analyzeBlockForDR14(const juce::AudioBuffer<float>& buffer);
    float calculateLUFS(const juce::AudioBuffer<float>& buffer);
    void updateHistogram(const juce::AudioBuffer<float>& buffer);
    
    double sampleRate_ = 48000.0;
    int blockSize_ = 4800; // 100ms at 48kHz
    int analysisMode_ = 0;
    
    // Current analysis state
    DynamicRangeResult currentResult_;
    std::vector<float> rmsBlockValues_;
    juce::AudioBuffer<float> accumulatedBuffer_;
    int accumulatedSamples_ = 0;
};

/**
 * @brief PLR (Peak to Loudness Ratio) meter
 * Measures the difference between peak and perceived loudness
 */
class PLRMeter {
public:
    PLRMeter();
    ~PLRMeter() = default;
    
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Get results
    float getPLR() const { return plr_; }
    float getTruePeak() const { return truePeak_; }
    float getLUFS() const { return lufs_; }
    
    // Targets for different uses
    static float getTargetPLR(const juce::String& useCase);
    
private:
    double sampleRate_ = 48000.0;
    float plr_ = 0.0f;
    float truePeak_ = -100.0f;
    float lufs_ = -23.0f;
    
    // Accumulation
    juce::AudioBuffer<float> buffer_;
};

/**
 * @brief Crest factor analyzer
 * Measures peak-to-average ratio
 */
class CrestFactorAnalyzer {
public:
    CrestFactorAnalyzer();
    ~CrestFactorAnalyzer() = default;
    
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Get results
    float getCrestFactor() const { return crestFactor_; }
    float getPeak() const { return peak_; }
    float getRMS() const { return rms_; }
    
    // Settings
    void setWindowSize(float ms);
    void setIntegrationTime(float ms);
    
private:
    double sampleRate_ = 48000.0;
    float crestFactor_ = 0.0f;
    float peak_ = 0.0f;
    float rms_ = 0.0f;
    
    int windowSamples_ = 4800;
    std::vector<float> peakHistory_;
    std::vector<float> rmsHistory_;
    int historyIndex_ = 0;
};

/**
 * @brief Level histogram for visual analysis
 */
class LevelHistogram {
public:
    LevelHistogram();
    ~LevelHistogram() = default;
    
    void initialize(int bins = 100, float minDb = -80.0f, float maxDb = 0.0f);
    void clear();
    
    // Processing
    void process(const juce::AudioBuffer<float>& buffer);
    void addSample(float levelDb);
    
    // Get data
    const std::vector<int>& getHistogram() const { return bins_; }
    int getTotalSamples() const { return totalSamples_; }
    int getBinForDb(float db) const;
    float getDbForBin(int bin) const;
    
    // Statistics
    float getPercentile(float percentile) const; // 0-100
    float getMean() const;
    float getMode() const; // Most common level
    
private:
    int numBins_ = 100;
    float minDb_ = -80.0f;
    float maxDb_ = 0.0f;
    std::vector<int> bins_;
    int totalSamples_ = 0;
};

/**
 * @brief Integrated dynamic range suite
 * Combines all dynamic range analysis tools
 */
class DynamicRangeSuite {
public:
    DynamicRangeSuite();
    ~DynamicRangeSuite() = default;
    
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Get analyzers
    const DynamicRangeAnalyzer& getDRAnalyzer() const { return drAnalyzer_; }
    const PLRMeter& getPLRMeter() const { return plrMeter_; }
    const CrestFactorAnalyzer& getCrestAnalyzer() const { return crestAnalyzer_; }
    const LevelHistogram& getHistogram() const { return histogram_; }
    
    // Quick access to common metrics
    float getDR14() const { return drAnalyzer_.getCurrentResult().dr14; }
    float getPLR() const { return plrMeter_.getPLR(); }
    float getCrestFactor() const { return crestAnalyzer_.getCrestFactor(); }
    
    // Assessment
    juce::String getDynamicRangeAssessment() const;
    bool isBrickwalled() const; // DR < 5dB
    bool isWellMastered() const; // DR 8-14dB
    
private:
    DynamicRangeAnalyzer drAnalyzer_;
    PLRMeter plrMeter_;
    CrestFactorAnalyzer crestAnalyzer_;
    LevelHistogram histogram_;
};

} // namespace Analysis
} // namespace omega
