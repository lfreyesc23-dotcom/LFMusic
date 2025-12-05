/*
  ==============================================================================

    ReferenceMatching.h
    Reference track matching and analysis for mixing/mastering
    FFT spectral comparison, LUFS matching, frequency response analysis

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <array>

namespace omega {
namespace Analysis {

/**
 * @brief Spectral analysis result from FFT
 */
struct SpectrumData {
    std::vector<float> magnitudes;  // dB values
    std::vector<float> frequencies; // Hz values
    int fftSize = 2048;
    double sampleRate = 48000.0;
    
    SpectrumData() = default;
    SpectrumData(int size, double sr) : fftSize(size), sampleRate(sr) {
        magnitudes.resize(size / 2);
        frequencies.resize(size / 2);
        
        // Calculate frequency bins
        for (size_t i = 0; i < frequencies.size(); ++i) {
            frequencies[i] = static_cast<float>(i * sampleRate / fftSize);
        }
    }
    
    void clear() {
        std::fill(magnitudes.begin(), magnitudes.end(), -100.0f);
    }
};

/**
 * @brief Loudness analysis (LUFS)
 */
struct LoudnessData {
    float integratedLUFS = -23.0f;   // Overall loudness
    float shortTermLUFS = -23.0f;    // 3-second window
    float momentaryLUFS = -23.0f;    // 400ms window
    float truePeak = -6.0f;          // True peak dBTP
    float loudnessRange = 10.0f;     // LRA in LU
    
    LoudnessData() = default;
};

/**
 * @brief Frequency response comparison result
 */
struct FrequencyMatchResult {
    std::vector<float> differences;  // dB difference per band
    std::vector<float> targetEQ;     // Suggested EQ curve
    float overallDifference = 0.0f;  // RMS difference
    bool isGoodMatch = false;        // < 3dB RMS difference
    
    FrequencyMatchResult() = default;
};

/**
 * @brief Reference track matching engine
 */
class ReferenceTrackMatcher {
public:
    ReferenceTrackMatcher();
    ~ReferenceTrackMatcher() = default;
    
    // Setup
    void initialize(double sampleRate, int fftSize = 4096);
    void setSampleRate(double newSampleRate);
    
    // Reference track management
    void loadReferenceTrack(const juce::File& audioFile);
    void setReferenceBuffer(const juce::AudioBuffer<float>& buffer);
    void clearReference();
    bool hasReference() const { return hasReference_; }
    
    // Analysis
    SpectrumData analyzeSpectrum(const juce::AudioBuffer<float>& buffer, int startSample = 0, int numSamples = -1);
    LoudnessData analyzeLoudness(const juce::AudioBuffer<float>& buffer);
    FrequencyMatchResult compareToReference(const juce::AudioBuffer<float>& buffer);
    
    // Get reference data
    const SpectrumData& getReferenceSpectrum() const { return referenceSpectrum_; }
    const LoudnessData& getReferenceLoudness() const { return referenceLoudness_; }
    
    // Settings
    void setFFTSize(int size);
    void setAnalysisWindowType(juce::dsp::WindowingFunction<float>::WindowingMethod type) { windowType_ = type; }
    void setSmoothingFactor(float factor) { smoothingFactor_ = juce::jlimit(0.0f, 1.0f, factor); }
    
    // A/B comparison
    void enableABComparison(bool enable) { abComparisonEnabled_ = enable; }
    bool isABEnabled() const { return abComparisonEnabled_; }
    void setABMode(bool useReference) { currentlyListeningToReference_ = useReference; }
    bool isListeningToReference() const { return currentlyListeningToReference_; }
    
private:
    void analyzeReferenceTrack();
    void performFFT(const juce::AudioBuffer<float>& buffer, SpectrumData& result, int startSample, int numSamples);
    float calculateLUFS(const juce::AudioBuffer<float>& buffer);
    float calculateTruePeak(const juce::AudioBuffer<float>& buffer);
    void applyKWeighting(std::vector<float>& spectrum);
    
    double sampleRate_ = 48000.0;
    int fftSize_ = 4096;
    
