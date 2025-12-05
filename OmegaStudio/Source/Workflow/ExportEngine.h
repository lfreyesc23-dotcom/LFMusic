#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace Workflow {

/**
 * @brief Export/Bounce Engine
 */
class ExportEngine {
public:
    struct ExportSettings {
        juce::File outputFile;
        int sampleRate{44100};
        int bitDepth{24};
        bool exportStems{false};
        bool exportMIDI{false};
        bool normalizeAudio{true};
        bool applyDithering{true};
        double startTime{0.0};
        double endTime{-1.0};  // -1 = project end
        
        // Stem export
        std::vector<int> stemTracks;  // Track IDs to export
        bool separateMasterFX{false};
    };
    
    void exportProject(const ExportSettings& settings) {
        if (settings.exportStems) {
            exportStems(settings);
        } else {
            exportMasterMix(settings);
        }
        
        if (settings.exportMIDI) {
            exportMIDI(settings);
        }
    }
    
    void exportMasterMix(const ExportSettings& settings) {
        // TODO: Render master output to file
        juce::WavAudioFormat wavFormat;
        
        auto fileStream = std::make_unique<juce::FileOutputStream>(settings.outputFile);
        if (!fileStream->openedOk()) {
            return;
        }
        
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(fileStream.get(), 
                                     settings.sampleRate,
                                     2,  // stereo
                                     settings.bitDepth,
                                     juce::StringPairArray(), 
                                     0)
        );
        
        if (writer) {
            fileStream.release();
            // Render audio...
        }
    }
    
    void exportStems(const ExportSettings& settings) {
        for (int trackId : settings.stemTracks) {
            juce::File stemFile = settings.outputFile.getSiblingFile(
                settings.outputFile.getFileNameWithoutExtension() + 
                "_Track_" + juce::String(trackId) + 
                settings.outputFile.getFileExtension()
            );
            
            ExportSettings stemSettings = settings;
            stemSettings.outputFile = stemFile;
            stemSettings.exportStems = false;
            
            // TODO: Render individual track
            exportMasterMix(stemSettings);
        }
    }
    
    void exportMIDI(const ExportSettings& settings) {
        juce::MidiFile midiFile;
        // TODO: Collect MIDI from all tracks
        
        juce::File midiOutput = settings.outputFile.withFileExtension(".mid");
        juce::FileOutputStream stream(midiOutput);
        
        if (stream.openedOk()) {
            midiFile.writeTo(stream);
        }
    }
    
    std::function<void(float)> onProgressUpdate;
    std::function<void(bool, const juce::String&)> onExportComplete;
};

/**
 * @brief Performance/CPU Manager
 */
class PerformanceManager {
public:
    struct TrackPerformance {
        int trackId{-1};
        juce::String trackName;
        float cpuUsage{0.0f};
        int pluginCount{0};
        bool isFrozen{false};
        juce::File frozenAudioFile;
    };
    
    // Track freezing
    void freezeTrack(int trackId) {
        // TODO: Render track to audio file and disable plugins
        TrackPerformance* track = getTrackPerformance(trackId);
        if (track) {
            track->isFrozen = true;
            // Render and save audio
            track->frozenAudioFile = juce::File::getSpecialLocation(
                juce::File::tempDirectory).getChildFile("frozen_" + juce::String(trackId) + ".wav");
        }
    }
    
    void unfreezeTrack(int trackId) {
        TrackPerformance* track = getTrackPerformance(trackId);
        if (track) {
            track->isFrozen = false;
            if (track->frozenAudioFile.existsAsFile()) {
                track->frozenAudioFile.deleteFile();
            }
        }
    }
    
    // Smart disable (auto-bypass inactive plugins)
    void enableSmartDisable(bool enable) {
        smartDisableEnabled_ = enable;
    }
    
    void updatePluginActivity() {
        if (!smartDisableEnabled_) return;
        
        // TODO: Monitor plugin I/O and disable if silent for > threshold
    }
    
    // CPU monitoring
    float getTotalCPUUsage() const {
        float total = 0.0f;
        for (const auto& track : tracks_) {
            total += track.cpuUsage;
        }
        return total;
    }
    
    void setTrackCPUUsage(int trackId, float usage) {
        TrackPerformance* track = getTrackPerformance(trackId);
        if (track) {
            track->cpuUsage = usage;
        }
    }
    
    const std::vector<TrackPerformance>& getTracks() const {
        return tracks_;
    }
    
    // Low-latency mode
    void setLowLatencyMode(bool enable) {
        if (enable) {
            // Reduce buffer size, disable heavy plugins
            currentBufferSize_ = 64;
        } else {
            currentBufferSize_ = 512;
        }
    }
    
private:
    std::vector<TrackPerformance> tracks_;
    bool smartDisableEnabled_{false};
    int currentBufferSize_{512};
    
    TrackPerformance* getTrackPerformance(int trackId) {
        for (auto& track : tracks_) {
            if (track.trackId == trackId) {
                return &track;
            }
        }
        return nullptr;
    }
};

} // namespace Workflow
} // namespace OmegaStudio
