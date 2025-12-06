//==============================================================================
// ProjectManagement.h - Project Management Profesional
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>

namespace OmegaStudio {

//==============================================================================
/** Auto-Save System con Versioning */
class AutoSaveSystem {
public:
    //==========================================================================
    struct Version {
        juce::String filename;
        juce::Time timestamp;
        int64 fileSize;
        juce::String description;
        int versionNumber;
    };
    
    //==========================================================================
    AutoSaveSystem();
    ~AutoSaveSystem() = default;
    
    // Setup
    void setProjectPath(const juce::File& projectFile);
    void setAutoSaveInterval(int seconds);  // 0 = disabled
    void setMaxVersions(int maxVersions);
    
    // Auto-save control
    void startAutoSave();
    void stopAutoSave();
    void performAutoSave();
    
    bool isAutoSaveEnabled() const { return autoSaveEnabled_; }
    int getAutoSaveInterval() const { return autoSaveIntervalSeconds_; }
    
    // Manual save
    void saveVersion(const juce::String& description = "");
    
    // Version history
    std::vector<Version> getVersionHistory() const;
    bool loadVersion(int versionNumber);
    bool deleteVersion(int versionNumber);
    void cleanupOldVersions();
    
    // Recovery
    bool hasRecoveryFile() const;
    bool loadRecoveryFile();
    void clearRecoveryFile();
    
    // Callbacks
    std::function<juce::ValueTree()> onSave;        // Return project state
    std::function<void(const juce::ValueTree&)> onLoad;  // Load project state
    
    // Listeners
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void autoSavePerformed(const Version& version) {}
        virtual void autoSaveFailed(const juce::String& error) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    juce::File projectFile_;
    juce::File autoSaveDirectory_;
    
    bool autoSaveEnabled_ = true;
    int autoSaveIntervalSeconds_ = 300;  // 5 minutes
    int maxVersions_ = 20;
    int currentVersionNumber_ = 0;
    
    juce::Timer autoSaveTimer_;
    juce::ListenerList<Listener> listeners_;
    
    juce::File getVersionFile(int versionNumber) const;
    juce::File getRecoveryFile() const;
    
    void notifyAutoSavePerformed(const Version& version);
    void notifyAutoSaveFailed(const juce::String& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoSaveSystem)
};

//==============================================================================
/** Project Markers - Markers con timestamps y notas */
class ProjectMarkersSystem {
public:
    //==========================================================================
    enum class MarkerType {
        Generic,        // Marcador genérico
        Section,        // Sección (Intro, Verse, Chorus, etc.)
        Todo,           // To-do / nota
        Warning,        // Advertencia / problema
        Bookmark        // Bookmark
    };
    
    struct Marker {
        int id;
        double timeInBeats;
        juce::String name;
        juce::String notes;
        MarkerType type;
        juce::Colour color;
        juce::Time timestamp;
        
        bool operator<(const Marker& other) const {
            return timeInBeats < other.timeInBeats;
        }
    };
    
    //==========================================================================
    ProjectMarkersSystem();
    ~ProjectMarkersSystem() = default;
    
    // Marker management
    int addMarker(double timeInBeats, const juce::String& name, 
                  MarkerType type = MarkerType::Generic);
    void removeMarker(int markerId);
    void clearMarkers();
    
    void updateMarker(int markerId, const juce::String& name, const juce::String& notes);
    void moveMarker(int markerId, double newTimeInBeats);
    
    // Queries
    int getNumMarkers() const { return static_cast<int>(markers_.size()); }
    const Marker* getMarker(int markerId) const;
    std::vector<const Marker*> getAllMarkers() const;
    
    const Marker* getMarkerAt(double timeInBeats, double tolerance = 0.1) const;
    std::vector<const Marker*> getMarkersInRange(double startBeats, double endBeats) const;
    
    // Navigation
    const Marker* getNextMarker(double currentTime) const;
    const Marker* getPreviousMarker(double currentTime) const;
    
    // Section markers (musical structure)
    void addSectionMarker(double timeInBeats, const juce::String& sectionName);
    std::vector<const Marker*> getSectionMarkers() const;
    
    // Export markers to text/CSV
    juce::String exportToText() const;
    bool importFromText(const juce::String& text);
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce:String& tree);
    
    // Listeners
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void markerAdded(const Marker& marker) {}
        virtual void markerRemoved(int markerId) {}
        virtual void markerUpdated(const Marker& marker) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    std::map<int, Marker> markers_;
    int nextMarkerId_ = 1;
    
    juce::ListenerList<Listener> listeners_;
    
    void notifyMarkerAdded(const Marker& marker);
    void notifyMarkerRemoved(int markerId);
    void notifyMarkerUpdated(const Marker& marker);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectMarkersSystem)
};

//==============================================================================
/** Project Statistics - Estadísticas del proyecto */
class ProjectStatistics {
public:
    //==========================================================================
    struct Stats {
        // Time
        double totalDurationBeats = 0.0;
        double totalDurationSeconds = 0.0;
        
        // Tracks & Clips
        int numTracks = 0;
        int numAudioClips = 0;
        int numMidiClips = 0;
        int numPatterns = 0;
        
        // Plugins
        int numPlugins = 0;
        int numVST3Plugins = 0;
        int numAUPlugins = 0;
        std::vector<juce::String> pluginList;
        
        // Resources
        int64 totalSampleSize = 0;      // bytes
        int numSamples = 0;
        int numPresets = 0;
        
        // CPU Usage (promedio durante sesión)
        float averageCpuUsage = 0.0f;   // 0.0 - 1.0
        float peakCpuUsage = 0.0f;
        
