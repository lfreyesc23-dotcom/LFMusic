#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio::AI {

/**
 * @brief Chord Generator - Intelligent Chord Progressions
 * Generates musically correct chord progressions for various styles
 */
class ChordGenerator {
public:
    enum class ChordType {
        Triad,
        Seventh,
        Extended
    };

    ChordGenerator();

    // Generate chord progression
    juce::MidiBuffer generateProgression(
        const juce::String& key,
        const juce::String& scale,
        const juce::String& style,
        int numBars,
        double tempo);

    // Generate single chord
    std::vector<int> generateChord(
        int rootNote,
        const std::vector<int>& scale,
        int degree,
        ChordType type);

    // Available options
    juce::StringArray getAvailableScales() const;
    juce::StringArray getAvailableStyles() const;

private:
    void initializeScales();
    void initializeChordProgressions();

    std::map<juce::String, std::vector<int>> scales_;
    std::map<juce::String, std::vector<std::vector<int>>> progressions_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordGenerator)
};

} // namespace OmegaStudio::AI
