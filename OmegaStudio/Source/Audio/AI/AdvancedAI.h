/*
  ==============================================================================
    AdvancedAI.h
    
    Advanced AI features para producción profesional:
    - StemSeparator: Separación AI de vocals/drums/bass/other
    - MasteringAssistant: Sugerencias automáticas de mastering
    - SmartEQ: Análisis espectral y corrección inteligente
    - MixAnalyzer: Análisis completo de mezclas
    
    Enterprise-grade AI audio processing
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace OmegaStudio {

//==============================================================================
/** StemSeparator - Separación AI de stems (vocals, drums, bass, other) */
class StemSeparator {
public:
    enum class StemType {
        Vocals,
        Drums,
        Bass,
        Other
    };
    
    StemSeparator();
    ~StemSeparator();
    
    // Processing
    void prepareToPlay(double sampleRate, int blockSize);
    void releaseResources();
    
    // Separate stems from mixed audio
    bool separateStems(const juce::AudioBuffer<float>& mixedAudio,
                      std::map<StemType, juce::AudioBuffer<float>>& separatedStems);
    
    // Process in chunks for real-time
    void startSeparation(const juce::File& inputFile);
    void stopSeparation();
    bool isSeparating() const { return separating; }
    float getProgress() const { return progress; }
    
    // Quality settings
    void setQuality(int quality);  // 0=draft, 1=good, 2=best
    void setModelPath(const juce::File& path);
    
    // Results
    bool exportStem(StemType type, const juce::File& outputFile);
    
private:
    double sampleRate { 44100.0 };
    int blockSize { 512 };
    bool separating { false };
    float progress { 0.0f };
    
    int quality { 1 };
    juce::File modelPath;
    
    std::map<StemType, juce::AudioBuffer<float>> stems;
    
    // AI model processing (stub)
    void processWithModel(const juce::AudioBuffer<float>& input);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparator)
};

//==============================================================================
/** MasteringAssistant - Asistente inteligente de mastering */
class MasteringAssistant {
public:
    struct MasteringSuggestion {
        juce::String description;
        juce::String category;  // "EQ", "Compression", "Limiting", "Stereo"
        float severity;  // 0-1, qué tan crítico es
        
        // Parámetros sugeridos
        std::map<juce::String, float> suggestedParameters;
        
        juce::var toVar() const;
    };
    
    MasteringAssistant();
    ~MasteringAssistant();
    
    // Analysis
    void analyzeAudio(const juce::AudioBuffer<float>& audio);
    std::vector<MasteringSuggestion> getSuggestions() const { return suggestions; }
    
    // Target loudness
    void setTargetLUFS(float lufs);  // -14 LUFS para streaming, -9 para CD
    void setTargetDynamicRange(float db);
    
    // Genre-specific analysis
    void setGenre(const juce::String& genre);  // "Electronic", "Rock", "Hip Hop", etc.
    
    // Apply suggestions automatically
    void applySuggestion(int index, juce::AudioProcessor* processor);
    void applyAllSuggestions(juce::AudioProcessor* processor);
    
    // Analysis metrics
    float getCurrentLUFS() const { return currentLUFS; }
    float getDynamicRange() const { return dynamicRange; }
    float getStereoWidth() const { return stereoWidth; }
    bool hasPhasingIssues() const { return phasingIssues; }
    
private:
    std::vector<MasteringSuggestion> suggestions;
    
    float targetLUFS { -14.0f };
    float targetDynamicRange { 8.0f };
    juce::String genre { "Electronic" };
    
    // Analysis results
    float currentLUFS { -23.0f };
    float dynamicRange { 12.0f };
    float stereoWidth { 1.0f };
    bool phasingIssues { false };
    
    void analyzeLoudness(const juce::AudioBuffer<float>& audio);
    void analyzeDynamics(const juce::AudioBuffer<float>& audio);
    void analyzeStereoField(const juce::AudioBuffer<float>& audio);
    void analyzeFrequencyBalance(const juce::AudioBuffer<float>& audio);
    void generateSuggestions();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringAssistant)
};

//==============================================================================
/** SmartEQ - EQ inteligente con sugerencias automáticas */
class SmartEQ {
public:
    enum class EQMode {
        Corrective,    // Corregir problemas
        Creative,      // Mejora creativa
        Matching       // Match a referencia
    };
    
    struct EQSuggestion {
        float frequency;
        float gain;      // dB
        float q;
        juce::String reason;
        
        juce::var toVar() const;
    };
    
    SmartEQ();
    ~SmartEQ();
    
    // Analysis
    void analyzeAudio(const juce::AudioBuffer<float>& audio);
    void setReferenceAudio(const juce::AudioBuffer<float>& reference);
    
    // Mode
    void setMode(EQMode mode) { currentMode = mode; }
    EQMode getMode() const { return currentMode; }
    
    // Suggestions
    std::vector<EQSuggestion> getSuggestions() const { return suggestions; }
    void applySuggestion(int index, juce::AudioProcessor* eq);
    
    // Auto-EQ
    void applyAutoEQ(juce::AudioProcessor* eq);
    
    // Target curve
    void setTargetCurve(const std::vector<float>& frequencies, const std::vector<float>& gains);
    
private:
    EQMode currentMode { EQMode::Corrective };
    std::vector<EQSuggestion> suggestions;
    
    std::vector<float> spectrumAnalysis;
    std::vector<float> referenceSpectrum;
    
    void detectResonances();
    void detectMuddy();
    void detectHarshness();
    void detectLackOfAir();
    void compareWithReference();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartEQ)
};

//==============================================================================
/** MixAnalyzer - Análisis completo de mezclas */
class MixAnalyzer {
public:
    struct MixAnalysis {
        // Overall metrics
        float lufs { -23.0f };
        float dynamicRange { 12.0f };
        float stereoWidth { 1.0f };
        float peakLevel { -6.0f };
        
        // Frequency balance
        float bassEnergy { 0.0f };      // 20-250 Hz
        float midEnergy { 0.0f };       // 250-4k Hz
        float highEnergy { 0.0f };      // 4k-20k Hz
        
        // Issues detected
        std::vector<juce::String> warnings;
        std::vector<juce::String> suggestions;
        
        // Comparison with professional mixes
        float professionalScore { 0.0f };  // 0-100
        
        juce::var toVar() const;
    };
    
    MixAnalyzer();
    ~MixAnalyzer();
    
    // Analysis
    MixAnalysis analyzeMix(const juce::AudioBuffer<float>& audio);
    MixAnalysis analyzeMix(const juce::File& audioFile);
    
    // Comparison
    void addReferenceTrack(const juce::File& file);
    void clearReferenceTracks();
    
    // Settings
    void setTargetGenre(const juce::String& genre);
    void setAnalysisDepth(int depth);  // 0=quick, 1=normal, 2=deep
    
    // Results
    MixAnalysis getLastAnalysis() const { return lastAnalysis; }
    
private:
    MixAnalysis lastAnalysis;
    juce::String targetGenre { "Electronic" };
    int analysisDepth { 1 };
    
    std::vector<juce::AudioBuffer<float>> referenceTracks;
    
    void analyzeFrequencyBalance(const juce::AudioBuffer<float>& audio, MixAnalysis& result);
    void analyzeDynamics(const juce::AudioBuffer<float>& audio, MixAnalysis& result);
    void analyzeStereoField(const juce::AudioBuffer<float>& audio, MixAnalysis& result);
    void detectIssues(MixAnalysis& result);
    void generateSuggestions(MixAnalysis& result);
    void compareWithReferences(MixAnalysis& result);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixAnalyzer)
};

} // namespace OmegaStudio
