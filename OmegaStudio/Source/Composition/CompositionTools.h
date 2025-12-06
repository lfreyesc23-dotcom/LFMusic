//==============================================================================
// CompositionTools.h - Herramientas Profesionales de Composición
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <set>

namespace OmegaStudio {

//==============================================================================
/** Notation View - Vista de partitura musical */
class NotationView : public juce::Component {
public:
    //==========================================================================
    enum class Clef { Treble, Bass, Alto, Tenor };
    enum class Accidental { Natural, Sharp, Flat, DoubleSharp, DoubleFlat };
    
    struct Note {
        int midiNote;                   // 0-127
        double startTime;               // En beats
        double duration;                // En beats
        int velocity;                   // 0-127
        
        Accidental accidental = Accidental::Natural;
        bool tied = false;
        bool dotted = false;
        bool doubleDotted = false;
    };
    
    struct TimeSignature {
        int numerator = 4;
        int denominator = 4;
        double position = 0.0;          // En beats
    };
    
    struct KeySignature {
        int sharps = 0;                 // Positivo para sostenidos, negativo para bemoles
        double position = 0.0;
    };
    
    //==========================================================================
    NotationView();
    ~NotationView() override = default;
    
    // Setup
    void setClef(Clef clef);
    void setTimeSignature(int numerator, int denominator);
    void setKeySignature(int sharps);
    
    // Notes
    void addNote(const Note& note);
    void removeNote(int noteIndex);
    void clearNotes();
    
    const std::vector<Note>& getNotes() const { return notes_; }
    
    // Conversión MIDI <-> Notation
    void importFromMIDI(const juce::MidiBuffer& midiBuffer, double ppq);
    juce::MidiBuffer exportToMIDI(double ppq) const;
    
    // Quantization
    void quantizeNotes(double quantizeValue);  // 1.0 = quarter note, 0.5 = eighth, etc
    
    // Display options
    void setZoom(float zoom);
    void setShowChordNames(bool show);
    void setShowLyrics(bool show);
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
private:
    //==========================================================================
    std::vector<Note> notes_;
    std::vector<TimeSignature> timeSignatures_;
    std::vector<KeySignature> keySignatures_;
    
    Clef currentClef_ = Clef::Treble;
    float zoom_ = 1.0f;
    bool showChordNames_ = true;
    bool showLyrics_ = false;
    
    // Rendering
    void drawStaff(juce::Graphics& g, juce::Rectangle<float> area);
    void drawNote(juce::Graphics& g, const Note& note, float x, float y);
    void drawTimeSignature(juce::Graphics& g, const TimeSignature& ts, float x);
    void drawKeySignature(juce::Graphics& g, const KeySignature& ks, float x);
    
    // Helper
    int getMIDINoteFromStaffPosition(int staffLine) const;
    int getStaffPositionFromMIDINote(int midiNote) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotationView)
};

//==============================================================================
/** Chord Progression System - 100+ progresiones de acordes profesionales */
class ChordProgressionSystem {
public:
    //==========================================================================
    enum class ChordQuality {
        Major, Minor, Diminished, Augmented,
        Major7, Minor7, Dominant7, MinorMaj7,
        Dim7, HalfDim7,
        Sus2, Sus4,
        Add9, Add11,
        Maj9, Min9, Dom9,
        Maj11, Min11, Dom11,
        Maj13, Min13, Dom13
    };
    
    enum class Scale {
        Major, NaturalMinor, HarmonicMinor, MelodicMinor,
        Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian,
        MajorPentatonic, MinorPentatonic,
        Blues, Chromatic,
        WholeTone, Diminished,
        HarmonicMajor, DoubleHarmonic
    };
    
    struct Chord {
        int root;                       // 0-11 (C=0, C#=1, D=2, etc)
        ChordQuality quality;
        std::vector<int> notes;         // MIDI notes relativos a root
        juce::String symbol;            // "Cmaj7", "Dm7", etc
    };
    
    struct Progression {
        juce::String name;
        juce::String genre;
        std::vector<Chord> chords;
        std::vector<int> degrees;       // I, ii, iii, IV, V, vi, viio
        int beatsPerChord = 4;
    };
    
    //==========================================================================
    ChordProgressionSystem();
    ~ChordProgressionSystem() = default;
    
    // Get progressions
    const Progression& getProgression(int index) const;
    int getNumProgressions() const { return static_cast<int>(progressions_.size()); }
    
