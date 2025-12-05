/*
  ==============================================================================
    ProjectManager.cpp
    Implementation del sistema de gestión de proyectos
  ==============================================================================
*/

#include "ProjectManager.h"

namespace OmegaStudio {

//==============================================================================
// ProjectMetadata Implementation
//==============================================================================

juce::var ProjectMetadata::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("projectName", projectName);
    obj->setProperty("author", author);
    obj->setProperty("genre", genre);
    obj->setProperty("bpm", bpm);
    obj->setProperty("key", key);
    obj->setProperty("timeSignatureNumerator", timeSignatureNumerator);
    obj->setProperty("timeSignatureDenominator", timeSignatureDenominator);
    obj->setProperty("notes", notes);
    obj->setProperty("version", version);
    
    juce::var tagArray;
    for (const auto& tag : tags)
        tagArray.append(tag);
    obj->setProperty("tags", tagArray);
    
    obj->setProperty("createdDate", createdDate.toMilliseconds());
    obj->setProperty("lastModifiedDate", lastModifiedDate.toMilliseconds());
    
    return juce::var(obj.get());
}

ProjectMetadata ProjectMetadata::fromVar(const juce::var& v) {
    ProjectMetadata meta;
    
    if (auto* obj = v.getDynamicObject()) {
        meta.projectName = obj->getProperty("projectName").toString();
        meta.author = obj->getProperty("author").toString();
        meta.genre = obj->getProperty("genre").toString();
        meta.bpm = obj->getProperty("bpm");
        meta.key = obj->getProperty("key").toString();
        meta.timeSignatureNumerator = obj->getProperty("timeSignatureNumerator");
        meta.timeSignatureDenominator = obj->getProperty("timeSignatureDenominator");
        meta.notes = obj->getProperty("notes").toString();
        meta.version = obj->getProperty("version").toString();
        
        if (auto* tagArray = obj->getProperty("tags").getArray()) {
            for (const auto& tag : *tagArray)
                meta.tags.add(tag.toString());
        }
        
        meta.createdDate = juce::Time(static_cast<juce::int64>(obj->getProperty("createdDate")));
        meta.lastModifiedDate = juce::Time(static_cast<juce::int64>(obj->getProperty("lastModifiedDate")));
    }
    
    return meta;
}

//==============================================================================
// TrackData Implementation
//==============================================================================

juce::var TrackData::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("name", name);
    obj->setProperty("colour", colour.toString());
    obj->setProperty("armed", armed);
    obj->setProperty("muted", muted);
    obj->setProperty("soloed", soloed);
    obj->setProperty("volume", volume);
    obj->setProperty("pan", pan);
    
    juce::var pluginArray;
    for (const auto& plugin : pluginChain)
        pluginArray.append(plugin);
    obj->setProperty("pluginChain", pluginArray);
    
    obj->setProperty("automationData", automationData);
    
    return juce::var(obj.get());
}

TrackData TrackData::fromVar(const juce::var& v) {
    TrackData track;
    
    if (auto* obj = v.getDynamicObject()) {
        track.name = obj->getProperty("name").toString();
        track.colour = juce::Colour::fromString(obj->getProperty("colour").toString());
        track.armed = obj->getProperty("armed");
        track.muted = obj->getProperty("muted");
        track.soloed = obj->getProperty("soloed");
        track.volume = obj->getProperty("volume");
        track.pan = obj->getProperty("pan");
        
        if (auto* pluginArray = obj->getProperty("pluginChain").getArray()) {
            for (const auto& plugin : *pluginArray)
                track.pluginChain.add(plugin.toString());
        }
        
        track.automationData = obj->getProperty("automationData");
    }
    
    return track;
}

//==============================================================================
// ProjectData Implementation
//==============================================================================

juce::var ProjectData::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("metadata", metadata.toVar());
    
    juce::var trackArray;
    for (const auto& track : tracks)
        trackArray.append(track.toVar());
    obj->setProperty("tracks", trackArray);
    
    juce::var audioArray;
    for (const auto& file : audioFiles)
        audioArray.append(file);
    obj->setProperty("audioFiles", audioArray);
    
    obj->setProperty("midiData", midiData);
    obj->setProperty("mixerState", mixerState);
    obj->setProperty("pluginStates", pluginStates);
    
    return juce::var(obj.get());
}

