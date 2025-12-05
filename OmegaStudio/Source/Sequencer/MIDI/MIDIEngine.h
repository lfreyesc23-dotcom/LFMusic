/*
  ==============================================================================
    MIDIEngine.h
    
    Sistema completo de MIDI:
    - MIDI recording/playback
    - Piano roll data structure
    - Note editing (add, remove, move, resize)
    - Velocity & CC editing
    - Quantization engine
    - Humanization
    - MIDI routing
    
    Professional MIDI sequencer core
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <map>

namespace OmegaStudio {

//==============================================================================
/** Nota MIDI */
struct MIDINote {
    int noteNumber { 60 };      // 0-127 (Middle C = 60)
    float startBeat { 0.0f };    // Position en beats
    float lengthBeats { 1.0f };  // Duration en beats
    uint8_t velocity { 100 };    // 0-127
    uint8_t channel { 1 };       // 1-16
    
    // Helpers
    double getStartTime(double bpm) const {
        return (startBeat / bpm) * 60.0;
    }
    
    double getEndBeat() const {
        return startBeat + lengthBeats;
    }
    
    double getEndTime(double bpm) const {
        return (getEndBeat() / bpm) * 60.0;
    }
    
    bool overlaps(const MIDINote& other) const {
        return noteNumber == other.noteNumber &&
               startBeat < other.getEndBeat() &&
               getEndBeat() > other.startBeat;
    }
    
    juce::var toVar() const;
    static MIDINote fromVar(const juce::var& v);
};

//==============================================================================
/** CC (Control Change) event */
struct MIDICCEvent {
    int ccNumber { 1 };          // 0-127 (1=Mod Wheel, 7=Volume, 10=Pan, etc.)
    uint8_t value { 64 };        // 0-127
    float beat { 0.0f };         // Position en beats
    uint8_t channel { 1 };       // 1-16
    
    juce::var toVar() const;
    static MIDICCEvent fromVar(const juce::var& v);
};

//==============================================================================
/** Clip MIDI - Contiene notas y eventos CC */
class MIDIClip {
public:
    MIDIClip() : name("MIDI Clip") {}
    MIDIClip(const juce::String& name) : name(name) {}
    MIDIClip(const MIDIClip&) = default;
    MIDIClip& operator=(const MIDIClip&) = default;
    
    // Properties
    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    
    juce::Colour getColour() const { return colour; }
    void setColour(juce::Colour newColour) { colour = newColour; }
    
    float getStartBeat() const { return startBeat; }
    void setStartBeat(float beat) { startBeat = beat; }
    
    float getLengthBeats() const { return lengthBeats; }
    void setLengthBeats(float length) { lengthBeats = length; }
    
    // Notes
    void addNote(const MIDINote& note);
    void removeNote(int index);
    void removeNotesInRange(float startBeat, float endBeat);
    void clearNotes();
    
    int getNumNotes() const { return static_cast<int>(notes.size()); }
    MIDINote& getNote(int index) { return notes[index]; }
    const MIDINote& getNote(int index) const { return notes[index]; }
    const std::vector<MIDINote>& getNotes() const { return notes; }
    
    std::vector<MIDINote> getNotesInRange(float startBeat, float endBeat) const;
    
    // CC Events
    void addCCEvent(const MIDICCEvent& event);
    void removeCCEvent(int index);
    void clearCCEvents();
    
    int getNumCCEvents() const { return static_cast<int>(ccEvents.size()); }
    const MIDICCEvent& getCCEvent(int index) const { return ccEvents[index]; }
    const std::vector<MIDICCEvent>& getCCEvents() const { return ccEvents; }
    
    // Editing
    void transpose(int semitones);
    void shiftTiming(float beatOffset);
    void scaleVelocity(float multiplier);
    void quantize(float gridSize);
    void humanize(float amountTiming, float amountVelocity);
    
    // Serialization
    juce::var toVar() const;
    static MIDIClip fromVar(const juce::var& v);
    
private:
    juce::String name;
    juce::Colour colour { juce::Colours::green };
    float startBeat { 0.0f };
    float lengthBeats { 4.0f };
    
    std::vector<MIDINote> notes;
    std::vector<MIDICCEvent> ccEvents;
    
    void sortNotes();
};

//==============================================================================
/** Track MIDI - Contiene múltiples clips */
class MIDITrack {
public:
    MIDITrack(const juce::String& name = "MIDI Track");
    
    // Properties
    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    
    uint8_t getMIDIChannel() const { return midiChannel; }
    void setMIDIChannel(uint8_t channel) { midiChannel = juce::jlimit<uint8_t>(1, 16, channel); }
    
    bool isMuted() const { return muted; }
    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    
    bool isSoloed() const { return soloed; }
    void setSoloed(bool shouldBeSoloed) { soloed = shouldBeSoloed; }
    
    // Clips
    void addClip(std::unique_ptr<MIDIClip> clip);
    void removeClip(int index);
    void clearClips();
    
    int getNumClips() const { return static_cast<int>(clips.size()); }
    MIDIClip* getClip(int index) { return clips[index].get(); }
    const MIDIClip* getClip(int index) const { return clips[index].get(); }
    
    // Rendering
    void renderToMIDIBuffer(juce::MidiBuffer& buffer, 
                           double startTime, double endTime, 
                           double bpm, int sampleRate) const;
    
    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<MIDITrack> fromVar(const juce::var& v);
    
    // Direct access for stats
    const std::vector<std::unique_ptr<MIDIClip>>& getClips() const { return clips; }
    
private:
    juce::String name;
    uint8_t midiChannel { 1 };
    bool muted { false };
    bool soloed { false };
    
    std::vector<std::unique_ptr<MIDIClip>> clips;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDITrack)
};