    // Filter by genre
    std::vector<Progression> getProgressionsByGenre(const juce::String& genre) const;
    juce::StringArray getAvailableGenres() const;
    
    // Create chords
    Chord createChord(int root, ChordQuality quality) const;
    Chord createChordFromDegree(int degree, Scale scale, int rootNote) const;
    
    // Analyze
    juce::String analyzeChord(const std::vector<int>& midiNotes) const;
    std::vector<int> getScaleDegrees(const std::vector<int>& midiNotes, Scale scale) const;
    
    // Generate
    Progression generateProgression(Scale scale, int rootNote, int numChords);
    
    // Apply to MIDI
    juce::MidiBuffer applyProgression(const Progression& prog, int rootNote, double tempo) const;
    
    // Popular progressions
    std::vector<Progression> getPopularProgressions() const;
    
private:
    //==========================================================================
    std::vector<Progression> progressions_;
    
    void initializeProgressions();
    void addProgression(const juce::String& name, const juce::String& genre, 
                       const std::vector<int>& degrees, int beatsPerChord = 4);
    
    // Theory helpers
    std::vector<int> getScaleNotes(Scale scale, int root) const;
    ChordQuality getChordQualityFromDegree(int degree, Scale scale) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordProgressionSystem)
};

//==============================================================================
/** Scale Detector - Detección automática de escalas */
class ScaleDetector {
public:
    //==========================================================================
    struct ScaleMatch {
        ChordProgressionSystem::Scale scale;
        int root;                       // 0-11
        float confidence;               // 0.0-1.0
        juce::String name;              // "C Major", "A Minor", etc
        
        std::vector<int> matchingNotes; // Notas que coinciden
        std::vector<int> extraNotes;    // Notas fuera de escala
    };
    
    //==========================================================================
    ScaleDetector();
    ~ScaleDetector() = default;
    
    // Analyze MIDI notes
    std::vector<ScaleMatch> detectScales(const std::vector<int>& midiNotes) const;
    ScaleMatch getBestMatch(const std::vector<int>& midiNotes) const;
    
    // Analyze MIDI buffer
    std::vector<ScaleMatch> detectScalesFromMIDI(const juce::MidiBuffer& midiBuffer) const;
    
    // Get scale info
    std::vector<int> getScaleNotes(ChordProgressionSystem::Scale scale, int root) const;
    juce::String getScaleName(ChordProgressionSystem::Scale scale, int root) const;
    
    // Suggestions
    std::vector<int> suggestNextNotes(const std::vector<int>& currentNotes, 
                                      ChordProgressionSystem::Scale scale,
                                      int root) const;
    
private:
    //==========================================================================
    float calculateScaleConfidence(const std::vector<int>& notes, 
                                   const std::vector<int>& scaleNotes) const;
    
    std::vector<int> normalizePitches(const std::vector<int>& midiNotes) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleDetector)
};

//==============================================================================
/** Tempo Map - Cambios de tempo en timeline */
class TempoMap {
public:
    //==========================================================================
    struct TempoChange {
        double position;                // En beats
        double bpm;
        juce::String label;
        
        enum class Curve { Linear, Exponential, Logarithmic, Instant };
        Curve curve = Curve::Instant;
        double curveDuration = 0.0;     // En beats (para gradual changes)
    };
    
    //==========================================================================
    TempoMap();
    ~TempoMap() = default;
    
    // Tempo changes
    void addTempoChange(double position, double bpm, const juce::String& label = "");
    void removeTempoChange(int index);
    void clearTempoChanges();
    
    int getNumTempoChanges() const { return static_cast<int>(tempoChanges_.size()); }
    const TempoChange& getTempoChange(int index) const { return tempoChanges_[index]; }
    
    // Get tempo at position
    double getTempoAtPosition(double beats) const;
    double getTempoAtTime(double seconds) const;
    
    // Convert between time and beats
    double beatsToSeconds(double beats) const;
    double secondsToBeats(double seconds) const;
    
    // Get tempo curve
    std::vector<juce::Point<double>> getTempoCurve(double startBeats, double endBeats, int numPoints) const;
    
    // Global tempo
    void setGlobalTempo(double bpm);
    double getGlobalTempo() const { return globalTempo_; }
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::vector<TempoChange> tempoChanges_;
    double globalTempo_ = 120.0;
    
    void sortTempoChanges();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMap)
};

