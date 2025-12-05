#include "PlaylistEngine.h"

namespace OmegaStudio {
namespace Sequencer {

//==============================================================================
// PlaylistEngine Implementation
//==============================================================================

PlaylistEngine::PlaylistEngine() {
    // Create default track
    addTrack("Main");
    
    // Create default pattern
    createPattern("Pattern 1");
}

PlaylistEngine::~PlaylistEngine() = default;

//==============================================================================
// Pattern Bank Management
//==============================================================================

int PlaylistEngine::createPattern(const juce::String& name) {
    auto pattern = std::make_shared<Pattern>(nextPatternId_++, name);
    pattern->colour = juce::Colour::fromHSV(
        juce::Random::getSystemRandom().nextFloat(), 
        0.7f, 0.9f, 1.0f
    );
    patternBank_.push_back(pattern);
    return pattern->id;
}

void PlaylistEngine::deletePattern(int patternId) {
    // Remove from bank
    patternBank_.erase(
        std::remove_if(patternBank_.begin(), patternBank_.end(),
            [patternId](const auto& p) { return p->id == patternId; }),
        patternBank_.end()
    );
    
    // Remove all instances
    for (auto& track : tracks_) {
        track.instances.erase(
            std::remove_if(track.instances.begin(), track.instances.end(),
                [patternId](const auto& inst) { return inst->patternId == patternId; }),
            track.instances.end()
        );
    }
}

Pattern* PlaylistEngine::getPattern(int patternId) {
    for (auto& pattern : patternBank_) {
        if (pattern->id == patternId) {
            return pattern.get();
        }
    }
    return nullptr;
}

//==============================================================================
// Pattern Instance Management
//==============================================================================

std::shared_ptr<PatternInstance> PlaylistEngine::addPatternToPlaylist(
    int patternId, int trackIndex, double startTime, double length) {
    
    if (trackIndex < 0 || trackIndex >= tracks_.size()) {
        return nullptr;
    }
    
    auto* pattern = getPattern(patternId);
    if (!pattern) {
        return nullptr;
    }
    
    auto instance = std::make_shared<PatternInstance>();
    instance->patternId = patternId;
    instance->trackIndex = trackIndex;
    instance->startTime = snapToGrid(startTime);
    instance->length = (length < 0) ? pattern->lengthInBars : length;
    instance->colour = pattern->colour;
    instance->name = pattern->name;
    
    tracks_[trackIndex].instances.push_back(instance);
    
    return instance;
}

void PlaylistEngine::removePatternInstance(std::shared_ptr<PatternInstance> instance) {
    if (!instance) return;
    
    for (auto& track : tracks_) {
        track.instances.erase(
            std::remove(track.instances.begin(), track.instances.end(), instance),
            track.instances.end()
        );
    }
}

void PlaylistEngine::movePatternInstance(std::shared_ptr<PatternInstance> instance, 
                                         int newTrack, double newStartTime) {
    if (!instance || newTrack < 0 || newTrack >= tracks_.size()) {
        return;
    }
    
    // Remove from old track
    for (auto& track : tracks_) {
        track.instances.erase(
            std::remove(track.instances.begin(), track.instances.end(), instance),
            track.instances.end()
        );
    }
    
    // Add to new track
    instance->trackIndex = newTrack;
    instance->startTime = snapToGrid(newStartTime);
    tracks_[newTrack].instances.push_back(instance);
}

void PlaylistEngine::resizePatternInstance(std::shared_ptr<PatternInstance> instance, 
                                           double newLength) {
    if (instance) {
        instance->length = juce::jmax(0.25, newLength);
    }
}

void PlaylistEngine::makePatternUnique(std::shared_ptr<PatternInstance> instance) {
    if (!instance || instance->isUnique) return;
    
    auto* pattern = getPattern(instance->patternId);
    if (!pattern) return;
    
    // Create unique copy of MIDI data
    instance->uniqueMidiData = std::make_unique<juce::MidiMessageSequence>(pattern->midiSequence);
    instance->isUnique = true;
    instance->name += " (unique)";
}

std::shared_ptr<PatternInstance> PlaylistEngine::clonePatternInstance(
    std::shared_ptr<PatternInstance> source) {
    
    if (!source) return nullptr;
    
    auto clone = std::make_shared<PatternInstance>();
    clone->patternId = source->patternId;
    clone->trackIndex = source->trackIndex;
    clone->startTime = source->startTime + source->length; // Place after original
    clone->length = source->length;
    clone->colour = source->colour;
    clone->name = source->name;
    clone->isUnique = source->isUnique;
    clone->isMuted = source->isMuted;
    clone->velocity = source->velocity;
    
    if (source->isUnique && source->uniqueMidiData) {
        clone->uniqueMidiData = std::make_unique<juce::MidiMessageSequence>(*source->uniqueMidiData);
    }
    
    if (source->trackIndex >= 0 && source->trackIndex < tracks_.size()) {
        tracks_[source->trackIndex].instances.push_back(clone);
    }
    
    return clone;
}

//==============================================================================
// Track Management
//==============================================================================

void PlaylistEngine::addTrack(const juce::String& name) {
    PlaylistTrack track(static_cast<int>(tracks_.size()), name);
    track.colour = juce::Colour::fromHSV(
        juce::Random::getSystemRandom().nextFloat(), 
        0.5f, 0.7f, 1.0f
    );
    tracks_.push_back(track);
}

void PlaylistEngine::removeTrack(int trackIndex) {
    if (trackIndex >= 0 && trackIndex < tracks_.size()) {
        tracks_.erase(tracks_.begin() + trackIndex);
        
        // Update indices
        for (size_t i = 0; i < tracks_.size(); ++i) {
            tracks_[i].index = static_cast<int>(i);
        }
    }
}

void PlaylistEngine::moveTrack(int fromIndex, int toIndex) {
    if (fromIndex >= 0 && fromIndex < tracks_.size() &&
        toIndex >= 0 && toIndex < tracks_.size() && fromIndex != toIndex) {
        
        auto track = tracks_[fromIndex];
        tracks_.erase(tracks_.begin() + fromIndex);
        tracks_.insert(tracks_.begin() + toIndex, track);
        
        // Update indices
        for (size_t i = 0; i < tracks_.size(); ++i) {
            tracks_[i].index = static_cast<int>(i);
        }
    }
}

PlaylistTrack* PlaylistEngine::getTrack(int index) {
    if (index >= 0 && index < tracks_.size()) {
        return &tracks_[index];
    }
    return nullptr;
}

//==============================================================================
// Playback
//==============================================================================

void PlaylistEngine::prepareToPlay(double sampleRate, int blockSize) {
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;
}

void PlaylistEngine::getNextMidiBlock(juce::MidiBuffer& buffer, double startTime, double endTime) {
    buffer.clear();
    
    for (const auto& track : tracks_) {
        if (track.isMuted) continue;
        
        for (const auto& instance : track.instances) {
            if (instance->isMuted) continue;
            
            // Check if pattern is active in this time range
            double instanceEnd = instance->startTime + instance->length;
            if (instanceEnd < startTime || instance->startTime > endTime) {
                continue;
            }
            
            auto midiSeq = getPatternMidi(*instance, startTime);
            
            for (int i = 0; i < midiSeq.getNumEvents(); ++i) {
                auto* event = midiSeq.getEventPointer(i);
                double eventTime = event->message.getTimeStamp();
                
                if (eventTime >= startTime && eventTime < endTime) {
                    auto message = event->message;
                    
                    // Apply velocity offset
                    if (message.isNoteOn()) {
                        int vel = juce::jlimit(1, 127, 
                            message.getVelocity() * instance->velocity / 100);
                        message.setVelocity(static_cast<uint8>(vel));
                    }
                    
                    int sampleOffset = static_cast<int>((eventTime - startTime) * sampleRate_);
                    buffer.addEvent(message, sampleOffset);
                }
            }
        }
    }
}

void PlaylistEngine::getNextAudioBlock(juce::AudioBuffer<float>& buffer, 
                                       double startTime, double endTime) {
    buffer.clear();
    
    for (const auto& track : tracks_) {
        if (track.isMuted) continue;
        
        for (const auto& instance : track.instances) {
            if (instance->isMuted) continue;
            
            double instanceEnd = instance->startTime + instance->length;
            if (instanceEnd < startTime || instance->startTime > endTime) {
                continue;
            }
            
            renderPatternAudio(*instance, buffer, startTime, endTime);
        }
    }
}

//==============================================================================
// Selection & Editing
//==============================================================================

void PlaylistEngine::selectPattern(std::shared_ptr<PatternInstance> instance) {
    selectedInstances_.clear();
    if (instance) {
        selectedInstances_.push_back(instance);
    }
}

void PlaylistEngine::selectMultiple(const std::vector<std::shared_ptr<PatternInstance>>& instances) {
    selectedInstances_ = instances;
}

void PlaylistEngine::deleteSelected() {
    for (auto& instance : selectedInstances_) {
        removePatternInstance(instance);
    }
    selectedInstances_.clear();
}

void PlaylistEngine::duplicateSelected() {
    std::vector<std::shared_ptr<PatternInstance>> newInstances;
    
    for (auto& instance : selectedInstances_) {
        auto clone = clonePatternInstance(instance);
        if (clone) {
            newInstances.push_back(clone);
        }
    }
    
    selectedInstances_ = newInstances;
}

void PlaylistEngine::splitPatternAtTime(std::shared_ptr<PatternInstance> instance, double time) {
    if (!instance || time <= instance->startTime || time >= instance->startTime + instance->length) {
        return;
    }
    
    // Create second half
    auto secondHalf = std::make_shared<PatternInstance>();
    secondHalf->patternId = instance->patternId;
    secondHalf->trackIndex = instance->trackIndex;
    secondHalf->colour = instance->colour;
    secondHalf->name = instance->name;
    secondHalf->isUnique = instance->isUnique;
    secondHalf->isMuted = instance->isMuted;
    secondHalf->velocity = instance->velocity;
    secondHalf->startTime = time;
    secondHalf->length = instance->startTime + instance->length - time;
    
    if (instance->isUnique && instance->uniqueMidiData) {
        secondHalf->uniqueMidiData = std::make_unique<juce::MidiMessageSequence>(*instance->uniqueMidiData);
    }
    
    // Adjust first half
    instance->length = time - instance->startTime;
    
    // Add second half to track
    if (instance->trackIndex >= 0 && instance->trackIndex < tracks_.size()) {
        tracks_[instance->trackIndex].instances.push_back(secondHalf);
    }
}

void PlaylistEngine::mergePatterns(std::shared_ptr<PatternInstance> first, 
                                   std::shared_ptr<PatternInstance> second) {
    if (!first || !second || first->trackIndex != second->trackIndex) {
        return;
    }
    
    // Extend first pattern
    double end1 = first->startTime + first->length;
    double end2 = second->startTime + second->length;
    first->length = juce::jmax(end1, end2) - first->startTime;
    
    // Remove second pattern
    removePatternInstance(second);
}

//==============================================================================
// Grid & Snapping
//==============================================================================

double PlaylistEngine::snapToGrid(double time) const {
    if (gridSize_ <= 0.0) return time;
    return std::round(time / gridSize_) * gridSize_;
}

//==============================================================================
// Color Coding
//==============================================================================

void PlaylistEngine::setPatternColor(int patternId, juce::Colour color) {
    auto* pattern = getPattern(patternId);
    if (pattern) {
        pattern->colour = color;
        
        // Update all instances
        for (auto& track : tracks_) {
            for (auto& instance : track.instances) {
                if (instance->patternId == patternId && !instance->isUnique) {
                    instance->colour = color;
                }
            }
        }
    }
}

void PlaylistEngine::setInstanceColor(std::shared_ptr<PatternInstance> instance, juce::Colour color) {
    if (instance) {
        instance->colour = color;
        if (!instance->isUnique) {
            makePatternUnique(instance);
        }
    }
}

//==============================================================================
// Helper Functions
//==============================================================================

juce::MidiMessageSequence PlaylistEngine::getPatternMidi(const PatternInstance& instance, 
                                                          double startTime) const {
    juce::MidiMessageSequence sequence;
    
    if (instance.isUnique && instance.uniqueMidiData) {
        sequence = *instance.uniqueMidiData;
    } else {
        auto* pattern = const_cast<PlaylistEngine*>(this)->getPattern(instance.patternId);
        if (pattern) {
            sequence = pattern->midiSequence;
        }
    }
    
    // Offset timestamps to match instance position
    for (int i = 0; i < sequence.getNumEvents(); ++i) {
        auto* event = sequence.getEventPointer(i);
        event->message.setTimeStamp(event->message.getTimeStamp() + instance.startTime);
    }
    
    return sequence;
}

void PlaylistEngine::renderPatternAudio(const PatternInstance& instance, 
                                        juce::AudioBuffer<float>& buffer,
                                        double startTime, double endTime) {
    // TODO: Implement audio clip rendering
    // This would handle audio patterns with audio clips
}

//==============================================================================
// Serialization
//==============================================================================

juce::ValueTree PlaylistEngine::toValueTree() const {
    juce::ValueTree tree("PlaylistEngine");
    
    // Save pattern bank
    juce::ValueTree patternsTree("Patterns");
    for (const auto& pattern : patternBank_) {
        juce::ValueTree patternTree("Pattern");
        patternTree.setProperty("id", pattern->id, nullptr);
        patternTree.setProperty("name", pattern->name, nullptr);
        patternTree.setProperty("colour", pattern->colour.toString(), nullptr);
        patternTree.setProperty("length", pattern->lengthInBars, nullptr);
        // TODO: Serialize MIDI data
        patternsTree.appendChild(patternTree, nullptr);
    }
    tree.appendChild(patternsTree, nullptr);
    
    // Save tracks
    juce::ValueTree tracksTree("Tracks");
    for (const auto& track : tracks_) {
        juce::ValueTree trackTree("Track");
        trackTree.setProperty("index", track.index, nullptr);
        trackTree.setProperty("name", track.name, nullptr);
        trackTree.setProperty("colour", track.colour.toString(), nullptr);
        trackTree.setProperty("muted", track.isMuted, nullptr);
        trackTree.setProperty("soloed", track.isSoloed, nullptr);
        
        // Save instances
        for (const auto& instance : track.instances) {
            juce::ValueTree instTree("Instance");
            instTree.setProperty("patternId", instance->patternId, nullptr);
            instTree.setProperty("startTime", instance->startTime, nullptr);
            instTree.setProperty("length", instance->length, nullptr);
            instTree.setProperty("isUnique", instance->isUnique, nullptr);
            instTree.setProperty("muted", instance->isMuted, nullptr);
            instTree.setProperty("velocity", instance->velocity, nullptr);
            trackTree.appendChild(instTree, nullptr);
        }
        
        tracksTree.appendChild(trackTree, nullptr);
    }
    tree.appendChild(tracksTree, nullptr);
    
    return tree;
}

void PlaylistEngine::fromValueTree(const juce::ValueTree& tree) {
    // TODO: Implement deserialization
}

//==============================================================================
// PlaylistComponent Implementation
//==============================================================================

PlaylistComponent::PlaylistComponent(PlaylistEngine& engine) 
    : engine_(engine) {
    setSize(800, 600);
}

PlaylistComponent::~PlaylistComponent() = default;

void PlaylistComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    drawGrid(g);
    drawTracks(g);
    drawPatternInstances(g);
    drawTimeRuler(g);
}

void PlaylistComponent::resized() {
    // TODO: Update scroll bars, zoom controls
}

void PlaylistComponent::mouseDown(const juce::MouseEvent& e) {
    auto instance = getInstanceAt(e.x, e.y);
    
    if (e.mods.isShiftDown()) {
        // Add to selection
    } else {
        engine_.selectPattern(instance);
    }
    
    if (instance) {
        draggingInstance_ = instance;
        dragStartPos_ = e.getPosition();
    }
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent& e) {
    if (draggingInstance_) {
        int trackIndex = getTrackIndexAt(e.y);
        double time = getTimeAt(e.x);
        engine_.movePatternInstance(draggingInstance_, trackIndex, time);
        repaint();
    }
}

void PlaylistComponent::mouseUp(const juce::MouseEvent& e) {
    draggingInstance_ = nullptr;
}

void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    auto instance = getInstanceAt(e.x, e.y);
    if (instance) {
        // TODO: Open pattern editor
    }
}

