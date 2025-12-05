/*
  ==============================================================================
    Timeline.h
    
    Sistema de timeline/arrangement:
    - Playlist editor (FL Studio style)
    - Regiones de audio/MIDI
    - Markers y secciones
    - Tempo automation
    - Time signature changes
    - Loop regions
    - Snap to grid
    
    Professional timeline/arrangement view
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {

//==============================================================================
/** Marker en el timeline */
struct Marker {
    juce::String name;
    double timeBeat { 0.0 };
    juce::Colour colour { juce::Colours::yellow };
    
    juce::var toVar() const;
    static Marker fromVar(const juce::var& v);
};

//==============================================================================
/** Tiempo signature change */
struct TimeSignatureChange {
    double beat { 0.0 };
    int numerator { 4 };
    int denominator { 4 };
    
    juce::var toVar() const;
    static TimeSignatureChange fromVar(const juce::var& v);
};

//==============================================================================
/** Tempo automation point */
struct TempoPoint {
    double beat { 0.0 };
    double bpm { 120.0 };
    
    enum class CurveType {
        Step,       // Instant change
        Linear,     // Linear interpolation
        Smooth      // Smooth curve
    };
    CurveType curve { CurveType::Linear };
    
    juce::var toVar() const;
    static TempoPoint fromVar(const juce::var& v);
};

//==============================================================================
/** Audio/MIDI Region en el timeline */
class TimelineRegion {
public:
    enum class Type {
        Audio,
        MIDI,
        Automation
    };
    
    TimelineRegion(Type type, const juce::String& name = "Region");
    virtual ~TimelineRegion() = default;
    
    // Properties
    Type getType() const { return type; }
    
    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    
    juce::Colour getColour() const { return colour; }
    void setColour(juce::Colour newColour) { colour = newColour; }
    
    // Position & Length
    double getStartBeat() const { return startBeat; }
    void setStartBeat(double beat) { startBeat = beat; }
    
    double getLengthBeats() const { return lengthBeats; }
    void setLengthBeats(double length) { lengthBeats = length; }
    
    double getEndBeat() const { return startBeat + lengthBeats; }
    
    // Offset (para audio regions)
    double getOffset() const { return offset; }
    void setOffset(double newOffset) { offset = newOffset; }
    
    // Fade in/out
    double getFadeIn() const { return fadeIn; }
    void setFadeIn(double fade) { fadeIn = fade; }
    
    double getFadeOut() const { return fadeOut; }
    void setFadeOut(double fade) { fadeOut = fade; }
    
    // Gain
    float getGain() const { return gain; }
    void setGain(float newGain) { gain = newGain; }
    
    // Mute
    bool isMuted() const { return muted; }
    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    
    // Track assignment
    int getTrackIndex() const { return trackIndex; }
    void setTrackIndex(int index) { trackIndex = index; }
    
    // Serialization
    virtual juce::var toVar() const;
    static std::unique_ptr<TimelineRegion> fromVar(const juce::var& v);
    
protected:
    Type type;
    juce::String name;
    juce::Colour colour;
    
    double startBeat { 0.0 };
    double lengthBeats { 4.0 };
    double offset { 0.0 };
    
    double fadeIn { 0.0 };
    double fadeOut { 0.0 };
    float gain { 1.0f };
    bool muted { false };
    
    int trackIndex { 0 };
    
    JUCE_LEAK_DETECTOR(TimelineRegion)
};

//==============================================================================
/** Audio Region */
class AudioRegion : public TimelineRegion {
public:
    AudioRegion(const juce::String& name = "Audio");
    
    // Audio file reference
    void setAudioFile(const juce::File& file);
    juce::File getAudioFile() const { return audioFile; }
    
    // Stretch & Pitch
    double getTimeStretch() const { return timeStretch; }
    void setTimeStretch(double stretch) { timeStretch = stretch; }
    
    double getPitchShift() const { return pitchShift; }
    void setPitchShift(double semitones) { pitchShift = semitones; }
    
    // Serialization
    juce::var toVar() const override;
    
private:
    juce::File audioFile;
    double timeStretch { 1.0 };
    double pitchShift { 0.0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRegion)
};

//==============================================================================
/** MIDI Region */
class MIDIRegion : public TimelineRegion {
public:
    MIDIRegion(const juce::String& name = "MIDI");
    
    // MIDI clip reference (from MIDIEngine)
    void setMIDIClipID(int id) { midiClipID = id; }
    int getMIDIClipID() const { return midiClipID; }
    
    // Serialization
    juce::var toVar() const override;
    
private:
    int midiClipID { -1 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIRegion)
};

//==============================================================================
/** Timeline - Gestión del arrangement */
class Timeline {
public:
    Timeline();
    ~Timeline();
    
