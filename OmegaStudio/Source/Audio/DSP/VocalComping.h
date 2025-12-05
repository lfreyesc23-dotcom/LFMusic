/**
 * @file VocalComping.h
 * @brief Professional vocal comping tool for multi-take compilation
 * 
 * Features:
 * - Multi-take recording and management
 * - Intelligent selection of best parts
 * - Automatic crossfade creation
 * - Comp compilation and rendering
 * - Waveform visualization
 */

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "../../Utils/Constants.h"

namespace omega {

/**
 * @struct VocalTake
 * @brief Single vocal recording take
 */
struct VocalTake {
    juce::String name;
    juce::AudioBuffer<float> audioData;
    double startTime { 0.0 };
    double sampleRate { 48000.0 };
    juce::Colour color;
    bool isMuted { false };
    float volume { 1.0f };
    
    // Quality analysis
    float averageRMS { 0.0f };
    float pitchStability { 0.0f };
    float timingAccuracy { 0.0f };
    float overallScore { 0.0f };
    
    VocalTake() : color(juce::Colours::blue) {}
    
    int getLengthInSamples() const { return audioData.getNumSamples(); }
    double getDurationSeconds() const { return audioData.getNumSamples() / sampleRate; }
};

/**
 * @struct CompSegment
 * @brief Selected segment from a specific take
 */
struct CompSegment {
    int takeIndex { 0 };
    double startTime { 0.0 };
    double endTime { 0.0 };
    float crossfadeIn { 0.01f };    // Crossfade duration in seconds
    float crossfadeOut { 0.01f };
    bool selected { false };
    
    enum class CrossfadeType {
        Linear,
        EqualPower,
        Logarithmic,
        SCurve
    };
    
    CrossfadeType fadeType { CrossfadeType::EqualPower };
    
    bool isValid() const { return startTime < endTime && takeIndex >= 0; }
    double getDuration() const { return endTime - startTime; }
};

/**
 * @class VocalCompingEngine
 * @brief Core engine for vocal comping operations
 */
class VocalCompingEngine {
public:
    VocalCompingEngine();
    ~VocalCompingEngine() = default;
    
    /**
     * Initialize comping engine
     * @param sampleRate Audio sample rate
     */
    void initialize(double sampleRate);
    
    /**
     * Add a new take to the comp
     * @param takeData Audio data
     * @param takeName Name for the take
     * @return Take index
     */
    int addTake(const juce::AudioBuffer<float>& takeData, const juce::String& takeName);
    
    /**
     * Remove take by index
     * @param takeIndex Index of take to remove
     */
    void removeTake(int takeIndex);
    
    /**
     * Get take by index
     * @param takeIndex Take index
     * @return Pointer to take or nullptr
     */
    VocalTake* getTake(int takeIndex);
    const VocalTake* getTake(int takeIndex) const;
    
    /**
     * Get total number of takes
     */
    int getNumTakes() const { return static_cast<int>(m_takes.size()); }
    
    /**
     * Add comp segment
     * @param segment Segment to add
     */
    void addSegment(const CompSegment& segment);
    
    /**
     * Remove segment by index
     * @param segmentIndex Segment index
     */
    void removeSegment(int segmentIndex);
    
    /**
     * Get segment by index
     */
    CompSegment* getSegment(int segmentIndex);
    const CompSegment* getSegment(int segmentIndex) const;
    
    /**
     * Get number of segments
     */
    int getNumSegments() const { return static_cast<int>(m_segments.size()); }
    
    /**
     * Clear all segments
     */
    void clearSegments();
    
    /**
     * Compile all segments into final audio
     * @param outputBuffer Output buffer to write to
     * @param startSample Starting sample in output
     * @param numSamples Number of samples to render
     */
    void compileToBuffer(juce::AudioBuffer<float>& outputBuffer, 
                        int startSample, 
                        int numSamples);
    
    /**
     * Export compiled comp to audio buffer
     * @return Compiled audio buffer
     */
    juce::AudioBuffer<float> exportCompiledComp();
    
    /**
     * Analyze take quality (RMS, pitch stability, timing)
     * @param takeIndex Take to analyze
     */
    void analyzeTakeQuality(int takeIndex);
    
    /**
     * Auto-select best segments from all takes
     * @param segmentDuration Duration of each segment in seconds
     */
    void autoSelectBestSegments(double segmentDuration);
    
    /**
     * Reset engine
     */
    void reset();
    