ProjectData ProjectData::fromVar(const juce::var& v) {
    ProjectData data;
    
    if (auto* obj = v.getDynamicObject()) {
        data.metadata = ProjectMetadata::fromVar(obj->getProperty("metadata"));
        
        if (auto* trackArray = obj->getProperty("tracks").getArray()) {
            for (const auto& trackVar : *trackArray)
                data.tracks.push_back(TrackData::fromVar(trackVar));
        }
        
        if (auto* audioArray = obj->getProperty("audioFiles").getArray()) {
            for (const auto& file : *audioArray)
                data.audioFiles.add(file.toString());
        }
        
        data.midiData = obj->getProperty("midiData");
        data.mixerState = obj->getProperty("mixerState");
        data.pluginStates = obj->getProperty("pluginStates");
    }
    
    return data;
}

//==============================================================================
// ProjectTemplate Implementation
//==============================================================================

std::vector<ProjectTemplate> ProjectTemplate::getBuiltInTemplates() {
    std::vector<ProjectTemplate> templates;
    
    // Template 1: Empty Project
    {
        ProjectTemplate t;
        t.name = "Empty Project";
        t.description = "Start from scratch";
        t.category = "Basic";
        t.templateData.metadata.projectName = "Untitled";
        t.templateData.metadata.bpm = 120.0;
        templates.push_back(t);
    }
    
    // Template 2: Hip Hop Beat
    {
        ProjectTemplate t;
        t.name = "Hip Hop Beat";
        t.description = "8 tracks: Kick, Snare, Hi-Hat, 808, Melody, Vocals x2, FX";
        t.category = "Hip Hop";
        t.templateData.metadata.projectName = "Hip Hop Beat";
        t.templateData.metadata.bpm = 140.0;
        t.templateData.metadata.genre = "Hip Hop";
        
        // Create tracks
        TrackData kick; kick.name = "Kick"; kick.colour = juce::Colours::red;
        TrackData snare; snare.name = "Snare"; snare.colour = juce::Colours::orange;
        TrackData hihat; hihat.name = "Hi-Hat"; hihat.colour = juce::Colours::yellow;
        TrackData bass; bass.name = "808 Bass"; bass.colour = juce::Colours::purple;
        TrackData melody; melody.name = "Melody"; melody.colour = juce::Colours::blue;
        TrackData vocal1; vocal1.name = "Lead Vocal"; vocal1.colour = juce::Colours::green;
        TrackData vocal2; vocal2.name = "Backing Vocal"; vocal2.colour = juce::Colours::lightgreen;
        TrackData fx; fx.name = "FX"; fx.colour = juce::Colours::grey;
        
        t.templateData.tracks = {kick, snare, hihat, bass, melody, vocal1, vocal2, fx};
        templates.push_back(t);
    }
    
    // Template 3: EDM Production
    {
        ProjectTemplate t;
        t.name = "EDM Production";
        t.description = "12 tracks for electronic music production";
        t.category = "Electronic";
        t.templateData.metadata.projectName = "EDM Track";
        t.templateData.metadata.bpm = 128.0;
        t.templateData.metadata.genre = "Electronic";
        
        TrackData kick; kick.name = "Kick";
        TrackData bass; bass.name = "Bass";
        TrackData lead; lead.name = "Lead Synth";
        TrackData pad; pad.name = "Pad";
        TrackData pluck; pluck.name = "Pluck";
        TrackData fx1; fx1.name = "FX 1";
        TrackData fx2; fx2.name = "FX 2";
        TrackData vocal; vocal.name = "Vocal";
        
        t.templateData.tracks = {kick, bass, lead, pad, pluck, fx1, fx2, vocal};
        templates.push_back(t);
    }
    
    // Template 4: Reggaeton
    {
        ProjectTemplate t;
        t.name = "Reggaeton/Urbano";
        t.description = "Template estilo Bad Bunny/Drake";
        t.category = "Urban";
        t.templateData.metadata.projectName = "Reggaeton Track";
        t.templateData.metadata.bpm = 95.0;
        t.templateData.metadata.genre = "Reggaeton";
        
        TrackData kick; kick.name = "Kick (Dembow)";
        TrackData snare; snare.name = "Snare";
        TrackData timbales; timbales.name = "Timbales";
        TrackData bass; bass.name = "808 Sub";
        TrackData melody; melody.name = "Melody";
        TrackData vocal; vocal.name = "Lead Vocal";
        TrackData adlib; adlib.name = "Ad-libs";
        TrackData fx; fx.name = "FX/Atmosphere";
        
        t.templateData.tracks = {kick, snare, timbales, bass, melody, vocal, adlib, fx};
        templates.push_back(t);
    }
    
    return templates;
}