//==============================================================================
/** Quantization settings */
struct QuantizeSettings {
    enum class Grid {
        Whole = 4,
        Half = 2,
        Quarter = 1,
        Eighth = 0,     // 1/8 = 0.5 beats
        Sixteenth = -1,  // 1/16 = 0.25 beats
        ThirtySecond = -2,
        Triplet = -3
    };
    
    Grid grid { Grid::Sixteenth };
    float strength { 1.0f };  // 0.0 = no quantize, 1.0 = full quantize
    bool quantizeStartTime { true };
    bool quantizeLength { false };
    
    float getGridSizeBeats() const {
        switch (grid) {
            case Grid::Whole: return 4.0f;
            case Grid::Half: return 2.0f;
            case Grid::Quarter: return 1.0f;
            case Grid::Eighth: return 0.5f;
            case Grid::Sixteenth: return 0.25f;
            case Grid::ThirtySecond: return 0.125f;
            case Grid::Triplet: return 1.0f / 3.0f;
            default: return 0.25f;
        }
    }
};

//==============================================================================
/** Humanization settings */
struct HumanizeSettings {
    float timingVariation { 0.05f };    // ±5% timing variation
    float velocityVariation { 0.1f };   // ±10% velocity variation
    int seed { 12345 };                 // Random seed
};

//==============================================================================
/** MIDI Engine - Motor principal */
class MIDIEngine {
public:
    MIDIEngine();
    ~MIDIEngine();
    
    // Tracks
    void addTrack(std::unique_ptr<MIDITrack> track);
    void removeTrack(int index);
    void clearTracks();
    
    int getNumTracks() const { return static_cast<int>(tracks.size()); }
    MIDITrack* getTrack(int index) { return tracks[index].get(); }
    const MIDITrack* getTrack(int index) const { return tracks[index].get(); }
    
    // Playback
    void renderMIDI(juce::MidiBuffer& buffer, 
                    double startTime, double endTime,
                    double bpm, int sampleRate) const;
    
    // Recording
    void startRecording(int trackIndex);
    void stopRecording();
    bool isRecording() const { return recording; }
    
    // Stats
    int getClipCount() const {
        int total = 0;
        for (const auto& track : tracks)
            total += track->getClips().size();
        return total;
    }
    
    int getTotalNoteCount() const {
        int total = 0;
        for (const auto& track : tracks)
            for (const auto& clip : track->getClips())
                total += clip->getNotes().size();
        return total;
    }
    int getRecordingTrack() const { return recordingTrackIndex; }
    
    void recordMIDIMessage(const juce::MidiMessage& message, double timestamp);
    
    // Editing operations (aplicar a selección)
    void quantizeNotes(const std::vector<MIDINote*>& notes, const QuantizeSettings& settings);
    void humanizeNotes(std::vector<MIDINote*>& notes, const HumanizeSettings& settings);
    void transposeNotes(const std::vector<MIDINote*>& notes, int semitones);
    void scaleVelocities(const std::vector<MIDINote*>& notes, float multiplier);
    
    // Serialization
    juce::var toVar() const;
    void loadFromVar(const juce::var& v);
    
private:
    std::vector<std::unique_ptr<MIDITrack>> tracks;
    
    bool recording { false };
    int recordingTrackIndex { -1 };
    std::unique_ptr<MIDIClip> recordingClip;
    double recordStartTime { 0.0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIEngine)
};

//==============================================================================
/** MIDI Input Manager - Gestión de dispositivos MIDI */
class MIDIInputManager : public juce::MidiInputCallback {
public:
    MIDIInputManager();
    ~MIDIInputManager() override;
    
    // Devices
    juce::StringArray getAvailableDevices() const;
    bool openDevice(const juce::String& deviceName);
    void closeDevice();
    bool isDeviceOpen() const { return currentDevice != nullptr; }
    juce::String getCurrentDeviceName() const;
    
    // Callbacks
    struct Listener {
        virtual ~Listener() = default;
        virtual void handleIncomingMidiMessage(const juce::MidiMessage& message) = 0;
    };
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // MidiInputCallback override
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;
    
private:
    std::unique_ptr<juce::MidiInput> currentDevice;
    juce::ListenerList<Listener> listeners;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIInputManager)
};

//==============================================================================
/** Piano Roll Data Model */
class PianoRollModel {
public:
    PianoRollModel();
    
    // View settings
    void setVerticalZoom(float zoom);
    void setHorizontalZoom(float zoom);
    float getVerticalZoom() const { return verticalZoom; }
    float getHorizontalZoom() const { return horizontalZoom; }
    
    void setViewPosition(float beat, int lowestNote);
    float getViewStartBeat() const { return viewStartBeat; }
    int getLowestVisibleNote() const { return lowestVisibleNote; }
    
    // Grid
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool isSnapEnabled() const { return snapEnabled; }
    
    void setGridSize(float beats) { gridSize = beats; }
    float getGridSize() const { return gridSize; }
    
    float snapToGrid(float beat) const {
        if (!snapEnabled) return beat;
        return std::round(beat / gridSize) * gridSize;
    }
    
    // Active clip
    void setActiveClip(MIDIClip* clip) { activeClip = clip; }
    MIDIClip* getActiveClip() const { return activeClip; }
    
private:
    MIDIClip* activeClip { nullptr };
    
    float verticalZoom { 1.0f };    // Note height multiplier
    float horizontalZoom { 1.0f };  // Beat width multiplier
    
    float viewStartBeat { 0.0f };
    int lowestVisibleNote { 36 };   // C2
    
    bool snapEnabled { true };
    float gridSize { 0.25f };       // 1/16 note
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollModel)
};

} // namespace OmegaStudio