    /**
     * Get/Set crossfade parameters
     */
    void setDefaultCrossfadeDuration(float seconds) { m_defaultCrossfade = seconds; }
    float getDefaultCrossfadeDuration() const { return m_defaultCrossfade; }
    
    void setDefaultCrossfadeType(CompSegment::CrossfadeType type) { m_defaultFadeType = type; }
    CompSegment::CrossfadeType getDefaultCrossfadeType() const { return m_defaultFadeType; }
    
private:
    void applyCrossfade(float* buffer, int numSamples, 
                       float fadeLevel, 
                       CompSegment::CrossfadeType type, 
                       bool fadeIn);
    
    float calculateCrossfadeGain(float position, 
                                CompSegment::CrossfadeType type, 
                                bool fadeIn);
    
    // Quality analysis helpers
    float calculateRMS(const float* buffer, int numSamples);
    float analyzePitchStability(const float* buffer, int numSamples);
    float analyzeTimingAccuracy(const float* buffer, int numSamples);
    
    std::vector<std::unique_ptr<VocalTake>> m_takes;
    std::vector<CompSegment> m_segments;
    
    double m_sampleRate { 48000.0 };
    float m_defaultCrossfade { 0.01f };  // 10ms default
    CompSegment::CrossfadeType m_defaultFadeType { CompSegment::CrossfadeType::EqualPower };
    
    // Temp buffers
    juce::AudioBuffer<float> m_tempBuffer;
};

/**
 * @class VocalCompingRecorder
 * @brief Handles recording of multiple takes
 */
class VocalCompingRecorder {
public:
    VocalCompingRecorder();
    ~VocalCompingRecorder() = default;
    
    /**
     * Initialize recorder
     * @param sampleRate Sample rate
     * @param maxLengthSeconds Maximum recording length
     */
    void initialize(double sampleRate, double maxLengthSeconds = 600.0);
    
    /**
     * Start recording a new take
     * @param takeName Name for the take
     * @return True if started successfully
     */
    bool startRecording(const juce::String& takeName);
    
    /**
     * Stop recording current take
     */
    void stopRecording();
    
    /**
     * Process audio input (call from audio callback)
     * @param inputBuffer Input audio
     * @param numSamples Number of samples
     */
    void processAudioInput(const float* inputBuffer, int numSamples);
    
    /**
     * Check if currently recording
     */
    bool isRecording() const { return m_isRecording; }
    
    /**
     * Get current take buffer
     */
    const juce::AudioBuffer<float>& getCurrentTakeBuffer() const { return m_recordBuffer; }
    
    /**
     * Get current take name
     */
    juce::String getCurrentTakeName() const { return m_currentTakeName; }
    
    /**
     * Reset recorder
     */
    void reset();
    
private:
    juce::AudioBuffer<float> m_recordBuffer;
    int m_writePosition { 0 };
    bool m_isRecording { false };
    double m_sampleRate { 48000.0 };
    int m_maxSamples { 0 };
    juce::String m_currentTakeName;
};

/**
 * @class VocalCompingManager
 * @brief High-level manager for complete vocal comping workflow
 */
class VocalCompingManager {
public:
    VocalCompingManager();
    ~VocalCompingManager() = default;
    
    /**
     * Initialize manager
     * @param sampleRate Sample rate
     */
    void initialize(double sampleRate);
    
    /**
     * Get comping engine
     */
    VocalCompingEngine& getEngine() { return m_engine; }
    const VocalCompingEngine& getEngine() const { return m_engine; }
    
    /**
     * Get recorder
     */
    VocalCompingRecorder& getRecorder() { return m_recorder; }
    const VocalCompingRecorder& getRecorder() const { return m_recorder; }
    
    /**
     * Start new take recording
     */
    bool startNewTake(const juce::String& takeName);
    
    /**
     * Stop recording and add to engine
     */
    void stopAndCommitTake();
    
    /**
     * Process audio for recording (RT-safe)
     */
    void processRecordingInput(const float* inputBuffer, int numSamples);
    
    /**
     * Save comp session to file
     * @param file Output file
     * @return True if successful
     */
    bool saveCompSession(const juce::File& file);
    
    /**
     * Load comp session from file
     * @param file Input file
     * @return True if successful
     */
    bool loadCompSession(const juce::File& file);
    
    /**
     * Export compiled comp to audio file
     * @param outputFile Output file path
     * @return True if successful
     */
    bool exportCompiledAudio(const juce::File& outputFile);
    
private:
    VocalCompingEngine m_engine;
    VocalCompingRecorder m_recorder;
    double m_sampleRate { 48000.0 };
};

} // namespace omega