void PlaylistComponent::drawGrid(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff2a2a2a));
    
    double ppb = engine_.getPixelsPerBar();
    double gridSize = engine_.getGridSize();
    
    for (double bar = 0; bar < 100; bar += gridSize) {
        int x = static_cast<int>(bar * ppb);
        g.drawVerticalLine(x, 0, static_cast<float>(getHeight()));
    }
}

void PlaylistComponent::drawTracks(juce::Graphics& g) {
    const auto& tracks = engine_.getTracks();
    int y = 30; // Offset for ruler
    
    for (const auto& track : tracks) {
        g.setColour(track.colour.withAlpha(0.1f));
        g.fillRect(0, y, getWidth(), track.height);
        
        g.setColour(juce::Colours::white);
        g.drawText(track.name, 5, y, 100, track.height, juce::Justification::centredLeft);
        
        y += track.height;
    }
}

void PlaylistComponent::drawPatternInstances(juce::Graphics& g) {
    const auto& tracks = engine_.getTracks();
    double ppb = engine_.getPixelsPerBar();
    int y = 30;
    
    for (const auto& track : tracks) {
        for (const auto& instance : track.instances) {
            int x = static_cast<int>(instance->startTime * ppb);
            int width = static_cast<int>(instance->length * ppb);
            
            g.setColour(instance->colour);
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y), 
                                   static_cast<float>(width), static_cast<float>(track.height - 2), 3.0f);
            
            g.setColour(juce::Colours::white);
            g.drawText(instance->name, x + 5, y, width - 10, track.height, 
                      juce::Justification::centredLeft, true);
        }
        y += track.height;
    }
}

