#include "PerformanceManager.h"
#include <algorithm>

namespace OmegaStudio {

//==============================================================================
// TrackFreezer Implementation
//==============================================================================
void TrackFreezer::freezeTrack(int trackId, const juce::AudioBuffer<float>& audio, double sampleRate) {
    FrozenTrack frozen;
    frozen.trackId = trackId;
    frozen.trackName = "Track " + juce::String(trackId);
    frozen.sampleRate = sampleRate;
    frozen.isFrozen = true;
    
    // Copy audio
    frozen.frozenAudio.makeCopyOf(audio);
    
    frozenTracks[trackId] = frozen;
}

void TrackFreezer::unfreezeTrack(int trackId) {
    auto it = frozenTracks.find(trackId);
    if (it != frozenTracks.end()) {
        it->second.isFrozen = false;
        it->second.frozenAudio.clear();
    }
}

bool TrackFreezer::isTrackFrozen(int trackId) const {
    auto it = frozenTracks.find(trackId);
    return (it != frozenTracks.end()) && it->second.isFrozen;
}

const juce::AudioBuffer<float>* TrackFreezer::getFrozenAudio(int trackId) const {
    auto it = frozenTracks.find(trackId);
    if (it != frozenTracks.end() && it->second.isFrozen) {
        return &it->second.frozenAudio;
    }
    return nullptr;
}

//==============================================================================
// SmartPluginManager Implementation
//==============================================================================
void SmartPluginManager::analyzePlugin(const juce::String& pluginName, const juce::AudioBuffer<float>& output) {
    auto& stats = pluginStats[pluginName];
    stats.pluginName = pluginName;
    
    // Check if output is silent
    if (isBufferSilent(output)) {
        stats.silentFrames++;
    } else {
        stats.silentFrames = 0;
        stats.isActive = true;
    }
    
    // Auto-bypass if silent for too long
    if (stats.silentFrames >= PluginStats::SILENCE_THRESHOLD) {
        stats.autoBypassed = true;
        stats.isActive = false;
    }
}

bool SmartPluginManager::shouldBypass(const juce::String& pluginName) const {
    auto it = pluginStats.find(pluginName);
    if (it != pluginStats.end()) {
        return it->second.autoBypassed;
    }
    return false;
}

void SmartPluginManager::resetStats(const juce::String& pluginName) {
    auto it = pluginStats.find(pluginName);
    if (it != pluginStats.end()) {
        it->second.silentFrames = 0;
        it->second.autoBypassed = false;
        it->second.isActive = true;
    }
}

std::vector<SmartPluginManager::PluginStats> SmartPluginManager::getAllStats() const {
    std::vector<PluginStats> stats;
    for (auto& pair : pluginStats) {
        stats.push_back(pair.second);
    }
    return stats;
}

bool SmartPluginManager::isBufferSilent(const juce::AudioBuffer<float>& buffer, float threshold) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::abs(data[i]) > threshold) {
                return false;
            }
        }
    }
    return true;
}

//==============================================================================
// CPUOptimizer Implementation
//==============================================================================
void CPUOptimizer::startFrame() {
    frameStartTime = juce::Time::getCurrentTime();
}

void CPUOptimizer::endFrame() {
    auto elapsed = (juce::Time::getCurrentTime() - frameStartTime).inMilliseconds();
    // Simple moving average
    metrics.overallCPU = metrics.overallCPU * 0.9 + elapsed * 0.1;
}

void CPUOptimizer::recordPluginTime(const juce::String& pluginName, double milliseconds) {
    metrics.pluginCPU[pluginName] = milliseconds;
}

void CPUOptimizer::recordTrackTime(int trackId, double milliseconds) {
    metrics.trackCPU[trackId] = milliseconds;
}

