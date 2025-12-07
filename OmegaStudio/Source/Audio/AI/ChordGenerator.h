#pragma once

#include <JuceHeader.h>
#include <vector>
#include <functional>

namespace omega {
namespace AI {

struct ChordGeneratorConfig {
    juce::String key = "C";
    juce::String scale = "Major"; // Major, Minor, Dorian, etc.
    int density = 2; // 0-4: sparse to dense
    bool voiceLeading = true;
    bool drop2 = true;
    bool drop4 = false;
    int octave = 4;
};

enum class ChordMode {
    Accompaniment,  // Background chords
    Lead,          // Melodic lead
    Pads           // Sustained pads
};

struct GeneratedChord {
    std::vector<int> midiNotes;
    juce::String chordName;
    float startBeat = 0.0f;
    float lengthBeats = 4.0f;
    int velocity = 100;
};

class ChordGenerator {
public:
    ChordGenerator();
    
    void setConfig(const ChordGeneratorConfig& config) { config_ = config; }
    
    // Generate chord progression
    std::vector<GeneratedChord> generate(ChordMode mode, int numChords = 4);
    
    // Generate specific chord with voicing
    GeneratedChord generateChord(const juce::String& chordName, 
                                 float startBeat, 
                                 float lengthBeats);
    
    // Create MIDI clip from chords
    juce::MidiBuffer createMidiClip(const std::vector<GeneratedChord>& chords, 
                                   double sampleRate, 
                                   double bpm);
    
private:
    ChordGeneratorConfig config_;
    
    // Scales database
    std::vector<int> getScaleIntervals(const juce::String& scale);
    
    // Chord progressions by key
    std::vector<juce::String> getCommonProgressions(ChordMode mode);
    
    // Voice leading algorithm
    std::vector<int> applyVoiceLeading(const std::vector<int>& previousVoicing,
                                      const std::vector<int>& targetNotes);
    
    // Drop-2 and Drop-4 voicings
    std::vector<int> applyDropVoicing(const std::vector<int>& notes);
    
    int noteToMidi(const juce::String& note, int octave);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordGenerator)
};

} // namespace AI
} // namespace omega