//==============================================================================
// UndoRedoManager Implementation
//==============================================================================

UndoRedoManager::UndoRedoManager(size_t maxMemoryBytes)
    : maxMemoryBytes(maxMemoryBytes) {}

void UndoRedoManager::performAction(std::unique_ptr<UndoableAction> action) {
    action->perform();
    
    currentMemoryUsage += action->getSizeInBytes();
    undoStack.push_back(std::move(action));
    
    // Clear redo stack
    for (const auto& a : redoStack)
        currentMemoryUsage -= a->getSizeInBytes();
    redoStack.clear();
    
    trimStacksIfNeeded();
    notifyListeners();
}

bool UndoRedoManager::canUndo() const {
    return !undoStack.empty();
}

bool UndoRedoManager::canRedo() const {
    return !redoStack.empty();
}

void UndoRedoManager::undo() {
    if (!canUndo()) return;
    
    auto action = std::move(undoStack.back());
    undoStack.pop_back();
    
    action->undo();
    redoStack.push_back(std::move(action));
    
    notifyListeners();
}

void UndoRedoManager::redo() {
    if (!canRedo()) return;
    
    auto action = std::move(redoStack.back());
    redoStack.pop_back();
    
    action->perform();
    undoStack.push_back(std::move(action));
    
    notifyListeners();
}

void UndoRedoManager::clearHistory() {
    undoStack.clear();
    redoStack.clear();
    currentMemoryUsage = 0;
    notifyListeners();
}

juce::String UndoRedoManager::getUndoDescription() const {
    if (canUndo())
        return undoStack.back()->getDescription();
    return {};
}

juce::String UndoRedoManager::getRedoDescription() const {
    if (canRedo())
        return redoStack.back()->getDescription();
    return {};
}

int UndoRedoManager::getNumActionsInHistory() const {
    return static_cast<int>(undoStack.size() + redoStack.size());
}

void UndoRedoManager::addListener(Listener* listener) {
    listeners.add(listener);
}

void UndoRedoManager::removeListener(Listener* listener) {
    listeners.remove(listener);
}

void UndoRedoManager::trimStacksIfNeeded() {
    while (currentMemoryUsage > maxMemoryBytes && !undoStack.empty()) {
        currentMemoryUsage -= undoStack.front()->getSizeInBytes();
        undoStack.pop_front();
    }
}

void UndoRedoManager::notifyListeners() {
    listeners.call([](Listener& l) { l.undoRedoStateChanged(); });
}

//==============================================================================
// ProjectManager Implementation
//==============================================================================

ProjectManager::ProjectManager() {
    projectData.metadata.createdDate = juce::Time::getCurrentTime();
    projectData.metadata.lastModifiedDate = juce::Time::getCurrentTime();
}

ProjectManager::~ProjectManager() = default;

void ProjectManager::newProject(const juce::String& name) {
    closeProject();
    
    projectData = ProjectData();
    projectData.metadata.projectName = name;
    projectData.metadata.createdDate = juce::Time::getCurrentTime();
    projectData.metadata.lastModifiedDate = juce::Time::getCurrentTime();
    
    state = ProjectState::Empty;
    currentFile = juce::File();
    
    undoRedoManager.clearHistory();
    if (onProjectChanged) onProjectChanged();
}

void ProjectManager::newProjectFromTemplate(const ProjectTemplate& templ) {
    closeProject();
    
    projectData = templ.templateData;
    projectData.metadata.createdDate = juce::Time::getCurrentTime();
    projectData.metadata.lastModifiedDate = juce::Time::getCurrentTime();
    
    state = ProjectState::Empty;
    currentFile = juce::File();
    
    undoRedoManager.clearHistory();
    if (onProjectChanged) onProjectChanged();
}

bool ProjectManager::loadProject(const juce::File& file) {
    if (!file.existsAsFile())
        return false;
    
    state = ProjectState::Loaded;
    
    if (deserializeFromFile(file)) {
        currentFile = file;
        undoRedoManager.clearHistory();
        if (onProjectChanged) onProjectChanged();
        return true;
    }
    
    state = ProjectState::Error;
    return false;
}