//==============================================================================
/** Time Signature Changes - Cambios de compás */
class TimeSignatureMap {
public:
    //==========================================================================
    struct TimeSignatureChange {
        double position;                // En beats
        int numerator;
        int denominator;
        juce::String label;
    };
    
    //==========================================================================
    TimeSignatureMap();
    ~TimeSignatureMap() = default;
    
    // Time signature changes
    void addTimeSignatureChange(double position, int numerator, int denominator, 
                                const juce::String& label = "");
    void removeTimeSignatureChange(int index);
    void clearTimeSignatureChanges();
    
    int getNumTimeSignatureChanges() const { return static_cast<int>(timeSignatureChanges_.size()); }
    const TimeSignatureChange& getTimeSignatureChange(int index) const { return timeSignatureChanges_[index]; }
    
    // Get time signature at position
    void getTimeSignatureAtPosition(double beats, int& numerator, int& denominator) const;
    
    // Bar calculations
    int getBarNumberAtPosition(double beats) const;
    double getBarStartPosition(int barNumber) const;
    
    // Global time signature
    void setGlobalTimeSignature(int numerator, int denominator);
    void getGlobalTimeSignature(int& numerator, int& denominator) const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::vector<TimeSignatureChange> timeSignatureChanges_;
    int globalNumerator_ = 4;
    int globalDenominator_ = 4;
    
    void sortTimeSignatureChanges();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeSignatureMap)
};

//==============================================================================
/** Arranger View - Vista de arreglo con secciones */
class ArrangerView : public juce::Component {
public:
    //==========================================================================
    struct Section {
        juce::String name;              // "Intro", "Verse", "Chorus", etc
        double startPosition;           // En beats
        double endPosition;             // En beats
        juce::Colour color;
        
        bool loop = false;
        int loopCount = 1;
    };
    
    //==========================================================================
    ArrangerView();
    ~ArrangerView() override = default;
    
    // Sections
    int addSection(const Section& section);
    void removeSection(int sectionIndex);
    void clearSections();
    
    int getNumSections() const { return static_cast<int>(sections_.size()); }
    const Section& getSection(int index) const { return sections_[index]; }
    
    // Get section at position
    int getSectionAtPosition(double beats) const;
    
    // Reorder sections
    void moveSection(int fromIndex, int toIndex);
    
    // Presets (song structures típicas)
    void applyPreset(const juce::String& presetName);
    juce::StringArray getAvailablePresets() const;
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
private:
    //==========================================================================
    std::vector<Section> sections_;
    int selectedSection_ = -1;
    
    void drawSection(juce::Graphics& g, const Section& section, juce::Rectangle<float> bounds);
    int getSectionAtPoint(juce::Point<int> point) const;
    
    void initializePresets();
    std::map<juce::String, std::vector<Section>> presets_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerView)
};

//==============================================================================
/** Harmony Generator - Generación automática de armonías */
class HarmonyGenerator {
public:
    //==========================================================================
    enum class HarmonyType {
        Third,          // Terceras paralelas
        Fifth,          // Quintas paralelas
        Octave,         // Octavas
        Fourth,         // Cuartas
        Sixth,          // Sextas
        
        Diatonic,       // Siguiendo la escala
        Chromatic,      // Cromático
        
        Bach,           // Estilo Bach (contrapunto)
        Barbershop,     // Barbershop quartet
        Gospel,         // Gospel/Soul
        Jazz            // Jazz voicings
    };
    
    //==========================================================================
    HarmonyGenerator();
    ~HarmonyGenerator() = default;
    
    // Generate harmony
    juce::MidiBuffer generateHarmony(const juce::MidiBuffer& melody, 
                                      HarmonyType type,
                                      ChordProgressionSystem::Scale scale,
                                      int rootNote,
                                      int numVoices = 2) const;
    
    // Custom intervals
    juce::MidiBuffer generateCustomHarmony(const juce::MidiBuffer& melody,
                                           const std::vector<int>& intervals) const;
    
    // Humanize
    void humanizeHarmony(juce::MidiBuffer& harmony, 
                        int timingVariation,    // En ticks
                        int velocityVariation) const;
    
private:
    //==========================================================================
    std::vector<int> getHarmonyNotes(int melodyNote, HarmonyType type,
                                     const std::vector<int>& scaleNotes,
                                     int numVoices) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonyGenerator)
};

} // namespace OmegaStudio
