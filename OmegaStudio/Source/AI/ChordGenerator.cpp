//==============================================================================
// ChordGenerator.cpp - Intelligent chord progression generator
//==============================================================================

#include "ChordGenerator.h"

namespace OmegaStudio::AI {

ChordGenerator::ChordGenerator() {
    initializeScales();
    initializeChordProgressions();
}

void ChordGenerator::initializeScales() {
    // Major scale intervals
    scales_["Major"] = {0, 2, 4, 5, 7, 9, 11};
    scales_["Minor"] = {0, 2, 3, 5, 7, 8, 10};
    scales_["Dorian"] = {0, 2, 3, 5, 7, 9, 10};
    scales_["Phrygian"] = {0, 1, 3, 5, 7, 8, 10};
    scales_["Lydian"] = {0, 2, 4, 6, 7, 9, 11};
    scales_["Mixolydian"] = {0, 2, 4, 5, 7, 9, 10};
    scales_["Aeolian"] = {0, 2, 3, 5, 7, 8, 10};
    scales_["Locrian"] = {0, 1, 3, 5, 6, 8, 10};
}

void ChordGenerator::initializeChordProgressions() {
    // Common progressions (Roman numerals as degree indices)
    progressions_["Pop"] = {{0, 3, 5, 3}, {0, 5, 3, 4}};  // I-IV-V-IV, I-V-vi-IV
    progressions_["Jazz"] = {{1, 4, 0, 3}, {0, 2, 1, 4}}; // ii-V-I-IV
    progressions_["Rock"] = {{0, 5, 3, 4}, {0, 4, 5, 4}}; // I-vi-IV-V
}

juce::MidiBuffer ChordGenerator::generateProgression(
    const juce::String& key,
    const juce::String& scale,
    const juce::String& style,
    int numBars,
    double tempo)
{
    juce::MidiBuffer buffer;
    
    auto scaleIntervals = scales_[scale];
    int rootNote = 60; // Middle C
    
    // Get progression pattern
    std::vector<int> pattern = {0, 3, 5, 3}; // Default I-IV-V-IV
    if (progressions_.count(style) > 0 && !progressions_[style].empty()) {
        pattern = progressions_[style][0];
    }
    
    double beatsPerBar = 4.0;
    double secondsPerBeat = 60.0 / tempo;
    double samplesPerBeat = secondsPerBeat * 44100.0;
    
    int samplePos = 0;
    
    for (int bar = 0; bar < numBars; ++bar) {
        int degree = pattern[bar % pattern.size()];
        auto chord = generateChord(rootNote, scaleIntervals, degree, ChordType::Triad);
        
        // Add chord notes
        for (auto note : chord) {
            buffer.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8)100), samplePos);
            buffer.addEvent(juce::MidiMessage::noteOff(1, note), samplePos + (int)(samplesPerBeat * beatsPerBar));
        }
        
        samplePos += (int)(samplesPerBeat * beatsPerBar);
    }
    
    return buffer;
}

std::vector<int> ChordGenerator::generateChord(
    int rootNote,
    const std::vector<int>& scale,
    int degree,
    ChordType type)
{
    std::vector<int> chord;
    
    if (scale.empty()) return chord;
    
    int root = rootNote + scale[degree % scale.size()];
    chord.push_back(root);
    
    // Add third (2 scale degrees up)
    if (scale.size() > 2) {
        int third = rootNote + scale[(degree + 2) % scale.size()];
        chord.push_back(third);
    }
    
    // Add fifth (4 scale degrees up)
    if (scale.size() > 4) {
        int fifth = rootNote + scale[(degree + 4) % scale.size()];
        chord.push_back(fifth);
    }
    
    // Add seventh for 7th chords
    if (type == ChordType::Seventh && scale.size() > 6) {
        int seventh = rootNote + scale[(degree + 6) % scale.size()];
        chord.push_back(seventh);
    }
    
    return chord;
}

juce::StringArray ChordGenerator::getAvailableScales() const {
    juce::StringArray result;
    for (const auto& pair : scales_) {
        result.add(pair.first);
    }
    return result;
}

juce::StringArray ChordGenerator::getAvailableStyles() const {
    juce::StringArray result;
    for (const auto& pair : progressions_) {
        result.add(pair.first);
    }
    return result;
}

} // namespace OmegaStudio::AI
