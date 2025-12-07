#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio {
namespace AI {

/**
 * @brief Loop Starter - AI Genre-Based Loop Generator (FL Studio 2025 Feature)
 * Instantly generate and arrange genre-based loops
 */
class LoopStarter
{
public:
    enum class Genre {
        HipHop,
        Trap,
        EDM,
        House,
        Techno,
        Dubstep,
        DrumAndBass,
        LoFi,
        Pop,
        Rock,
        Jazz,
        Reggaeton,
        Afrobeat,
        Ambient,
        Custom
    };

    enum class Complexity {
        Simple,      // 4-8 elements
        Medium,      // 8-16 elements
        Complex,     // 16-32 elements
        Chaotic      // 32+ elements
    };

    struct LoopPattern {
        juce::String name;
        Genre genre;
        int bpm;
        int bars = 4;
        juce::String timeSignature = "4/4";
        
        // Pattern layers
        juce::MidiBuffer drums;
        juce::MidiBuffer bass;
        juce::MidiBuffer chords;
        juce::MidiBuffer melody;
        juce::MidiBuffer fx;
        
        // Arrangement
        std::vector<int> structure; // Bar arrangement [0,1,2,3]
        juce::String key = "C";
        juce::String scale = "minor";
    };

    struct GenerationSettings {
        Genre genre = Genre::HipHop;
        Complexity complexity = Complexity::Medium;
        int bpm = 120;
        int bars = 4;
        juce::String key = "C";
        juce::String scale = "minor";
        float swing = 0.0f;      // 0-100%
        float humanize = 0.0f;   // 0-100%
        bool includeDrums = true;
        bool includeBass = true;
        bool includeChords = true;
        bool includeMelody = true;
        bool includeFX = false;
    };

    LoopStarter();
    ~LoopStarter();

    // Generate loops
    LoopPattern generateLoop(const GenerationSettings& settings);
    std::vector<LoopPattern> generateVariations(const LoopPattern& base, int count = 4);
    
    // Genre templates
    static std::vector<Genre> getAvailableGenres();
    static juce::String getGenreName(Genre genre);
    static GenerationSettings getGenreTemplate(Genre genre);
    
    // BPM suggestions
    static juce::Range<int> getGenreBPMRange(Genre genre);
    static int suggestBPM(Genre genre);
    
    // Pattern analysis
    float analyzeGroove(const juce::MidiBuffer& pattern);
    float analyzeDensity(const juce::MidiBuffer& pattern);
    juce::String detectGenre(const LoopPattern& pattern);
    
    // Pattern operations
    void quantizePattern(juce::MidiBuffer& pattern, float strength = 1.0f);
    void humanizePattern(juce::MidiBuffer& pattern, float amount = 0.5f);
    void applySwing(juce::MidiBuffer& pattern, float amount = 0.5f);
    void transpose(juce::MidiBuffer& pattern, int semitones);
    
    // Export
    bool exportToMidi(const LoopPattern& pattern, const juce::File& file);
    juce::AudioBuffer<float> renderToAudio(const LoopPattern& pattern, double sampleRate);

private:
    struct GenreCharacteristics {
        juce::Range<int> bpmRange;
        std::vector<int> commonKicks;      // MIDI notes
        std::vector<int> commonSnares;
        std::vector<int> commonHats;
        std::vector<float> kickPattern;    // Probability per 16th note
        std::vector<float> snarePattern;
        std::vector<float> hatPattern;
        float swingAmount;
        float velocityVariation;
    };

    std::map<Genre, GenreCharacteristics> genreDatabase;
    juce::Random random;

    void initializeGenreDatabase();
    
    // Pattern generators
    juce::MidiBuffer generateDrumPattern(const GenerationSettings& settings);
    juce::MidiBuffer generateBassPattern(const GenerationSettings& settings);
    juce::MidiBuffer generateChordPattern(const GenerationSettings& settings);
    juce::MidiBuffer generateMelodyPattern(const GenerationSettings& settings);
    juce::MidiBuffer generateFXPattern(const GenerationSettings& settings);
    
    // Music theory helpers
    std::vector<int> getScaleNotes(const juce::String& key, const juce::String& scale);
    std::vector<std::vector<int>> generateChordProgression(const juce::String& key, 
                                                           const juce::String& scale,
                                                           int bars);
    int getNoteFromScale(int degree, const std::vector<int>& scale);
    
    // Rhythm helpers
    void addNote(juce::MidiBuffer& buffer, int note, float time, float duration, uint8 velocity);
    float getSwingOffset(float position, float swingAmount);
    uint8 humanizeVelocity(uint8 baseVelocity, float humanizeAmount);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopStarter)
};

} // namespace AI
} // namespace OmegaStudio