bool ProjectManager::saveProject(const juce::File& file) {
    state = ProjectState::Saving;
    
    projectData.metadata.lastModifiedDate = juce::Time::getCurrentTime();
    
    if (serializeToFile(file)) {
        currentFile = file;
        state = ProjectState::Loaded;
        if (onProjectChanged) onProjectChanged();
        return true;
    }
    
    state = ProjectState::Error;
    return false;
}

bool ProjectManager::saveProjectAs(const juce::File& file) {
    return saveProject(file);
}

void ProjectManager::closeProject() {
    // TODO: Prompt to save if modified
    projectData = ProjectData();
    state = ProjectState::Empty;
    currentFile = juce::File();
    undoRedoManager.clearHistory();
    if (onProjectChanged) onProjectChanged();
}

void ProjectManager::setAutosaveEnabled(bool enabled) {
    autosaveEnabled = enabled;
    // Autosave timer removed for simplicity
}

void ProjectManager::setAutosaveInterval(int seconds) {
    autosaveIntervalSeconds = seconds;
    // Autosave timer removed for simplicity
}

void ProjectManager::addTrack(const TrackData& track) {
    projectData.tracks.push_back(track);
    markAsModified();
}

void ProjectManager::removeTrack(int index) {
    if (index >= 0 && index < getNumTracks()) {
        projectData.tracks.erase(projectData.tracks.begin() + index);
        markAsModified();
    }
}

void ProjectManager::moveTrack(int fromIndex, int toIndex) {
    if (fromIndex >= 0 && fromIndex < getNumTracks() &&
        toIndex >= 0 && toIndex < getNumTracks()) {
        auto track = projectData.tracks[fromIndex];
        projectData.tracks.erase(projectData.tracks.begin() + fromIndex);
        projectData.tracks.insert(projectData.tracks.begin() + toIndex, track);
        markAsModified();
    }
}

TrackData& ProjectManager::getTrack(int index) {
    jassert(index >= 0 && index < getNumTracks());
    return projectData.tracks[index];
}

void ProjectManager::markAsModified() {
    if (state == ProjectState::Loaded || state == ProjectState::Empty)
        state = ProjectState::Modified;
    
    projectData.metadata.lastModifiedDate = juce::Time::getCurrentTime();
    if (onProjectChanged) onProjectChanged();
}

bool ProjectManager::exportStemsTracks(const juce::File& directory) {
    // TODO: Implement stems export
    return false;
}

bool ProjectManager::exportMixdown(const juce::File& file, int bitDepth) {
    // TODO: Implement mixdown export
    return false;
}


void ProjectManager::performAutosave() {
    if (!autosaveEnabled || !hasUnsavedChanges())
        return;
    
    if (currentFile.exists()) {
        auto autosaveFile = currentFile.getSiblingFile(currentFile.getFileNameWithoutExtension() + "_autosave.omegastudio");
        serializeToFile(autosaveFile);
    }
}

bool ProjectManager::serializeToFile(const juce::File& file) {
    try {
        auto jsonString = juce::JSON::toString(projectData.toVar(), true);
        return file.replaceWithText(jsonString);
    } catch (...) {
        return false;
    }
}

bool ProjectManager::deserializeFromFile(const juce::File& file) {
    try {
        auto jsonString = file.loadFileAsString();
        auto json = juce::JSON::parse(jsonString);
        
        if (json.isVoid())
            return false;
        
        projectData = ProjectData::fromVar(json);
        return true;
    } catch (...) {
        return false;
    }
}

//==============================================================================
// Undo Actions Implementation
//==============================================================================

void AddTrackAction::perform() {
    projectManager.addTrack(trackData);
    addedIndex = projectManager.getNumTracks() - 1;
}

void AddTrackAction::undo() {
    if (addedIndex >= 0)
        projectManager.removeTrack(addedIndex);
}

void RemoveTrackAction::perform() {
    removedTrack = projectManager.getTrack(trackIndex);
    projectManager.removeTrack(trackIndex);
}

void RemoveTrackAction::undo() {
    projectManager.getProjectData().tracks.insert(
        projectManager.getProjectData().tracks.begin() + trackIndex,
        removedTrack
    );
    projectManager.markAsModified();
}

void ChangeMetadataAction::perform() {
    projectManager.getMetadata() = newMetadata;
    projectManager.markAsModified();
}

void ChangeMetadataAction::undo() {
    projectManager.getMetadata() = oldMetadata;
    projectManager.markAsModified();
}

} // namespace OmegaStudio