    // Regions
    void addRegion(std::unique_ptr<TimelineRegion> region);
    void removeRegion(int index);
    void clearRegions();
    
    int getNumRegions() const { return static_cast<int>(regions.size()); }
    TimelineRegion* getRegion(int index);
    const TimelineRegion* getRegion(int index) const;
    
    std::vector<TimelineRegion*> getRegionsInRange(double startBeat, double endBeat) const;
    std::vector<TimelineRegion*> getRegionsOnTrack(int trackIndex) const;
    
    // Markers
    void addMarker(const Marker& marker);
    void removeMarker(int index);
    void clearMarkers();
    
    int getNumMarkers() const { return static_cast<int>(markers.size()); }
    const Marker& getMarker(int index) const { return markers[index]; }
    
    // Time Signatures
    void addTimeSignature(const TimeSignatureChange& change);
    void removeTimeSignature(int index);
    TimeSignatureChange getTimeSignatureAt(double beat) const;
    
    // Tempo Automation
    void addTempoPoint(const TempoPoint& point);
    void removeTempoPoint(int index);
    double getTempoAt(double beat) const;
    
    // Playback
    void setPlaybackPosition(double beat) { playbackPositionBeat = beat; }
    double getPlaybackPosition() const { return playbackPositionBeat; }
    
    void setLoopEnabled(bool enabled) { loopEnabled = enabled; }
    bool isLoopEnabled() const { return loopEnabled; }
    
    void setLoopStart(double beat) { loopStart = beat; }
    double getLoopStart() const { return loopStart; }
    
    void setLoopEnd(double beat) { loopEnd = beat; }
    double getLoopEnd() const { return loopEnd; }
    
    // Length
    double getTotalLengthBeats() const;
    double getTotalLengthSeconds(double bpm) const;
    
    // Grid
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool isSnapEnabled() const { return snapEnabled; }
    
    void setGridSize(double beats) { gridSize = beats; }
    double getGridSize() const { return gridSize; }
    
    double snapToGrid(double beat) const {
        if (!snapEnabled) return beat;
        return std::round(beat / gridSize) * gridSize;
    }
    
    // Serialization
    juce::var toVar() const;
    void loadFromVar(const juce::var& v);
    
private:
    std::vector<std::unique_ptr<TimelineRegion>> regions;
    std::vector<Marker> markers;
    std::vector<TimeSignatureChange> timeSignatures;
    std::vector<TempoPoint> tempoPoints;
    
    double playbackPositionBeat { 0.0 };
    bool loopEnabled { false };
    double loopStart { 0.0 };
    double loopEnd { 16.0 };
    
    bool snapEnabled { true };
    double gridSize { 0.25 }; // 1/16 note
    
    void sortMarkers();
    void sortTimeSignatures();
    void sortTempoPoints();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Timeline)
};

//==============================================================================
/** Transport - Control de playback */
class Transport {
public:
    enum class State {
        Stopped,
        Playing,
        Recording,
        Paused
    };
    
    Transport();
    
    // State
    void play();
    void stop();
    void pause();
    void record();
    
    State getState() const { return state; }
    bool isPlaying() const { return state == State::Playing || state == State::Recording; }
    bool isRecording() const { return state == State::Recording; }
    
    // Position
    void setPosition(double beat);
    double getPosition() const { return positionBeat; }
    
    // Tempo
    void setTempo(double bpm);
    double getTempo() const { return tempo; }
    
    // Time signature
    void setTimeSignature(int numerator, int denominator);
    int getTimeSignatureNumerator() const { return timeSignatureNumerator; }
    int getTimeSignatureDenominator() const { return timeSignatureDenominator; }
    
    // Metronome
    void setMetronomeEnabled(bool enabled) { metronomeEnabled = enabled; }
    bool isMetronomeEnabled() const { return metronomeEnabled; }
    
    void setMetronomeVolume(float volume) { metronomeVolume = volume; }
    float getMetronomeVolume() const { return metronomeVolume; }
    
    // Count-in
    void setCountInBars(int bars) { countInBars = bars; }
    int getCountInBars() const { return countInBars; }
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void transportStateChanged(State newState) {}
        virtual void transportPositionChanged(double beat) {}
        virtual void transportTempoChanged(double bpm) {}
    };
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    State state { State::Stopped };
    double positionBeat { 0.0 };
    double tempo { 120.0 };
    
    int timeSignatureNumerator { 4 };
    int timeSignatureDenominator { 4 };
    
    bool metronomeEnabled { true };
    float metronomeVolume { 0.7f };
    int countInBars { 1 };
    
    juce::ListenerList<Listener> listeners;
    
    void notifyStateChanged();
    void notifyPositionChanged();
    void notifyTempoChanged();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Transport)
};

} // namespace OmegaStudio
