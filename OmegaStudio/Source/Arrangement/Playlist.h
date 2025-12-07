#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioClip.h"
#include <vector>
#include <memory>

namespace OmegaStudio {

/**
 * @brief FL Studio 2025 Professional Playlist
 * Complete arrangement system with patterns, audio clips, automation, time signature changes
 */

// Forward declarations
class Pattern;
class AutomationClip;

/**
 * @brief Time Signature Change Point
 */
struct TimeSignatureChange {
    double barPosition;
    int numerator = 4;
    int denominator = 4;
    
    TimeSignatureChange(double bar = 0.0, int num = 4, int denom = 4)
        : barPosition(bar), numerator(num), denominator(denom) {}
};

/**
 * @brief Tempo Change Point
 */
struct TempoChange {
    double barPosition;
    double bpm;
    bool isGradual = false;  // Tempo automation
    
    TempoChange(double bar = 0.0, double tempo = 120.0)
        : barPosition(bar), bpm(tempo) {}
};

/**
 * @brief Playlist Track
 */
class PlaylistTrack
{
public:
    enum class TrackType {
        Audio,
        Pattern,
        Automation,
        Video,
        Group
    };

    PlaylistTrack(const juce::String& name, TrackType type);
    ~PlaylistTrack();

    // Track properties
    void setName(const juce::String& name) { trackName = name; }
    juce::String getName() const { return trackName; }
    void setColour(juce::Colour colour) { trackColour = colour; }
    juce::Colour getColour() const { return trackColour; }
    TrackType getType() const { return type; }
    
    // Track state
    void setMuted(bool muted) { isMuted = muted; }
    bool getMuted() const { return isMuted; }
    void setSolo(bool solo) { isSolo = solo; }
    bool getSolo() const { return isSolo; }
    void setLocked(bool locked) { isLocked = locked; }
    bool getLocked() const { return isLocked; }
    
    // Height control
    void setHeight(int height) { trackHeight = juce::jlimit(20, 400, height); }
    int getHeight() const { return trackHeight; }
    
    // Items on track
    struct TrackItem {
        double startBar;
        double lengthBars;
        std::shared_ptr<void> data;  // Pattern*, AudioClip*, or AutomationClip*
        juce::String itemType;
        juce::Colour colour;
        bool selected = false;
    };
    
    void addItem(const TrackItem& item);
    void removeItem(int index);
    void clearItems();
    std::vector<TrackItem>& getItems() { return items; }
    const std::vector<TrackItem>& getItems() const { return items; }
    
    // Serialization
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);

private:
    juce::String trackName;
    TrackType type;
    juce::Colour trackColour;
    bool isMuted = false;
    bool isSolo = false;
    bool isLocked = false;
    int trackHeight = 50;
    std::vector<TrackItem> items;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistTrack)
};

/**
 * @brief Pattern - MIDI/Note Pattern
 */
class Pattern
{
public:
    Pattern(const juce::String& name, int lengthBars = 4);
    ~Pattern();
    
    void setName(const juce::String& name) { patternName = name; }
    juce::String getName() const { return patternName; }
    
    void setLengthInBars(int bars) { lengthBars = bars; }
    int getLengthInBars() const { return lengthBars; }
    
    void setColour(juce::Colour colour) { patternColour = colour; }
    juce::Colour getColour() const { return patternColour; }
    
    // MIDI data
    juce::MidiBuffer& getMidiBuffer() { return midiData; }
    const juce::MidiBuffer& getMidiBuffer() const { return midiData; }
    void clear() { midiData.clear(); }
    
    // Channel assignment
    void setChannelNumber(int channel) { channelNumber = channel; }
    int getChannelNumber() const { return channelNumber; }
    
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);

private:
    juce::String patternName;
    int lengthBars;
    juce::Colour patternColour;
    juce::MidiBuffer midiData;
    int channelNumber = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Pattern)
};

/**
 * @brief Automation Clip
 */
class AutomationClip
{
public:
    struct AutomationPoint {
        double time;        // In beats
        float value;        // 0.0 to 1.0
        float tension = 0.0f;  // Bezier curve
    };
    
    AutomationClip(const juce::String& targetParameter);
    ~AutomationClip();
    
    void setTargetParameter(const juce::String& param) { targetParameter = param; }
    juce::String getTargetParameter() const { return targetParameter; }
    
