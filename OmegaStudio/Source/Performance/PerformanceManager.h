#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {

/**
 * @brief Track Freezing System
 * 
 * Allows rendering tracks to audio to save CPU
 */
class TrackFreezer {
public:
    struct FrozenTrack {
        int trackId;
        juce::String trackName;
        juce::AudioBuffer<float> frozenAudio;
        double sampleRate;
        bool isFrozen = false;
        
        // Store original state for unfreeze
        juce::MemoryBlock pluginStates;
    };
    
    void freezeTrack(int trackId, const juce::AudioBuffer<float>& audio, double sampleRate);
    void unfreezeTrack(int trackId);
    bool isTrackFrozen(int trackId) const;
    const juce::AudioBuffer<float>* getFrozenAudio(int trackId) const;
    
private:
    std::map<int, FrozenTrack> frozenTracks;
};

//==============================================================================
/**
 * @brief Smart Plugin Auto-Bypass
 * 
 * Automatically bypasses inactive plugins to save CPU
 */
class SmartPluginManager {
public:
    struct PluginStats {
        juce::String pluginName;
        double cpuUsage = 0.0;
        bool isActive = true;
        bool autoBypassed = false;
        int silentFrames = 0;
        
        static constexpr int SILENCE_THRESHOLD = 4410; // 100ms at 44.1kHz
    };
    
    void analyzePlugin(const juce::String& pluginName, const juce::AudioBuffer<float>& output);
    bool shouldBypass(const juce::String& pluginName) const;
    void resetStats(const juce::String& pluginName);
    
    std::vector<PluginStats> getAllStats() const;
    
private:
    std::map<juce::String, PluginStats> pluginStats;
    
    bool isBufferSilent(const juce::AudioBuffer<float>& buffer, float threshold = 0.0001f);
};

//==============================================================================
/**
 * @brief CPU Optimization & Monitoring
 */
class CPUOptimizer {
public:
    struct PerformanceMetrics {
        double overallCPU = 0.0;
        std::map<juce::String, double> pluginCPU;
        std::map<int, double> trackCPU;
        double bufferFillPercentage = 0.0;
        int dropouts = 0;
    };
    
    void startFrame();
    void endFrame();
    void recordPluginTime(const juce::String& pluginName, double milliseconds);
    void recordTrackTime(int trackId, double milliseconds);
    
    PerformanceMetrics getMetrics() const { return metrics; }
    
    // Optimization suggestions
    std::vector<juce::String> getOptimizationSuggestions();
    
private:
    PerformanceMetrics metrics;
    juce::Time frameStartTime;
};

//==============================================================================
/**
 * @brief Stem Export System
 */
class StemExporter {
public:
    struct ExportSettings {
        juce::File outputDirectory;
        juce::String fileFormat = "wav";  // wav, aiff, flac
        int bitDepth = 24;                // 16, 24, 32
        double sampleRate = 44100.0;
        bool normalizeStems = false;
        float normalizeLevel = -0.1f;     // dBFS
        bool includeMaster = true;
        
        // Track selection
        std::vector<int> trackIds;        // Empty = all tracks
    };
    
    struct ExportProgress {
        int totalTracks = 0;
        int completedTracks = 0;
        float percentage = 0.0f;
        juce::String currentTrack;
        bool isComplete = false;
        bool hasError = false;
        juce::String errorMessage;
    };
    
    void exportStems(const ExportSettings& settings, 
                    std::function<void(const ExportProgress&)> progressCallback);
    
    void cancel();
    bool isExporting() const { return exporting.load(); }
    
private:
    std::atomic<bool> exporting{false};
    std::atomic<bool> cancelRequested{false};
    
    void exportSingleStem(int trackId, const juce::File& outputFile,
                         const ExportSettings& settings);
};

//==============================================================================
/**
 * @brief Performance Manager - Combines all optimization systems
 */
class PerformanceManager {
public:
    PerformanceManager();
    
    // Subsystems
    TrackFreezer& getTrackFreezer() { return trackFreezer; }
    SmartPluginManager& getPluginManager() { return pluginManager; }
    CPUOptimizer& getCPUOptimizer() { return cpuOptimizer; }
    StemExporter& getStemExporter() { return stemExporter; }
    
    // Global settings
    void setAutoBypassEnabled(bool enabled) { autoBypassEnabled = enabled; }
    void setAutoFreezeEnabled(bool enabled) { autoFreezeEnabled = enabled; }
    void setCPUThreshold(double threshold) { cpuThreshold = threshold; }
    
    // Monitoring
    void update();
    juce::String getPerformanceReport();
    
private:
    TrackFreezer trackFreezer;
    SmartPluginManager pluginManager;
    CPUOptimizer cpuOptimizer;
    StemExporter stemExporter;
    
    bool autoBypassEnabled = true;
    bool autoFreezeEnabled = false;
    double cpuThreshold = 80.0; // percent
};

} // namespace OmegaStudio
