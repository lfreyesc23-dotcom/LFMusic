/*
  ==============================================================================
    ProjectManager.h
    
    Sistema de gestión de proyectos profesional con:
    - Save/Load de proyectos .omegastudio
    - Undo/Redo system ilimitado
    - Project templates
    - Autosave con versioning
    - Metadata completa (BPM, key, author, etc.)
    
    Enterprise-grade project management
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <deque>
#include <functional>
#include <optional>

namespace OmegaStudio {

//==============================================================================
/** Estados del proyecto */
enum class ProjectState {
    Empty,          // Proyecto vacío/nuevo
    Loaded,         // Cargado desde archivo
    Modified,       // Modificado sin guardar
    Saving,         // Guardando...
    Error           // Error al cargar/guardar
};

//==============================================================================
/** Metadata del proyecto */
struct ProjectMetadata {
    juce::String projectName { "Untitled Project" };
    juce::String author { "Unknown Artist" };
    juce::String genre { "Electronic" };
    
    double bpm { 120.0 };
    juce::String key { "C Major" };
    int timeSignatureNumerator { 4 };
    int timeSignatureDenominator { 4 };
    
    juce::String notes;
    juce::StringArray tags;
    
    juce::Time createdDate;
    juce::Time lastModifiedDate;
    juce::String version { "1.0.0" };
    
    // Serialization
    juce::var toVar() const;
    static ProjectMetadata fromVar(const juce::var& v);
};

//==============================================================================
/** Acción para undo/redo */
class UndoableAction {
public:
    virtual ~UndoableAction() = default;
    
    virtual void perform() = 0;
    virtual void undo() = 0;
    virtual juce::String getDescription() const = 0;
    
    // Size estimation para gestión de memoria
    virtual size_t getSizeInBytes() const { return 1024; }
};

//==============================================================================
/** Sistema de Undo/Redo con límite de memoria */
class UndoRedoManager {
public:
    UndoRedoManager(size_t maxMemoryBytes = 100 * 1024 * 1024); // 100 MB default
    
    void performAction(std::unique_ptr<UndoableAction> action);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearHistory();
    
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    int getNumActionsInHistory() const;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void undoRedoStateChanged() = 0;
    };
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    std::deque<std::unique_ptr<UndoableAction>> undoStack;
    std::deque<std::unique_ptr<UndoableAction>> redoStack;
    
    size_t maxMemoryBytes;
    size_t currentMemoryUsage { 0 };
    
    juce::ListenerList<Listener> listeners;
    
    void trimStacksIfNeeded();
    void notifyListeners();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoRedoManager)
};

//==============================================================================
/** Track data para serialización */
struct TrackData {
    juce::String name;
    juce::Colour colour;
    bool armed { false };
    bool muted { false };
    bool soloed { false };
    float volume { 0.8f };
    float pan { 0.0f };
    
    // Plugin chain (será implementado)
    juce::StringArray pluginChain;
    
    // Automation data
    juce::var automationData;
    
    juce::var toVar() const;
    static TrackData fromVar(const juce::var& v);
};

//==============================================================================
/** Datos del proyecto completo */
struct ProjectData {
    ProjectMetadata metadata;
    std::vector<TrackData> tracks;
    
    // Audio files referenced
    juce::StringArray audioFiles;
    
    // MIDI data (será implementado)
    juce::var midiData;
    
    // Mixer state
    juce::var mixerState;
    
    // Plugin states
    juce::var pluginStates;
    
    juce::var toVar() const;
    static ProjectData fromVar(const juce::var& v);
};

//==============================================================================
/** Template del proyecto */
struct ProjectTemplate {
    juce::String name;
    juce::String description;
    juce::String category;
    ProjectData templateData;
    
    static std::vector<ProjectTemplate> getBuiltInTemplates();
};

//==============================================================================
/** Manager principal de proyectos */
class ProjectManager {
public:
    ProjectManager();
    ~ProjectManager();
    
    // Listener callback for project changes
    std::function<void()> onProjectChanged;
    
    // Project lifecycle
    void newProject(const juce::String& name = "Untitled");
    void newProjectFromTemplate(const ProjectTemplate& templ);
    bool loadProject(const juce::File& file);
    bool saveProject(const juce::File& file);
    bool saveProjectAs(const juce::File& file);
    void closeProject();
    
    // Autosave
    void setAutosaveEnabled(bool enabled);
    void setAutosaveInterval(int seconds);
    
    // State
    ProjectState getState() const { return state; }
    bool hasUnsavedChanges() const { return state == ProjectState::Modified; }
    juce::File getCurrentProjectFile() const { return currentFile; }
    
    // Data access
    ProjectMetadata& getMetadata() { return projectData.metadata; }
    const ProjectMetadata& getMetadata() const { return projectData.metadata; }
    ProjectData& getProjectData() { return projectData; }
    const ProjectData& getProjectData() const { return projectData; }
    
    // Undo/Redo
    UndoRedoManager& getUndoRedoManager() { return undoRedoManager; }
    
    // Tracks management
    void addTrack(const TrackData& track);
    void removeTrack(int index);
    void moveTrack(int fromIndex, int toIndex);
    TrackData& getTrack(int index);
    int getNumTracks() const { return static_cast<int>(projectData.tracks.size()); }
    
    // Mark as modified
    void markAsModified();
    
    // Export
    bool exportStemsTracks(const juce::File& directory);
    bool exportMixdown(const juce::File& file, int bitDepth = 24);
    
private:
    ProjectData projectData;
    ProjectState state { ProjectState::Empty };
    juce::File currentFile;
    
    UndoRedoManager undoRedoManager;
    
    // Autosave
    bool autosaveEnabled { true };
    int autosaveIntervalSeconds { 300 }; // 5 minutes
    
    void performAutosave();
    
    // Serialization helpers
    bool serializeToFile(const juce::File& file);
    bool deserializeFromFile(const juce::File& file);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};

//==============================================================================
/** Undo actions concretas */

class AddTrackAction : public UndoableAction {
public:
    AddTrackAction(ProjectManager& pm, TrackData track)
        : projectManager(pm), trackData(std::move(track)) {}
    
    void perform() override;
    void undo() override;
    juce::String getDescription() const override { return "Add Track"; }
    
private:
    ProjectManager& projectManager;
    TrackData trackData;
    int addedIndex { -1 };
};

class RemoveTrackAction : public UndoableAction {
public:
    RemoveTrackAction(ProjectManager& pm, int index)
        : projectManager(pm), trackIndex(index) {}
    
    void perform() override;
    void undo() override;
    juce::String getDescription() const override { return "Remove Track"; }
    
private:
    ProjectManager& projectManager;
    int trackIndex;
    TrackData removedTrack;
};

class ChangeMetadataAction : public UndoableAction {
public:
    ChangeMetadataAction(ProjectManager& pm, ProjectMetadata newMeta)
        : projectManager(pm), newMetadata(std::move(newMeta))
        , oldMetadata(pm.getMetadata()) {}
    
    void perform() override;
    void undo() override;
    juce::String getDescription() const override { return "Change Metadata"; }
    
private:
    ProjectManager& projectManager;
    ProjectMetadata oldMetadata;
    ProjectMetadata newMetadata;
};

} // namespace OmegaStudio