    void addPoint(double time, float value, float tension = 0.0f);
    void removePoint(int index);
    void movePoint(int index, double newTime, float newValue);
    void clearPoints();
    
    std::vector<AutomationPoint>& getPoints() { return points; }
    const std::vector<AutomationPoint>& getPoints() const { return points; }
    
    float getValueAtTime(double time) const;
    
    void setLengthInBars(int bars) { lengthBars = bars; }
    int getLengthInBars() const { return lengthBars; }
    
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);

private:
    juce::String targetParameter;
    std::vector<AutomationPoint> points;
    int lengthBars = 4;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationClip)
};

/**
 * @brief Main Playlist
 */
class Playlist
{
public:
    Playlist();
    ~Playlist();
    
    // Track management
    PlaylistTrack* addTrack(const juce::String& name, PlaylistTrack::TrackType type);
    void removeTrack(int index);
    void clearTracks();
    PlaylistTrack* getTrack(int index);
    int getNumTracks() const { return static_cast<int>(tracks.size()); }
    
    // Track ordering
    void moveTrack(int fromIndex, int toIndex);
    
    // Time signature
    void addTimeSignatureChange(double bar, int numerator, int denominator);
    void removeTimeSignatureChange(int index);
    TimeSignatureChange getTimeSignatureAtBar(double bar) const;
    std::vector<TimeSignatureChange>& getTimeSignatureChanges() { return timeSignatures; }
    
    // Tempo
    void addTempoChange(double bar, double bpm);
    void removeTempoChange(int index);
    double getTempoAtBar(double bar) const;
    std::vector<TempoChange>& getTempoChanges() { return tempoChanges; }
    
    // Pattern management
    Pattern* createPattern(const juce::String& name, int lengthBars = 4);
    void deletePattern(Pattern* pattern);
    std::vector<Pattern*> getAllPatterns() { return patterns; }
    
    // Automation management
    AutomationClip* createAutomationClip(const juce::String& targetParameter);
    void deleteAutomationClip(AutomationClip* clip);
    std::vector<AutomationClip*> getAllAutomationClips() { return automationClips; }
    
    // Playback
    void setLoopPoints(double startBar, double endBar);
    void clearLoopPoints();
    juce::Range<double> getLoopRange() const { return loopRange; }
    bool isLoopEnabled() const { return loopEnabled; }
    void setLoopEnabled(bool enabled) { loopEnabled = enabled; }
    
    // Song length
    void setLengthInBars(int bars) { lengthInBars = bars; }
    int getLengthInBars() const { return lengthInBars; }
    double getLengthInSeconds() const;
    
    // Markers
    struct Marker {
        double barPosition;
        juce::String name;
        juce::Colour colour;
    };
    void addMarker(double bar, const juce::String& name);
    void removeMarker(int index);
    std::vector<Marker>& getMarkers() { return markers; }
    
    // Grid/Snap
    enum class SnapMode {
        None,
        Bar,
        Beat,
        HalfBeat,
        QuarterBeat,
        Eighth,
        Sixteenth,
        Triplet
    };
    void setSnapMode(SnapMode mode) { snapMode = mode; }
    SnapMode getSnapMode() const { return snapMode; }
    double snapToGrid(double barPosition) const;
    
    // Selection
    void selectAll();
    void deselectAll();
    std::vector<PlaylistTrack::TrackItem*> getSelectedItems();
    
    // Clipboard
    void cut();
    void copy();
    void paste(double atBar);
    void deleteSelected();
    
    // Serialization
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);
    
    // Export
    bool exportMidiRegion(double startBar, double endBar, const juce::File& file);

private:
    std::vector<std::unique_ptr<PlaylistTrack>> tracks;
    std::vector<Pattern*> patterns;
    std::vector<AutomationClip*> automationClips;
    
    std::vector<TimeSignatureChange> timeSignatures;
    std::vector<TempoChange> tempoChanges;
    std::vector<Marker> markers;
    
    int lengthInBars = 64;
    juce::Range<double> loopRange;
    bool loopEnabled = false;
    SnapMode snapMode = SnapMode::Beat;
    
    // Clipboard
    struct ClipboardData {
        std::vector<PlaylistTrack::TrackItem> items;
    };
    ClipboardData clipboard;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Playlist)
};

} // namespace OmegaStudio
