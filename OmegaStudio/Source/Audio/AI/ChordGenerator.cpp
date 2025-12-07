#include "ChordGenerator.h"
#include <map>
#include <random>

namespace omega {
namespace AI {

ChordGenerator::ChordGenerator() {
}

std::vector<int> ChordGenerator::getScaleIntervals(const juce::String& scale) {
    static const std::map<juce::String, std::vector<int>> scales = {
        {"Major", {0, 2, 4, 5, 7, 9, 11}},
        {"Minor", {0, 2, 3, 5, 7, 8, 10}},
        {"Dorian", {0, 2, 3, 5, 7, 9, 10}},
        {"Phrygian", {0, 1, 3, 5, 7, 8, 10}},
        {"Lydian", {0, 2, 4, 6, 7, 9, 11}},
        {"Mixolydian", {0, 2, 4, 5, 7, 9, 10}},
        {"Aeolian", {0, 2, 3, 5, 7, 8, 10}},
        {"Locrian", {0, 1, 3, 5, 6, 8, 10}},
        {"Harmonic Minor", {0, 2, 3, 5, 7, 8, 11}},
        {"Melodic Minor", {0, 2, 3, 5, 7, 9, 11}}
    };
    
    auto it = scales.find(scale);
    if (it != scales.end())
        return it->second;
    
    return scales.at("Major");
}

std::vector<juce::String> ChordGenerator::getCommonProgressions(ChordMode mode) {
    switch (mode) {
        case ChordMode::Accompaniment:
            return {"I", "V", "vi", "IV", "I", "IV", "V", "I"};
        case ChordMode::Lead:
            return {"I", "iii", "vi", "ii", "V", "I"};
        case ChordMode::Pads:
            return {"I", "vi", "IV", "V"};
        default:
            return {"I", "V", "vi", "IV"};
    }
}

int ChordGenerator::noteToMidi(const juce::String& note, int octave) {
    static const std::map<juce::String, int> noteMap = {
        {"C", 0}, {"C#", 1}, {"Db", 1},
        {"D", 2}, {"D#", 3}, {"Eb", 3},
        {"E", 4},
        {"F", 5}, {"F#", 6}, {"Gb", 6},
        {"G", 7}, {"G#", 8}, {"Ab", 8},
        {"A", 9}, {"A#", 10}, {"Bb", 10},
        {"B", 11}
    };
    
    auto it = noteMap.find(note);
    if (it != noteMap.end())
        return (octave + 1) * 12 + it->second;
    
    return 60; // Middle C
}

std::vector<GeneratedChord> ChordGenerator::generate(ChordMode mode, int numChords) {
    std::vector<GeneratedChord> result;
    
    auto progression = getCommonProgressions(mode);
    auto scale = getScaleIntervals(config_.scale);
    int rootNote = noteToMidi(config_.key, config_.octave);
    
    // Roman numeral to scale degree
    std::map<juce::String, int> romanToScaleDegree = {
        {"I", 0}, {"II", 1}, {"III", 2}, {"IV", 3}, 
        {"V", 4}, {"VI", 5}, {"VII", 6},
        {"i", 0}, {"ii", 1}, {"iii", 2}, {"iv", 3},
        {"v", 4}, {"vi", 5}, {"vii", 6}
    };
    
    std::vector<int> previousVoicing;
    
    for (int i = 0; i < numChords && i < (int)progression.size(); ++i) {
        GeneratedChord chord;
        chord.startBeat = i * 4.0f;
        chord.lengthBeats = 4.0f;
        chord.velocity = 80 + (mode == ChordMode::Pads ? 20 : 0);
        chord.chordName = progression[i];
        
        // Get scale degree
        int degree = romanToScaleDegree[progression[i]];
        int chordRoot = rootNote + scale[degree];
        
        // Build triad or 7th chord based on density
        std::vector<int> notes;
        notes.push_back(chordRoot);
        notes.push_back(chordRoot + scale[(degree + 2) % scale.size()]);
        notes.push_back(chordRoot + scale[(degree + 4) % scale.size()]);
        
        if (config_.density >= 2) {
            notes.push_back(chordRoot + 12 + scale[(degree + 6) % scale.size()]);
        }
        
        if (config_.density >= 3) {
            notes.push_back(chordRoot + 12 + scale[(degree + 1) % scale.size()]);
        }
        
        // Apply voicing
        if (config_.voiceLeading && !previousVoicing.empty()) {
            notes = applyVoiceLeading(previousVoicing, notes);
        }
        
        if (config_.drop2 || config_.drop4) {
            notes = applyDropVoicing(notes);
        }
        
        chord.midiNotes = notes;
        previousVoicing = notes;
        result.push_back(chord);
    }
    
    return result;
}

GeneratedChord ChordGenerator::generateChord(const juce::String& chordName, 
                                            float startBeat, 
                                            float lengthBeats) {
    GeneratedChord chord;
    chord.chordName = chordName;
    chord.startBeat = startBeat;
    chord.lengthBeats = lengthBeats;
    chord.velocity = 100;
    
    // Simple chord generation (can be expanded)
    int rootNote = noteToMidi(config_.key, config_.octave);
    chord.midiNotes = {rootNote, rootNote + 4, rootNote + 7}; // Major triad
    
    return chord;
}

std::vector<int> ChordGenerator::applyVoiceLeading(const std::vector<int>& previousVoicing,
                                                  const std::vector<int>& targetNotes) {
    // Minimize total movement between voicings
    std::vector<int> result = targetNotes;
    
    for (size_t i = 0; i < result.size() && i < previousVoicing.size(); ++i) {
        int diff = result[i] - previousVoicing[i];
        
        // Move to nearest octave
        if (diff > 6) {
            result[i] -= 12;
        } else if (diff < -6) {
            result[i] += 12;
        }
    }
    
    return result;
}

std::vector<int> ChordGenerator::applyDropVoicing(const std::vector<int>& notes) {
    if (notes.size() < 4) return notes;
    
    std::vector<int> result = notes;
    
    if (config_.drop2) {
        // Drop second highest note by an octave
        result[result.size() - 2] -= 12;
    }
    
    if (config_.drop4) {
        // Drop fourth highest note by an octave
        if (result.size() >= 4)
            result[result.size() - 4] -= 12;
    }
    
    return result;
}

juce::MidiBuffer ChordGenerator::createMidiClip(const std::vector<GeneratedChord>& chords,
                                               double sampleRate,
                                               double bpm) {
    juce::MidiBuffer midiBuffer;
    double samplesPerBeat = (sampleRate * 60.0) / bpm;
    
    for (const auto& chord : chords) {
        int startSample = static_cast<int>(chord.startBeat * samplesPerBeat);
        int endSample = static_cast<int>((chord.startBeat + chord.lengthBeats) * samplesPerBeat);
        
        // Add note-on messages
        for (int note : chord.midiNotes) {
            midiBuffer.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)chord.velocity), 
                              startSample);
        }
        
        // Add note-off messages
        for (int note : chord.midiNotes) {
            midiBuffer.addEvent(juce::MidiMessage::noteOff(1, note), endSample);
        }
    }
    
    return midiBuffer;
}

} // namespace AI
} // namespace omega