        // File info
        juce::File projectFile;
        int64 projectFileSize = 0;
        juce::Time lastSaved;
        juce::Time created;
        juce::Time lastModified;
        
        // Editing
        int totalEdits = 0;
        juce::Time totalEditTime;
    };
    
    //==========================================================================
    ProjectStatistics();
    ~ProjectStatistics() = default;
    
    // Update stats
    void updateStats();
    const Stats& getStats() const { return stats_; }
    
    // Callbacks para obtener datos
    std::function<int()> getNumTracks;
    std::function<int()> getNumAudioClips;
    std::function<int()> getNumMidiClips;
    std::function<std::vector<juce::String>()> getPluginList;
    std::function<double()> getTotalDuration;
    
    // CPU monitoring
    void updateCpuUsage(float usage);
    
    // Edit tracking
    void incrementEditCount();
    void startEditSession();
    void endEditSession();
    
    // Export stats
    juce::String generateReport() const;
    void exportToFile(const juce::File& file) const;
    
private:
    //==========================================================================
    Stats stats_;
    juce::Time editSessionStart_;
    bool inEditSession_ = false;
    
    // CPU usage history
    std::vector<float> cpuUsageHistory_;
    static constexpr int maxCpuHistorySize = 1000;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectStatistics)
};

//==============================================================================
/** Backup System - Backup automático */
class BackupSystem {
public:
    //==========================================================================
    struct BackupInfo {
        juce::File backupFile;
        juce::Time timestamp;
        int64 fileSize;
        juce::String description;
    };
    
    //==========================================================================
    BackupSystem();
    ~BackupSystem() = default;
    
    // Setup
    void setBackupDirectory(const juce::File& directory);
    void setBackupInterval(int minutes);  // 0 = manual only
    void setMaxBackups(int max);
    
    // Backup control
    void startAutoBackup();
    void stopAutoBackup();
    void performBackup(const juce::String& description = "");
    
    // Backup management
    std::vector<BackupInfo> getBackupList() const;
    bool restoreBackup(const BackupInfo& backup);
    bool deleteBackup(const BackupInfo& backup);
    void cleanupOldBackups();
    
    // Settings
    void setCompression(bool enabled) { compressionEnabled_ = enabled; }
    bool isCompressionEnabled() const { return compressionEnabled_; }
    
    void setIncludeAssets(bool include) { includeAssets_ = include; }
    bool areAssetsIncluded() const { return includeAssets_; }
    
    // Callbacks
    std::function<juce::ValueTree()> onBackup;
    std::function<void(const juce::ValueTree&)> onRestore;
    
    // Listeners
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void backupCreated(const BackupInfo& backup) {}
        virtual void backupRestored(const BackupInfo& backup) {}
        virtual void backupFailed(const juce::String& error) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    juce::File backupDirectory_;
    juce::File currentProjectFile_;
    
    int backupIntervalMinutes_ = 30;
    int maxBackups_ = 10;
    bool compressionEnabled_ = true;
    bool includeAssets_ = true;
    
    juce::Timer backupTimer_;
    juce::ListenerList<Listener> listeners_;
    
    juce::File createBackupFile();
    bool compressBackup(const juce::File& source, const juce::File& destination);
    
    void notifyBackupCreated(const BackupInfo& backup);
    void notifyBackupRestored(const BackupInfo& backup);
    void notifyBackupFailed(const juce::String& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackupSystem)
};

//==============================================================================
/** Collaboration Features - Stems export, notas compartidas */
class CollaborationSystem {
public:
    //==========================================================================
    struct StemExportSettings {
        bool separateTracks = true;
        bool includeEffects = false;
        bool normalize = false;
        float normalizeLevel = -1.0f;  // dBFS
        
        juce::String format = "wav";    // wav, flac, mp3
        int bitDepth = 24;              // 16, 24, 32
        int sampleRate = 48000;
        
        bool includeMarkers = true;
        bool includeTempo = true;
    };
    
    struct SharedNote {
        juce::String author;
        juce::Time timestamp;
        double timeInBeats;
        juce::String content;
        int trackIndex = -1;  // -1 = global note
        
        std::vector<juce::String> tags;
        bool resolved = false;
    };
    
    //==========================================================================
    CollaborationSystem();
    ~CollaborationSystem() = default;
    
    // Stem export
    bool exportStems(const juce::File& outputDirectory, const StemExportSettings& settings);
    
    // Get export progress
    float getExportProgress() const { return exportProgress_; }
    void cancelExport();
    
    // Shared notes
    void addNote(const SharedNote& note);
    void removeNote(int noteId);
    void updateNote(int noteId, const juce::String& content);
    void resolveNote(int noteId);
    
    std::vector<SharedNote> getAllNotes() const;
    std::vector<SharedNote> getNotesForTrack(int trackIndex) const;
    std::vector<SharedNote> getUnresolvedNotes() const;
    
    // Collaboration metadata
    void setCollaborators(const juce::StringArray& collaborators);
    juce::StringArray getCollaborators() const;
    
    void addComment(double timeInBeats, const juce::String& comment, const juce::String& author);
    
    // Export project package (project + assets)
    bool exportProjectPackage(const juce::File& outputFile);
    bool importProjectPackage(const juce::File& packageFile);
    
private:
    //==========================================================================
    std::vector<SharedNote> notes_;
    int nextNoteId_ = 1;
    
    juce::StringArray collaborators_;
    float exportProgress_ = 0.0f;
    bool exportCancelled_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollaborationSystem)
};

} // namespace OmegaStudio