    // FFT
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;
    juce::dsp::WindowingFunction<float>::WindowingMethod windowType_;
    std::vector<float> fftData_;
    
    // Reference track
    juce::AudioBuffer<float> referenceBuffer_;
    bool hasReference_ = false;
    SpectrumData referenceSpectrum_;
    LoudnessData referenceLoudness_;
    
    // Analysis settings
    float smoothingFactor_ = 0.8f;  // 0 = no smoothing, 1 = maximum smoothing
    
    // A/B comparison
    bool abComparisonEnabled_ = false;
    bool currentlyListeningToReference_ = false;
};

/**
 * @brief Real-time reference matching processor
 * Continuously compares input to reference and provides visual feedback
 */
class ReferenceMatchingProcessor {
public:
    ReferenceMatchingProcessor();
    ~ReferenceMatchingProcessor() = default;
    
    void initialize(double sampleRate);
    void setReferenceFile(const juce::File& file);
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Get current analysis
    const SpectrumData& getCurrentSpectrum() const { return currentSpectrum_; }
    const SpectrumData& getReferenceSpectrum() const { return matcher_.getReferenceSpectrum(); }
    const FrequencyMatchResult& getMatchResult() const { return matchResult_; }
    
    // Auto-matching (apply EQ to match reference)
    void enableAutoMatch(bool enable) { autoMatchEnabled_ = enable; }
    bool isAutoMatchEnabled() const { return autoMatchEnabled_; }
    float getAutoMatchAmount() const { return autoMatchAmount_; }
    void setAutoMatchAmount(float amount) { autoMatchAmount_ = juce::jlimit(0.0f, 1.0f, amount); }
    
    // LUFS matching
    void enableLUFSMatch(bool enable) { lufsMatchEnabled_ = enable; }
    void setTargetLUFS(float target) { targetLUFS_ = target; }
    float getCurrentLUFS() const { return currentLoudness_.integratedLUFS; }
    float getTargetLUFS() const { return targetLUFS_; }
    
private:
    void applyAutoMatch(juce::AudioBuffer<float>& buffer);
    void applyLUFSMatch(juce::AudioBuffer<float>& buffer);
    
    ReferenceTrackMatcher matcher_;
    SpectrumData currentSpectrum_;
    LoudnessData currentLoudness_;
    FrequencyMatchResult matchResult_;
    
    // Auto-matching
    bool autoMatchEnabled_ = false;
    float autoMatchAmount_ = 0.5f;
    std::array<juce::dsp::IIR::Filter<float>, 10> matchingEQs_; // 10-band EQ for matching
    
    // LUFS matching
    bool lufsMatchEnabled_ = false;
    float targetLUFS_ = -14.0f;
    float currentGain_ = 1.0f;
    
    // Analysis rate limiting
    int analysisSampleCounter_ = 0;
    int analysisSampleInterval_ = 4800; // Analyze every 100ms at 48kHz
};

/**
 * @brief Reference library manager
 * Store and organize reference tracks by genre/style
 */
class ReferenceLibrary {
public:
    struct ReferenceTrack {
        juce::String name;
        juce::String artist;
        juce::String genre;
        juce::File file;
        float integratedLUFS = -14.0f;
        float truePeak = -1.0f;
        juce::String notes;
        
        ReferenceTrack() = default;
    };
    
    void addReference(const ReferenceTrack& ref);
    void removeReference(int index);
    void clear();
    
    int getNumReferences() const { return static_cast<int>(references_.size()); }
    const ReferenceTrack& getReference(int index) const { return references_[index]; }
    
    // Search
    std::vector<int> findByGenre(const juce::String& genre) const;
    std::vector<int> findByArtist(const juce::String& artist) const;
    std::vector<int> findByLoudness(float minLUFS, float maxLUFS) const;
    
    // Persistence
    void saveToFile(const juce::File& file);
    void loadFromFile(const juce::File& file);
    
private:
    std::vector<ReferenceTrack> references_;
};

} // namespace Analysis
} // namespace omega
