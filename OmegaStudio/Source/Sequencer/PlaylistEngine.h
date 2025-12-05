#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <unordered_map>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @brief Pattern instance in playlist (like FL Studio)
 * Can be clone or unique pattern
 */
struct PatternInstance {
    int patternId{-1};           // Reference to pattern in PatternBank
    int trackIndex{0};            // Which playlist track
    double startTime{0.0};        // In bars
    double length{4.0};           // In bars
    juce::Colour colour;
    juce::String name;
    bool isUnique{false};         // If true, edits don't affect other instances
    bool isMuted{false};
    int velocity{100};            // Global velocity offset
    
    // Unique pattern modifications (if isUnique == true)
    std::unique_ptr<juce::MidiMessageSequence> uniqueMidiData;
    
    juce::Rectangle<float> bounds; // For rendering
    
    PatternInstance() : colour(juce::Colours::orange) {}
};

/**
 * @brief Pattern definition (reusable)
 */
struct Pattern {
    int id{0};
    juce::String name{"Pattern"};
    juce::Colour colour{juce::Colours::orange};
    double lengthInBars{4.0};
    
    // MIDI data
    juce::MidiMessageSequence midiSequence;
    
    // Automation data
    std::unordered_map<juce::String, juce::Array<std::pair<double, float>>> automationCurves;
    
    // Audio clips (for audio patterns)
    struct AudioClip {
        juce::String filePath;
        double startOffset{0.0};
        double length{0.0};
        float gain{1.0f};
    };
    std::vector<AudioClip> audioClips;
    
    Pattern() = default;
    Pattern(int id_, const juce::String& name_) : id(id_), name(name_) {}
    
    // Deep copy for making unique patterns
    Pattern clone() const {
        Pattern p;
        p.id = id;
        p.name = name + " (unique)";
        p.colour = colour;
        p.lengthInBars = lengthInBars;
        p.midiSequence = midiSequence;
        p.automationCurves = automationCurves;
        p.audioClips = audioClips;
        return p;
    }
};

/**
 * @brief Playlist track (horizontal lane)
 */
struct PlaylistTrack {
    int index{0};
    juce::String name{"Track"};
    juce::Colour colour{juce::Colours::grey};
    int height{50};  // pixels
    bool isMuted{false};
    bool isSoloed{false};
    
    std::vector<std::shared_ptr<PatternInstance>> instances;
    
    PlaylistTrack() = default;
    PlaylistTrack(int idx, const juce::String& n) : index(idx), name(n) {}
};

/**
 * @brief Main Playlist Engine (FL Studio-style arrangement)
 */
class PlaylistEngine {
public:
    PlaylistEngine();
    ~PlaylistEngine();
    
    // Pattern Bank Management
    int createPattern(const juce::String& name = "Pattern");
    void deletePattern(int patternId);
    Pattern* getPattern(int patternId);
    const std::vector<std::shared_ptr<Pattern>>& getAllPatterns() const { return patternBank_; }
    
    // Pattern Instance Management
    std::shared_ptr<PatternInstance> addPatternToPlaylist(
        int patternId, 
        int trackIndex, 
        double startTime,
        double length = -1.0  // -1 = use pattern default length
    );
    
    void removePatternInstance(std::shared_ptr<PatternInstance> instance);
    void movePatternInstance(std::shared_ptr<PatternInstance> instance, int newTrack, double newStartTime);
    void resizePatternInstance(std::shared_ptr<PatternInstance> instance, double newLength);
    
    // Make pattern unique (break link to original)
    void makePatternUnique(std::shared_ptr<PatternInstance> instance);
    
    // Clone pattern instance (still linked)
    std::shared_ptr<PatternInstance> clonePatternInstance(std::shared_ptr<PatternInstance> source);
    
    // Track Management
    void addTrack(const juce::String& name = "Track");
    void removeTrack(int trackIndex);
    void moveTrack(int fromIndex, int toIndex);
    PlaylistTrack* getTrack(int index);
    const std::vector<PlaylistTrack>& getTracks() const { return tracks_; }
    
    // Playback
    void prepareToPlay(double sampleRate, int blockSize);
    void getNextMidiBlock(juce::MidiBuffer& buffer, double startTime, double endTime);
    void getNextAudioBlock(juce::AudioBuffer<float>& buffer, double startTime, double endTime);
    
    // Selection & Editing
    void selectPattern(std::shared_ptr<PatternInstance> instance);
    void selectMultiple(const std::vector<std::shared_ptr<PatternInstance>>& instances);
    void deleteSelected();
    void duplicateSelected();
    void splitPatternAtTime(std::shared_ptr<PatternInstance> instance, double time);
    void mergePatterns(std::shared_ptr<PatternInstance> first, std::shared_ptr<PatternInstance> second);
    
    // Grid & Snapping
    void setGridSize(double bars) { gridSize_ = bars; }
    double getGridSize() const { return gridSize_; }
    double snapToGrid(double time) const;
    
    // Zoom & View
    void setPixelsPerBar(double ppb) { pixelsPerBar_ = ppb; }
    double getPixelsPerBar() const { return pixelsPerBar_; }
    void setViewStart(double bars) { viewStart_ = bars; }
    double getViewStart() const { return viewStart_; }
    
    // Color coding
    void setPatternColor(int patternId, juce::Colour color);
    void setInstanceColor(std::shared_ptr<PatternInstance> instance, juce::Colour color);
    
    // Export
    juce::MidiFile exportToMidi() const;
    void exportToAudio(const juce::File& outputFile, double startTime, double endTime);
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    // Pattern bank (reusable patterns)
    std::vector<std::shared_ptr<Pattern>> patternBank_;
    int nextPatternId_{1};
    
    // Playlist tracks
    std::vector<PlaylistTrack> tracks_;
    
    // Selection
    std::vector<std::shared_ptr<PatternInstance>> selectedInstances_;
    
    // Grid & View
    double gridSize_{0.25};      // 1/4 bar
    double pixelsPerBar_{100.0};
    double viewStart_{0.0};
    
    // Playback state
    double sampleRate_{44100.0};
    int blockSize_{512};
    
    // Helper functions
    juce::MidiMessageSequence getPatternMidi(const PatternInstance& instance, double startTime) const;
    void renderPatternAudio(const PatternInstance& instance, juce::AudioBuffer<float>& buffer, 
                           double startTime, double endTime);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistEngine)
};

/**
 * @brief Playlist GUI Component (FL Studio-style)
 */
class PlaylistComponent : public juce::Component,
                          public juce::DragAndDropContainer {
public:
    PlaylistComponent(PlaylistEngine& engine);
    ~PlaylistComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    // Drag & Drop support
    void itemDropped(const juce::String& description, int x, int y);
    
private:
    PlaylistEngine& engine_;
    
    // View state
    double zoom_{1.0};
    double scrollX_{0.0};
    double scrollY_{0.0};
    
    // Interaction state
    enum class Tool { Select, Draw, Slice, Paint };
    Tool currentTool_{Tool::Select};
    
    std::shared_ptr<PatternInstance> hoveredInstance_;
    std::shared_ptr<PatternInstance> draggingInstance_;
    juce::Point<int> dragStartPos_;
    
    // Rendering helpers
    void drawGrid(juce::Graphics& g);
    void drawTracks(juce::Graphics& g);
    void drawPatternInstances(juce::Graphics& g);
    void drawTimeRuler(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    
    // Hit testing
    std::shared_ptr<PatternInstance> getInstanceAt(int x, int y);
    int getTrackIndexAt(int y);
    double getTimeAt(int x);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)
};

} // namespace Sequencer
} // namespace OmegaStudio