std::vector<juce::String> CPUOptimizer::getOptimizationSuggestions() {
    std::vector<juce::String> suggestions;
    
    // Find CPU-heavy plugins
    std::vector<std::pair<juce::String, double>> pluginsByTime;
    for (auto& pair : metrics.pluginCPU) {
        pluginsByTime.push_back(pair);
    }
    
    std::sort(pluginsByTime.begin(), pluginsByTime.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (metrics.overallCPU > 70.0) {
        suggestions.push_back("Overall CPU usage is high (>70%)");
        
        if (!pluginsByTime.empty() && pluginsByTime[0].second > 10.0) {
            suggestions.push_back("Consider freezing tracks with '" + 
                pluginsByTime[0].first + "' (highest CPU usage)");
        }
        
        suggestions.push_back("Enable Smart Auto-Bypass to disable inactive plugins");
        suggestions.push_back("Increase buffer size in audio settings");
    }
    
    // Check for inefficient tracks
    std::vector<std::pair<int, double>> tracksByTime;
    for (auto& pair : metrics.trackCPU) {
        tracksByTime.push_back(pair);
    }
    
    std::sort(tracksByTime.begin(), tracksByTime.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (!tracksByTime.empty() && tracksByTime[0].second > 15.0) {
        suggestions.push_back("Track " + juce::String(tracksByTime[0].first) + 
            " is using significant CPU - consider freezing");
    }
    
    if (metrics.dropouts > 0) {
        suggestions.push_back(juce::String(metrics.dropouts) + 
            " audio dropouts detected - increase buffer size");
    }
    
    return suggestions;
}

//==============================================================================
// StemExporter Implementation
//==============================================================================
void StemExporter::exportStems(const ExportSettings& settings,
                              std::function<void(const ExportProgress&)> progressCallback) {
    if (exporting.load()) {
        return; // Already exporting
    }
    
    exporting.store(true);
    cancelRequested.store(false);
    
    ExportProgress progress;
    progress.totalTracks = settings.trackIds.empty() ? 0 : (int)settings.trackIds.size();
    
    // Create output directory if needed
    if (!settings.outputDirectory.exists()) {
        settings.outputDirectory.createDirectory();
    }
    
    // Export each track
    for (size_t i = 0; i < settings.trackIds.size(); ++i) {
        if (cancelRequested.load()) {
            progress.hasError = true;
            progress.errorMessage = "Export cancelled by user";
            progressCallback(progress);
            break;
        }
        
        int trackId = settings.trackIds[i];
        progress.currentTrack = "Track " + juce::String(trackId);
        progress.completedTracks = (int)i;
        progress.percentage = (float)i / progress.totalTracks * 100.0f;
        
        progressCallback(progress);
        
        // Create output file
        juce::String filename = "Track_" + juce::String(trackId) + "." + settings.fileFormat;
        juce::File outputFile = settings.outputDirectory.getChildFile(filename);
        
        try {
            exportSingleStem(trackId, outputFile, settings);
        } catch (const std::exception& e) {
            progress.hasError = true;
            progress.errorMessage = "Error exporting track " + juce::String(trackId) + ": " + e.what();
            progressCallback(progress);
            break;
        }
    }
    
    // Export master if requested
    if (settings.includeMaster && !cancelRequested.load()) {
        progress.currentTrack = "Master";
        progress.completedTracks = progress.totalTracks;
        progress.percentage = 100.0f;
        progressCallback(progress);
        
        juce::String filename = "Master." + settings.fileFormat;
        juce::File outputFile = settings.outputDirectory.getChildFile(filename);
        
        try {
            // Export master mix (implementation would render full project)
            // exportSingleStem(-1, outputFile, settings);
        } catch (const std::exception& e) {
            progress.hasError = true;
            progress.errorMessage = "Error exporting master: " + juce::String(e.what());
        }
    }
    
    progress.isComplete = true;
    progress.percentage = 100.0f;
    progressCallback(progress);
    
    exporting.store(false);
}

void StemExporter::cancel() {
    cancelRequested.store(true);
}

void StemExporter::exportSingleStem(int trackId, const juce::File& outputFile,
                                   const ExportSettings& settings) {
    // This would render the track audio and save to file
    // Implementation would:
    // 1. Get track audio buffer
    // 2. Apply normalization if requested
    // 3. Convert to target sample rate/bit depth
    // 4. Write to file using appropriate format writer
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    // Get appropriate writer
    std::unique_ptr<juce::AudioFormat> format;
    if (settings.fileFormat == "wav") {
        format.reset(formatManager.findFormatForFileExtension("wav"));
    } else if (settings.fileFormat == "aiff") {
        format.reset(formatManager.findFormatForFileExtension("aiff"));
    } else if (settings.fileFormat == "flac") {
        format.reset(formatManager.findFormatForFileExtension("flac"));
    }
    
    if (format == nullptr) {
        throw std::runtime_error("Unsupported audio format");
    }
    
    // Create audio buffer for this track (placeholder)
    juce::AudioBuffer<float> trackBuffer(2, (int)(settings.sampleRate * 10.0)); // 10 seconds placeholder
    trackBuffer.clear();
    
    // Normalize if requested
    if (settings.normalizeStems) {
        float maxLevel = trackBuffer.getMagnitude(0, trackBuffer.getNumSamples());
        if (maxLevel > 0.0f) {
            float targetLevel = juce::Decibels::decibelsToGain(settings.normalizeLevel);
            float gain = targetLevel / maxLevel;
            trackBuffer.applyGain(gain);
        }
    }
    
    // Write to file
    if (auto* writer = format->createWriterFor(new juce::FileOutputStream(outputFile),
                                               settings.sampleRate,
                                               trackBuffer.getNumChannels(),
                                               settings.bitDepth,
                                               {},
                                               0)) {
        writer->writeFromAudioSampleBuffer(trackBuffer, 0, trackBuffer.getNumSamples());
        delete writer;
    }
}

//==============================================================================
// PerformanceManager Implementation
//==============================================================================
PerformanceManager::PerformanceManager() {}

void PerformanceManager::update() {
    // Update CPU monitoring
    cpuOptimizer.startFrame();
    
    // Auto-bypass logic
    if (autoBypassEnabled) {
        auto allStats = pluginManager.getAllStats();
        for (auto& stats : allStats) {
            if (stats.autoBypassed) {
                // Plugin has been auto-bypassed
            }
        }
    }
    
    // Auto-freeze logic
    if (autoFreezeEnabled) {
        auto metrics = cpuOptimizer.getMetrics();
        if (metrics.overallCPU > cpuThreshold) {
            // Find tracks to freeze automatically
            std::vector<std::pair<int, double>> tracksByTime;
            for (auto& pair : metrics.trackCPU) {
                tracksByTime.push_back(pair);
            }
            
            std::sort(tracksByTime.begin(), tracksByTime.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            // Freeze the most CPU-intensive track if not already frozen
            if (!tracksByTime.empty() && !trackFreezer.isTrackFrozen(tracksByTime[0].first)) {
                // Would trigger freeze here
                // trackFreezer.freezeTrack(tracksByTime[0].first, audio, sampleRate);
            }
        }
    }
    
    cpuOptimizer.endFrame();
}

juce::String PerformanceManager::getPerformanceReport() {
    juce::String report;
    auto metrics = cpuOptimizer.getMetrics();
    
    report << "=== PERFORMANCE REPORT ===\n\n";
    report << "Overall CPU: " << juce::String(metrics.overallCPU, 1) << "%\n";
    report << "Dropouts: " << juce::String(metrics.dropouts) << "\n\n";
    
    report << "Top 5 CPU-Heavy Plugins:\n";
    std::vector<std::pair<juce::String, double>> pluginsByTime;
    for (auto& pair : metrics.pluginCPU) {
        pluginsByTime.push_back(pair);
    }
    std::sort(pluginsByTime.begin(), pluginsByTime.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(pluginsByTime.size(), size_t(5)); ++i) {
        report << "  " << pluginsByTime[i].first << ": " 
               << juce::String(pluginsByTime[i].second, 2) << "ms\n";
    }
    
    report << "\nTop 5 CPU-Heavy Tracks:\n";
    std::vector<std::pair<int, double>> tracksByTime;
    for (auto& pair : metrics.trackCPU) {
        tracksByTime.push_back(pair);
    }
    std::sort(tracksByTime.begin(), tracksByTime.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(tracksByTime.size(), size_t(5)); ++i) {
        report << "  Track " << juce::String(tracksByTime[i].first) << ": " 
               << juce::String(tracksByTime[i].second, 2) << "ms\n";
    }
    
    report << "\nOptimization Suggestions:\n";
    auto suggestions = cpuOptimizer.getOptimizationSuggestions();
    for (auto& suggestion : suggestions) {
        report << "  - " << suggestion << "\n";
    }
    
    report << "\nAuto-Bypass Status: " << (autoBypassEnabled ? "Enabled" : "Disabled") << "\n";
    report << "Auto-Freeze Status: " << (autoFreezeEnabled ? "Enabled" : "Disabled") << "\n";
    
    auto pluginStats = pluginManager.getAllStats();
    int bypassedCount = 0;
    for (auto& stats : pluginStats) {
        if (stats.autoBypassed) bypassedCount++;
    }
    report << "Auto-Bypassed Plugins: " << juce::String(bypassedCount) << "\n";
    
    return report;
}

} // namespace OmegaStudio
