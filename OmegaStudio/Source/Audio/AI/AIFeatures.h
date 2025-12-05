#pragma once

#include <JuceHeader.h>
#include <vector>
#include <string>
#include <memory>

namespace omega {
namespace AI {

/**
 * @brief AI Beat Generator - Text-to-beat generation
 */
class BeatGenerator {
public:
    struct BeatPattern {
        juce::String name;
        juce::String genre;          // "Trap", "Drill", "House", etc.
        int bpm;
        std::vector<int> kickPattern;
        std::vector<int> snarePattern;
        std::vector<int> hihatPattern;
        std::vector<int> pattern808;
        
        BeatPattern() : bpm(140) {}
    };
    
    BeatGenerator();
    ~BeatGenerator();
    
    // Text-to-beat generation
    BeatPattern generateFromText(const juce::String& prompt);  // "hard drill beat 140 bpm"
    BeatPattern generateFromGenre(const juce::String& genre, int bpm, const juce::String& mood = "");
    
    // Pattern variations
    BeatPattern addVariation(const BeatPattern& base, float amount = 0.3f);
    BeatPattern humanize(const BeatPattern& pattern, float amount = 0.2f);
    
private:
    struct GenreTemplate {
        juce::String name;
        std::vector<int> kickBase;
        std::vector<int> snareBase;
        std::vector<int> hihatBase;
        int defaultBpm;
    };
    
    std::map<juce::String, GenreTemplate> templates_;
    void initializeTemplates();
    
    GenreTemplate parsePrompt(const juce::String& prompt);
};

/**
 * @brief AI Melody Generator - Scale-aware composition
 */
class MelodyGenerator {
public:
    struct Melody {
        std::vector<int> notes;          // MIDI notes
        std::vector<double> durations;   // Beats
        std::vector<float> velocities;   // 0.0-1.0
        juce::String scale;
        int rootNote;
        
        Melody() : rootNote(60) {}
    };
    
    MelodyGenerator();
    ~MelodyGenerator();
    
    // Generate melody from parameters
    Melody generate(const juce::String& scale, int rootNote, int numBars, const juce::String& mood = "happy");
    
    // Chord progression-based generation
    Melody generateFromChords(const std::vector<juce::String>& chordProgression, int rootNote);
    
    // Variation
    Melody addVariation(const Melody& base);
    Melody transpose(const Melody& base, int semitones);
    
private:
    std::vector<int> getScaleNotes(const juce::String& scaleName, int root);
    int selectNextNote(int currentNote, const std::vector<int>& scale, const juce::String& mood);
};

/**
 * @brief AI Mix Assistant - Real-time mixing suggestions
 */
class MixAssistant {
public:
    struct Suggestion {
        juce::String type;        // "EQ", "Compression", "Level", "Pan"
        juce::String description;
        int channelId;
        float severity;           // 0.0-1.0 (how urgent)
        juce::String action;      // "Boost 3kHz +3dB", "Reduce lows -2dB"
    };
    
    MixAssistant();
    ~MixAssistant();
    
    // Analyze mix and provide suggestions
    std::vector<Suggestion> analyze(const std::vector<juce::AudioBuffer<float>>& tracks);
    
    // Specific analysis
    Suggestion analyzeFrequencyBalance(const juce::AudioBuffer<float>& masterBus);
    Suggestion analyzeDynamicRange(const juce::AudioBuffer<float>& track);
    Suggestion analyzeStereoWidth(const juce::AudioBuffer<float>& track);
    
    // Learn mode (user feedback)
    void acceptSuggestion(int suggestionId);
    void rejectSuggestion(int suggestionId);
    
private:
    std::vector<float> analyzeSpectrum(const juce::AudioBuffer<float>& buffer);
    float calculateCrestFactor(const juce::AudioBuffer<float>& buffer);
};

/**
 * @brief AI Tempo & Key Detection
 */
class TempoKeyDetector {
public:
    struct DetectionResult {
        double bpm;
        float confidence;         // 0.0-1.0
        juce::String key;         // "C", "Am", etc.
        float keyConfidence;
        int timeSignatureNum;
        int timeSignatureDenom;
    };
    
    TempoKeyDetector();
    ~TempoKeyDetector();
    
    // Detect from audio
    DetectionResult analyze(const juce::AudioBuffer<float>& audio, double sampleRate);
    
    // Real-time detection
    void processBlock(const float* data, int numSamples);
    bool hasResult() const;
    DetectionResult getResult();
    
private:
    // Tempo detection
    std::vector<double> detectOnsets(const juce::AudioBuffer<float>& audio, double sampleRate);
    double estimateTempo(const std::vector<double>& onsets);
    
    // Key detection
    std::vector<float> chromagram(const juce::AudioBuffer<float>& audio, double sampleRate);
    juce::String detectKey(const std::vector<float>& chroma);
    
    DetectionResult currentResult_;
    bool hasResult_;
};

/**
 * @brief AI Mastering Chain - Genre-specific mastering
 */
class MasteringChain {
public:
    struct ChainSettings {
        // EQ
        float lowShelfGain;
        float lowShelfFreq;
        float highShelfGain;
        float highShelfFreq;
        
        // Multiband Compression
        float lowBandThreshold;
        float midBandThreshold;
        float highBandThreshold;
        
        // Limiter
        float limiterThreshold;
        float limiterRelease;
        
        // Target
        float targetLUFS;
        
        ChainSettings() : lowShelfGain(0), lowShelfFreq(80), highShelfGain(0), highShelfFreq(8000),
                         lowBandThreshold(-20), midBandThreshold(-15), highBandThreshold(-10),
                         limiterThreshold(-0.3f), limiterRelease(100), targetLUFS(-14.0f) {}
    };
    
    MasteringChain();
    ~MasteringChain();
    
    // Generate mastering chain for genre
    ChainSettings generateForGenre(const juce::String& genre, const juce::AudioBuffer<float>& reference);
    
    // Analyze reference track
    ChainSettings matchReference(const juce::AudioBuffer<float>& input, 
                                 const juce::AudioBuffer<float>& reference);
    
    // Presets
    ChainSettings getPreset(const juce::String& genre);
    
private:
    std::map<juce::String, ChainSettings> presets_;
    void initializePresets();
    
    float analyzeLUFS(const juce::AudioBuffer<float>& audio);
    std::vector<float> analyzeFrequencyResponse(const juce::AudioBuffer<float>& audio);
};

/**
 * @brief AI Sample Matcher - Find similar samples by timbre
 */
class SampleMatcher {
public:
    struct SampleFeatures {
        float brightness;         // 0.0-1.0
        float warmth;
        float punchiness;
        float decay;
        std::vector<float> mfcc;  // Mel-frequency cepstral coefficients
    };
    
    SampleMatcher();
    ~SampleMatcher();
    
    // Extract features
    SampleFeatures extractFeatures(const juce::AudioBuffer<float>& sample);
    
    // Find similar samples
    std::vector<int> findSimilar(const SampleFeatures& query, 
                                 const std::vector<SampleFeatures>& database,
                                 int numResults = 10);
    
    // Semantic search
    std::vector<int> searchByDescription(const juce::String& description,
                                        const std::vector<SampleFeatures>& database);
    
private:
    float calculateSimilarity(const SampleFeatures& a, const SampleFeatures& b);
    std::vector<float> calculateMFCC(const juce::AudioBuffer<float>& audio);
};

} // namespace AI
} // namespace omega
