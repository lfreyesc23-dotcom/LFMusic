/*
  ==============================================================================

    TonalBalance.h
    Tonal balance analyzer with target curves and 1/3 octave analysis

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>

namespace omega {
namespace Analysis {

/**
 * @brief Target curve type for tonal balance reference
 */
enum class TargetCurve {
    Flat,           // Flat response
    Harman,         // Harman target curve (consumer)
    Studio,         // Studio monitoring curve
    Broadcast,      // Broadcast standard
    Vinyl,          // Vinyl mastering
    Streaming,      // Streaming platform optimization
    Custom          // User-defined curve
};

/**
 * @brief 1/3 octave band definition
 */
struct OctaveBand {
    float centerFreq;   // Hz
    float lowerFreq;    // Hz
    float upperFreq;    // Hz
    juce::String name;  // e.g., "1kHz", "250Hz"
    
    OctaveBand() : centerFreq(1000.0f), lowerFreq(891.0f), upperFreq(1122.0f), name("1kHz") {}
    OctaveBand(float center) : centerFreq(center) {
        // 1/3 octave bandwidth
        lowerFreq = center / std::pow(2.0f, 1.0f/6.0f);
        upperFreq = center * std::pow(2.0f, 1.0f/6.0f);
        
        if (center < 1000.0f) {
            name = juce::String(static_cast<int>(center)) + "Hz";
        } else {
            name = juce::String(center / 1000.0f, 1) + "kHz";
        }
    }
};

/**
 * @brief Tonal balance analysis result
 */
struct TonalBalanceResult {
    std::vector<float> octaveBandLevels;   // dB per band
    std::vector<float> targetCurve;        // dB reference
    std::vector<float> difference;         // dB difference from target
    float overallScore = 0.0f;             // 0-100, how close to target
    float lowEnergy = 0.0f;                // <200Hz average (dB)
    float midEnergy = 0.0f;                // 200Hz-5kHz average (dB)
    float highEnergy = 0.0f;               // >5kHz average (dB)
    
    TonalBalanceResult() = default;
};

/**
 * @brief Tonal balance analyzer with target curve comparison
 */
class TonalBalanceAnalyzer {
public:
    TonalBalanceAnalyzer();
    ~TonalBalanceAnalyzer() = default;
    
    void initialize(double sampleRate, int fftSize = 8192);
    void setSampleRate(double newSampleRate);
    
    // Analysis
    TonalBalanceResult analyze(const juce::AudioBuffer<float>& buffer);
    void processBlock(const juce::AudioBuffer<float>& buffer);
    const TonalBalanceResult& getCurrentResult() const { return currentResult_; }
    
    // Target curves
    void setTargetCurve(TargetCurve curve);
    void setCustomCurve(const std::vector<float>& curve);
    TargetCurve getCurrentTargetCurve() const { return targetCurveType_; }
    
    // 1/3 octave bands
    static std::vector<OctaveBand> getStandardOctaveBands();
    static std::vector<OctaveBand> getExtendedOctaveBands(); // 20Hz-20kHz
    const std::vector<OctaveBand>& getBands() const { return bands_; }
    
    // Settings
    void setFFTSize(int size);
    void setSmoothingTime(float ms) { smoothingMs_ = ms; }
    void setReferenceLevel(float db) { referenceLevel_ = db; }
    
    // Comparison
    float calculateDifferenceScore() const; // 0-100
    juce::String getTonalBalanceAssessment() const;
    
private:
    void performOctaveBandAnalysis(const juce::AudioBuffer<float>& buffer);
    void calculateEnergyDistribution();
    std::vector<float> getTargetCurveData(TargetCurve curve) const;
    
    double sampleRate_ = 48000.0;
    int fftSize_ = 8192;
    TargetCurve targetCurveType_ = TargetCurve::Flat;
    
    // FFT
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;
    std::vector<float> fftData_;
    
    // Octave bands
    std::vector<OctaveBand> bands_;
    
