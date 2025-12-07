//==============================================================================
// LoopStarter.cpp - AI-powered loop generation (FL Studio 2025 Feature)
//==============================================================================

#include "LoopStarter.h"

namespace OmegaStudio::AI {

LoopStarter::LoopStarter() {
    initializeGenreDatabase();
}

LoopStarter::~LoopStarter() = default;

void LoopStarter::initializeGenreDatabase() {
    // Initialize basic genre characteristics
    // Full implementation can be added progressively
}

LoopStarter::LoopPattern LoopStarter::generateLoop(const GenerationSettings& settings) {
    LoopPattern pattern;
    pattern.genre = settings.genre;
    pattern.bpm = settings.bpm;
    pattern.bars = settings.bars;
    pattern.key = settings.key;
    pattern.scale = settings.scale;
    pattern.name = getGenreName(settings.genre) + " Loop";
    
    if (settings.includeDrums) {
        pattern.drums = generateDrumPattern(settings);
    }
    if (settings.includeBass) {
        pattern.bass = generateBassPattern(settings);
    }
    if (settings.includeChords) {
        pattern.chords = generateChordPattern(settings);
    }
    if (settings.includeMelody) {
        pattern.melody = generateMelodyPattern(settings);
    }
    if (settings.includeFX) {
        pattern.fx = generateFXPattern(settings);
    }
    
    return pattern;
}

std::vector<LoopStarter::LoopPattern> LoopStarter::generateVariations(const LoopPattern& base, int count) {
    std::vector<LoopPattern> variations;
    variations.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        variations.push_back(base);  // Simple copy for now
    }
    
    return variations;
}

std::vector<LoopStarter::Genre> LoopStarter::getAvailableGenres() {
    return {
        Genre::HipHop, Genre::Trap, Genre::EDM, Genre::House,
        Genre::Techno, Genre::Dubstep, Genre::DrumAndBass,
        Genre::LoFi, Genre::Pop, Genre::Rock, Genre::Jazz,
        Genre::Reggaeton, Genre::Afrobeat, Genre::Ambient
    };
}

juce::String LoopStarter::getGenreName(Genre genre) {
    switch (genre) {
        case Genre::HipHop: return "Hip Hop";
        case Genre::Trap: return "Trap";
        case Genre::EDM: return "EDM";
        case Genre::House: return "House";
        case Genre::Techno: return "Techno";
        case Genre::Dubstep: return "Dubstep";
        case Genre::DrumAndBass: return "Drum & Bass";
        case Genre::LoFi: return "Lo-Fi";
        case Genre::Pop: return "Pop";
        case Genre::Rock: return "Rock";
        case Genre::Jazz: return "Jazz";
        case Genre::Reggaeton: return "Reggaeton";
        case Genre::Afrobeat: return "Afrobeat";
        case Genre::Ambient: return "Ambient";
        case Genre::Custom: return "Custom";
        default: return "Unknown";
    }
}

LoopStarter::GenerationSettings LoopStarter::getGenreTemplate(Genre genre) {
    GenerationSettings settings;
    settings.genre = genre;
    settings.bpm = suggestBPM(genre);
    return settings;
}

juce::Range<int> LoopStarter::getGenreBPMRange(Genre genre) {
    switch (genre) {
        case Genre::HipHop: return {85, 95};
        case Genre::Trap: return {140, 170};
        case Genre::EDM: return {128, 132};
        case Genre::House: return {120, 130};
        case Genre::Techno: return {125, 135};
        case Genre::Dubstep: return {138, 145};
        case Genre::DrumAndBass: return {170, 180};
        case Genre::LoFi: return {70, 90};
        case Genre::Pop: return {110, 125};
        case Genre::Rock: return {110, 140};
        case Genre::Jazz: return {120, 180};
        case Genre::Reggaeton: return {90, 105};
        case Genre::Afrobeat: return {105, 125};
        case Genre::Ambient: return {80, 110};
        default: return {120, 120};
    }
}

int LoopStarter::suggestBPM(Genre genre) {
    auto range = getGenreBPMRange(genre);
    return (range.getStart() + range.getEnd()) / 2;
}

float LoopStarter::analyzeGroove(const juce::MidiBuffer& pattern) {
    return 0.5f;  // Stub
}

float LoopStarter::analyzeDensity(const juce::MidiBuffer& pattern) {
    int noteCount = 0;
    for (const auto metadata : pattern) {
        if (metadata.getMessage().isNoteOn()) noteCount++;
    }
    return juce::jmin(1.0f, noteCount / 32.0f);
}

juce::String LoopStarter::detectGenre(const LoopPattern& pattern) {
    return getGenreName(pattern.genre);
}

void LoopStarter::quantizePattern(juce::MidiBuffer& pattern, float strength) {
    // Stub implementation
}

void LoopStarter::humanizePattern(juce::MidiBuffer& pattern, float amount) {
    // Stub implementation
}

void LoopStarter::applySwing(juce::MidiBuffer& pattern, float amount) {
    // Stub implementation
}

