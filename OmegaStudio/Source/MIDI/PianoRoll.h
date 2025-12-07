#pragma once
#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio {

/**
 * @brief FL Studio 2025 Professional Piano Roll
 * Advanced MIDI editor with ghost notes, chords, strumming, humanization
 */

struct MIDINote {
    int noteNumber;
    double startTime;    // In beats
    double length;       // In beats
    uint8 velocity;
    uint8 releaseVelocity = 64;
    float pan = 0.0f;    // -1.0 to 1.0
    int fine Pitch = 0;  // -100 to +100 cents
    juce::Colour colour;
    bool selected = false;
    bool muted = false;
    
    MIDINote() : noteNumber(60), startTime(0), length(1.0), velocity(100) {}
    MIDINote(int note, double start, double len, uint8 vel)
        : noteNumber(note), startTime(start), length(len), velocity(vel) {}
};

class PianoRoll
{
public:
    PianoRoll();
    ~PianoRoll();
    
    // Note management
    void addNote(const MIDINote& note);
    void removeNote(int index);
    void clearNotes();
    std::vector<MIDINote>& getNotes() { return notes; }
    const std::vector<MIDINote>& getNotes() const { return notes; }
    
    // Selection
    void selectNote(int index);
    void deselectNote(int index);
    void selectAll();
    void deselectAll();
    void selectInRange(int minNote, int maxNote, double startTime, double endTime);
    std::vector<int> getSelectedNoteIndices() const;
    
    // Editing operations
    void transpose(int semitones, bool selectedOnly = true);
    void quantize(double gridSize, float strength = 1.0f, bool selectedOnly = true);
    void humanize(float timingAmount, float velocityAmount, bool selectedOnly = true);
    void legato(bool selectedOnly = true);
    void strumChord(float strumTime, bool upward = true, bool selectedOnly = true);
    void arpeggiate(double noteLength, int octaves = 1, bool selectedOnly = true);
    void randomizeVelocity(float amount, bool selectedOnly = true);
    void scaleVelocity(float factor, bool selectedOnly = true);
    void fixedVelocity(uint8 velocity, bool selectedOnly = true);
    
    // Chord tools
    struct Chord {
        juce::String name;
        std::vector<int> intervals;  // Semitones from root
    };
    void insertChord(int rootNote, double startTime, double length, const Chord& chord);
    std::vector<Chord> detectChords() const;
    static Chord getChordByName(const juce::String& name);
    static std::vector<juce::String> getChordTypes();
    
    // Scale tools
    struct Scale {
        juce::String name;
        std::vector<int> intervals;
    };
    void snapToScale(const Scale& scale, int rootNote, bool selectedOnly = true);
    void highlightScale(const Scale& scale, int rootNote);
    static Scale getScaleByName(const juce::String& name);
    static std::vector<juce::String> getScaleTypes();
    
    // Ghost notes (show notes from other patterns)
    struct GhostChannel {
        int channelNumber;
        juce::Colour colour;
        float opacity = 0.3f;
        std::vector<MIDINote> notes;
    };
    void addGhostChannel(int channelNumber, const std::vector<MIDINote>& notes);
    void removeGhostChannel(int channelNumber);
    void clearGhostChannels();
    std::vector<GhostChannel>& getGhostChannels() { return ghostChannels; }
    
    // Note properties editing
    void setNoteVelocity(int index, uint8 velocity);
    void setNotePan(int index, float pan);
    void setNoteFinePitch(int index, int cents);
    void setNoteColour(int index, juce::Colour colour);
    
    // Clipboard operations
    void cut();
    void copy();
    void paste(double atTime);
    void duplicate(double offset = 1.0);
    void deleteSelected();
    
    // Grid/Snap
    enum class GridSize {
        Bar,
        Half,
        Quarter,
        Eighth,
        Sixteenth,
        ThirtySecond,
        Triplet,
        Dotted,
        Free
    };
    void setGridSize(GridSize size) { gridSize = size; }
    GridSize getGridSize() const { return gridSize; }
    double snapToGrid(double time) const;
    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool getSnapEnabled() const { return snapEnabled; }
    
    // View settings
    void setVisibleNoteRange(int minNote, int maxNote);
    juce::Range<int> getVisibleNoteRange() const { return visibleNoteRange; }
    void setZoom(float horizontal, float vertical);
    
    // Pattern length
    void setLengthInBeats(double beats) { lengthInBeats = beats; }
    double getLengthInBeats() const { return lengthInBeats; }
    
    // Swing
    void setSwing(float amount) { swingAmount = amount; }
    float getSwing() const { return swingAmount; }
    void applySwing(bool selectedOnly = true);
    
    // Riff machine (AI-assisted melody generation)
    struct RiffSettings {
        Scale scale;
        int rootNote = 60;
        double length = 4.0;
        int complexity = 2;  // 1-5
        float rhythmDensity = 0.5f;
        bool useExistingNotes = false;
    };
    void generateRiff(const RiffSettings& settings);
    void generateChordProgression(const Scale& scale, int rootNote, int bars);
    
    // Pattern operations
    void flipHorizontal();
    void flipVertical(int centerNote = 60);
    void stretch(float factor);
    void compress(float factor);
    void reverse();
    
    // Note colors (FL Studio style)
    enum class NoteColorMode {
        None,
        Velocity,
        Pitch,
        Channel,
        Manual
    };
    void setNoteColorMode(NoteColorMode mode) { noteColorMode = mode; }
    NoteColorMode getNoteColorMode() const { return noteColorMode; }
    void colorizeNotes();
    
    // Portamento/Slide
    void addPortamento(int fromIndex, int toIndex);
    void removePortamento(int index);
    
    // LFO tool
    struct LFOSettings {
        enum class Target { Velocity, Pan, Pitch, ModX, ModY };
        enum class Shape { Sine, Triangle, Square, Saw, Random };
        
        Target target = Target::Velocity;
        Shape shape = Shape::Sine;
        float frequency = 1.0f;
        float amount = 0.5f;
        float phase = 0.0f;
    };
    void applyLFO(const LFOSettings& settings, bool selectedOnly = true);
    
    // Export/Import
    juce::MidiMessageSequence exportToMidiSequence() const;
    void importFromMidiSequence(const juce::MidiMessageSequence& sequence);
    
    // Serialization
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);
    
    // Statistics
    int getLowestNote() const;
    int getHighestNote() const;
    int getTotalNotes() const { return static_cast<int>(notes.size()); }
    double getTotalDuration() const;

private:
    std::vector<MIDINote> notes;
    std::vector<GhostChannel> ghostChannels;
    
    GridSize gridSize = GridSize::Sixteenth;
    bool snapEnabled = true;
    juce::Range<int> visibleNoteRange{0, 127};
    float horizontalZoom = 1.0f;
    float verticalZoom = 1.0f;
    double lengthInBeats = 16.0;
    float swingAmount = 0.0f;
    NoteColorMode noteColorMode = NoteColorMode::None;
    
    // Clipboard
    struct ClipboardData {
        std::vector<MIDINote> notes;
        double earliestTime = 0.0;
    };
    ClipboardData clipboard;
    
    // Helper functions
    void sortNotes();
    double quantizeTime(double time, double grid, float strength) const;
    juce::Colour getVelocityColour(uint8 velocity) const;
    juce::Colour getPitchColour(int note) const;
    
    // Chord database
    static std::map<juce::String, std::vector<int>> initChordDatabase();
    static std::map<juce::String, std::vector<int>> initScaleDatabase();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRoll)
};

} // namespace OmegaStudio