void PlaylistComponent::drawTimeRuler(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, 0, getWidth(), 30);
    
    g.setColour(juce::Colours::white);
    double ppb = engine_.getPixelsPerBar();
    
    for (int bar = 0; bar < 100; ++bar) {
        int x = static_cast<int>(bar * ppb);
        g.drawText(juce::String(bar + 1), x, 0, static_cast<int>(ppb), 30, 
                  juce::Justification::centred);
    }
}

std::shared_ptr<PatternInstance> PlaylistComponent::getInstanceAt(int x, int y) {
    const auto& tracks = engine_.getTracks();
    double ppb = engine_.getPixelsPerBar();
    int trackY = 30;
    
    for (const auto& track : tracks) {
        if (y >= trackY && y < trackY + track.height) {
            for (const auto& instance : track.instances) {
                int instX = static_cast<int>(instance->startTime * ppb);
                int instWidth = static_cast<int>(instance->length * ppb);
                
                if (x >= instX && x < instX + instWidth) {
                    return instance;
                }
            }
        }
        trackY += track.height;
    }
    
    return nullptr;
}

int PlaylistComponent::getTrackIndexAt(int y) {
    const auto& tracks = engine_.getTracks();
    int trackY = 30;
    
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (y >= trackY && y < trackY + tracks[i].height) {
            return static_cast<int>(i);
        }
        trackY += tracks[i].height;
    }
    
    return static_cast<int>(tracks.size()) - 1;
}

double PlaylistComponent::getTimeAt(int x) {
    return static_cast<double>(x) / engine_.getPixelsPerBar();
}

} // namespace Sequencer
} // namespace OmegaStudio