void LoopStarter::transpose(juce::MidiBuffer& pattern, int semitones) {
    juce::MidiBuffer transposed;
    
    for (const auto metadata : pattern) {
        auto message = metadata.getMessage();
        if (message.isNoteOnOrOff()) {
            int newNote = juce::jlimit(0, 127, message.getNoteNumber() + semitones);
            if (message.isNoteOn()) {
                message = juce::MidiMessage::noteOn(message.getChannel(), newNote, message.getVelocity());
            } else {
                message = juce::MidiMessage::noteOff(message.getChannel(), newNote);
            }
        }
        transposed.addEvent(message, metadata.samplePosition);
    }
    
    pattern = transposed;
}

bool LoopStarter::exportToMidi(const LoopPattern& pattern, const juce::File& file) {
    // Stub - would need JUCE MidiFile implementation
    return false;
}

juce::AudioBuffer<float> LoopStarter::renderToAudio(const LoopPattern& pattern, double sampleRate) {
    // Stub - would need synth rendering
    return juce::AudioBuffer<float>();
}

juce::MidiBuffer LoopStarter::generateDrumPattern(const GenerationSettings& settings) {
    juce::MidiBuffer drums;
    
    // Simple 4-on-the-floor kick pattern
    int ppq = 480;
    int ticksPerBar = ppq * 4;
    
    for (int bar = 0; bar < settings.bars; ++bar) {
        int barStart = bar * ticksPerBar;
        
        // Kick on every beat
        for (int beat = 0; beat < 4; ++beat) {
            addNote(drums, 36, barStart + beat * ppq, ppq / 4, 100);
        }
        
        // Snare on 2 and 4
        addNote(drums, 38, barStart + ppq, ppq / 4, 95);
        addNote(drums, 38, barStart + 3 * ppq, ppq / 4, 95);
        
        // Hi-hats on 8ths
        for (int i = 0; i < 8; ++i) {
            addNote(drums, 42, barStart + i * (ppq / 2), ppq / 8, 80);
        }
    }
    
    return drums;
}

juce::MidiBuffer LoopStarter::generateBassPattern(const GenerationSettings& settings) {
    juce::MidiBuffer bass;
    
    int ppq = 480;
    int ticksPerBar = ppq * 4;
    int rootNote = 36;  // C1
    
    for (int bar = 0; bar < settings.bars; ++bar) {
        int barStart = bar * ticksPerBar;
        addNote(bass, rootNote, barStart, ppq, 90);
        addNote(bass, rootNote, barStart + 2 * ppq, ppq, 85);
    }
    
    return bass;
}

juce::MidiBuffer LoopStarter::generateChordPattern(const GenerationSettings& settings) {
    juce::MidiBuffer chords;
    
    int ppq = 480;
    int ticksPerBar = ppq * 4;
    int rootNote = 60;  // C4
    
    for (int bar = 0; bar < settings.bars; ++bar) {
        int barStart = bar * ticksPerBar;
        
        // Major triad
        addNote(chords, rootNote, barStart, ticksPerBar, 70);
        addNote(chords, rootNote + 4, barStart, ticksPerBar, 70);
        addNote(chords, rootNote + 7, barStart, ticksPerBar, 70);
    }
    
    return chords;
}

juce::MidiBuffer LoopStarter::generateMelodyPattern(const GenerationSettings& settings) {
    juce::MidiBuffer melody;
    
    int ppq = 480;
    int ticksPerBeat = ppq;
    int rootNote = 72;  // C5
    
    for (int bar = 0; bar < settings.bars; ++bar) {
        int barStart = bar * ppq * 4;
        
        for (int i = 0; i < 4; ++i) {
            int note = rootNote + (i * 2);  // Simple ascending pattern
            addNote(melody, note, barStart + i * ticksPerBeat, ticksPerBeat / 2, 80);
        }
    }
    
    return melody;
}

juce::MidiBuffer LoopStarter::generateFXPattern(const GenerationSettings& settings) {
    return juce::MidiBuffer();  // Stub
}

std::vector<int> LoopStarter::getScaleNotes(const juce::String& key, const juce::String& scale) {
    // Return C major scale for now
    return {0, 2, 4, 5, 7, 9, 11};
}

std::vector<std::vector<int>> LoopStarter::generateChordProgression(
    const juce::String& key,
    const juce::String& scale,
    int bars)
{
    std::vector<std::vector<int>> progression;
    progression.reserve(bars);
    
    // Simple I-IV-V-I progression
    for (int i = 0; i < bars; ++i) {
        progression.push_back({60, 64, 67});  // C major triad
    }
    
    return progression;
}

int LoopStarter::getNoteFromScale(int degree, const std::vector<int>& scale) {
    if (scale.empty()) return 60;
    return 60 + scale[degree % scale.size()];
}

void LoopStarter::addNote(juce::MidiBuffer& buffer, int note, float time, float duration, uint8 velocity) {
    buffer.addEvent(juce::MidiMessage::noteOn(1, note, velocity), (int)time);
    buffer.addEvent(juce::MidiMessage::noteOff(1, note), (int)(time + duration));
}

float LoopStarter::getSwingOffset(float position, float swingAmount) {
    return 0.0f;  // Stub
}

uint8 LoopStarter::humanizeVelocity(uint8 baseVelocity, float humanizeAmount) {
    int variation = (int)(random.nextFloat() * humanizeAmount * 20.0f);
    int result = baseVelocity + (random.nextBool() ? variation : -variation);
    return (uint8)juce::jlimit(1, 127, result);
}

} // namespace OmegaStudio::AI