    // Analysis state
    TonalBalanceResult currentResult_;
    float smoothingMs_ = 200.0f;
    float referenceLevel_ = -18.0f;
    
    // Update rate limiting
    int sampleCounter_ = 0;
    int samplesPerUpdate_ = 4800; // Update every 100ms at 48kHz
};

/**
 * @brief EQ suggestion generator based on tonal balance
 */
class TonalBalanceEQSuggester {
public:
    struct EQBand {
        float frequency;    // Hz
        float gain;         // dB
        float Q;            // Q factor
        juce::String type;  // "bell", "shelf"
        
        EQBand() : frequency(1000.0f), gain(0.0f), Q(1.0f), type("bell") {}
    };
    
    TonalBalanceEQSuggester();
    ~TonalBalanceEQSuggester() = default;
    
    // Generate EQ suggestions from tonal balance analysis
    std::vector<EQBand> suggestEQ(const TonalBalanceResult& result, float amount = 0.5f);
    
    // Generate EQ curve to match target
    std::vector<EQBand> generateMatchingEQ(const std::vector<float>& current,
                                          const std::vector<float>& target,
                                          float amount = 0.5f);
    
    // Settings
    void setMaxBands(int count) { maxBands_ = count; }
    void setMinGain(float db) { minGain_ = db; }
    void setMaxGain(float db) { maxGain_ = db; }
    
private:
    int maxBands_ = 8;
    float minGain_ = -6.0f;
    float maxGain_ = 6.0f;
};

/**
 * @brief Frequency spectrum with smoothing
 */
class SmoothedSpectrum {
public:
    SmoothedSpectrum();
    ~SmoothedSpectrum() = default;
    
    void initialize(int fftSize, double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    
    const std::vector<float>& getMagnitudes() const { return smoothedMagnitudes_; }
    float getMagnitudeAt(float frequency) const;
    
    void setSmoothingFactor(float factor) { smoothingFactor_ = juce::jlimit(0.0f, 1.0f, factor); }
    void clear();
    
private:
    void performFFT(const juce::AudioBuffer<float>& buffer);
    
    int fftSize_ = 2048;
    double sampleRate_ = 48000.0;
    
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;
    std::vector<float> fftData_;
    std::vector<float> smoothedMagnitudes_;
    float smoothingFactor_ = 0.7f;
};

/**
 * @brief A/B comparison of tonal balance
 */
class TonalBalanceComparer {
public:
    TonalBalanceComparer();
    ~TonalBalanceComparer() = default;
    
    void initialize(double sampleRate);
    
    // Set reference (A)
    void setReferenceSpectrum(const TonalBalanceResult& result);
    void setReferenceFromBuffer(const juce::AudioBuffer<float>& buffer);
    
    // Compare with current (B)
    std::vector<float> compare(const TonalBalanceResult& current);
    float getSimilarityScore() const; // 0-100
    
    // Get difference curve
    const std::vector<float>& getDifference() const { return difference_; }
    
private:
    TonalBalanceAnalyzer analyzer_;
    TonalBalanceResult referenceResult_;
    std::vector<float> difference_;
    bool hasReference_ = false;
};

/**
 * @brief Target curve library
 */
class TargetCurveLibrary {
public:
    static std::vector<float> getHarmanCurve();
    static std::vector<float> getStudioCurve();
    static std::vector<float> getBroadcastCurve();
    static std::vector<float> getVinylCurve();
    static std::vector<float> getStreamingCurve();
    static std::vector<float> getFlatCurve();
    
    // Genre-specific curves
    static std::vector<float> getHipHopCurve();   // Enhanced low end
    static std::vector<float> getRockCurve();     // Scooped mids
    static std::vector<float> getClassicalCurve(); // Natural balance
    static std::vector<float> getElectronicCurve(); // Enhanced highs
    
private:
    static std::vector<float> interpolateCurve(const std::vector<std::pair<float, float>>& points, int numBands);
};

} // namespace Analysis
} // namespace omega
