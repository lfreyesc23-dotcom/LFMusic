#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <array>
#include <random>
#include <algorithm>

namespace OmegaStudio {
namespace Sequencer {
namespace MIDIFX {

/**
 * @class Arpeggiator
 * @brief Arpeggiator profesional con múltiples patrones
 */
class Arpeggiator {
public:
    enum class Pattern {
        Up,
        Down,
        UpDown,
        DownUp,
        Random,
        Chord,
        AsPlayed
    };
    
    enum class OctaveMode {
        Single,
        Double,
        Triple,
        Quad
    };
    
    struct Parameters {
        Pattern pattern { Pattern::Up };
        OctaveMode octaveMode { OctaveMode::Single };
        float rate { 0.25f };         // En beats (1/16 = 0.25)
        float gate { 0.8f };          // 0.0 - 2.0
        int swing { 0 };              // 0-100%
        bool latch { false };         // Mantener notas
        int velocityMode { 0 };       // 0=original, 1=fixed, 2=incremental
        uint8_t fixedVelocity { 100 };
    };
    
    void setParameters(const Parameters& params) {
        params_ = params;
    }
    
    void reset() {
        heldNotes_.clear();
        currentStep_ = 0;
        lastOutputTime_ = 0.0;
    }
    
    void noteOn(int noteNumber, uint8_t velocity) {
        heldNotes_[noteNumber] = velocity;
        
        if (!params_.latch) {
            generateArpSequence();
        }
    }
    
    void noteOff(int noteNumber) {
        if (!params_.latch) {
            heldNotes_.erase(noteNumber);
            generateArpSequence();
        }
    }
    
    void allNotesOff() {
        if (!params_.latch) {
            heldNotes_.clear();
            arpSequence_.clear();
        }
    }
    
    void process(juce::MidiBuffer& midiMessages, double currentTime, double tempo) {
        if (heldNotes_.empty()) return;
        
        double beatsPerSecond = tempo / 60.0;
        double stepDuration = params_.rate / beatsPerSecond;
        
        // Check if it's time for next step
        if (currentTime - lastOutputTime_ >= stepDuration) {
            if (!arpSequence_.empty()) {
                int noteIndex = currentStep_ % arpSequence_.size();
                auto [note, velocity] = arpSequence_[noteIndex];
                
                // Apply velocity mode
                uint8_t outputVel = velocity;
                if (params_.velocityMode == 1) {
                    outputVel = params_.fixedVelocity;
                } else if (params_.velocityMode == 2) {
                    outputVel = juce::jlimit(1, 127, 64 + (currentStep_ % 8) * 8);
                }
                
                // Note On
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, note, outputVel), 0);
                
                // Schedule Note Off
                double gateTime = stepDuration * params_.gate;
                int noteOffSample = gateTime * 44100;  // TODO: use actual sample rate
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, note), noteOffSample);
                
                currentStep_++;
            }
            
            lastOutputTime_ = currentTime;
        }
    }
    
private:
    void generateArpSequence() {
        arpSequence_.clear();
        
        if (heldNotes_.empty()) return;
        
        // Convert map to vector
        std::vector<std::pair<int, uint8_t>> notes(heldNotes_.begin(), heldNotes_.end());
        
        // Sort by note number
        std::sort(notes.begin(), notes.end());
        
        // Generate pattern
        switch (params_.pattern) {
            case Pattern::Up:
                arpSequence_ = notes;
                break;
                
            case Pattern::Down:
                arpSequence_ = notes;
                std::reverse(arpSequence_.begin(), arpSequence_.end());
                break;
                
            case Pattern::UpDown:
                arpSequence_ = notes;
                for (int i = notes.size() - 2; i >= 1; --i) {
                    arpSequence_.push_back(notes[i]);
                }
                break;
                
            case Pattern::DownUp:
                arpSequence_ = notes;
                std::reverse(arpSequence_.begin(), arpSequence_.end());
                for (int i = notes.size() - 2; i >= 1; --i) {
                    arpSequence_.push_back(notes[notes.size() - 1 - i]);
                }
                break;
                
            case Pattern::Random:
                arpSequence_ = notes;
                std::shuffle(arpSequence_.begin(), arpSequence_.end(), 
                            std::default_random_engine{});
                break;
                
            case Pattern::Chord:
                arpSequence_ = notes;
                break;
                
            case Pattern::AsPlayed:
                arpSequence_ = notes;
                break;
        }
        
        // Apply octave mode
        if (params_.octaveMode != OctaveMode::Single) {
            std::vector<std::pair<int, uint8_t>> extended;
            int octaves = static_cast<int>(params_.octaveMode);
            
            for (int oct = 0; oct < octaves; ++oct) {
                for (const auto& [note, vel] : arpSequence_) {
                    int transposed = note + oct * 12;
                    if (transposed <= 127) {
                        extended.push_back({transposed, vel});
                    }
                }
            }
            
            arpSequence_ = extended;
        }
    }
    
    Parameters params_;
    std::map<int, uint8_t> heldNotes_;
    std::vector<std::pair<int, uint8_t>> arpSequence_;
    int currentStep_ { 0 };
    double lastOutputTime_ { 0.0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Arpeggiator)
};

/**
 * @class ChordGenerator
 * @brief Genera acordes a partir de notas individuales
 */
class ChordGenerator {
public:
    enum class ChordType {
        Major,
        Minor,
        Diminished,
        Augmented,
        Major7,
        Minor7,
        Dominant7,
        Suspended2,
        Suspended4,
        Power5,
        Major9,
        Minor9
    };
    
    struct Parameters {
        ChordType type { ChordType::Major };
        int inversion { 0 };      // 0=root, 1=1st inv, 2=2nd inv
        int octaveSpread { 0 };   // -2 to +2
        bool voiceLead { true };  // Smart voice leading
        float velocitySpread { 0.1f };  // Randomize velocities slightly
    };
    
    void setParameters(const Parameters& params) {
        params_ = params;
    }
    
    void processNoteOn(int rootNote, uint8_t velocity, juce::MidiBuffer& output) {
        auto intervals = getIntervals(params_.type);
        
        // Apply inversion
        if (params_.inversion > 0) {
            std::rotate(intervals.begin(), 
                       intervals.begin() + std::min(params_.inversion, (int)intervals.size()), 
                       intervals.end());
            // Transpose inverted notes up an octave
            for (int i = 0; i < params_.inversion && i < intervals.size(); ++i) {
                intervals[i] += 12;
            }
        }
        
        // Generate chord notes
        for (int interval : intervals) {
            int note = rootNote + interval + params_.octaveSpread * 12;
            
            if (note >= 0 && note <= 127) {
                // Apply velocity spread
                uint8_t vel = velocity;
                if (params_.velocitySpread > 0.0f) {
                    float random = juce::Random::getSystemRandom().nextFloat() - 0.5f;
                    vel = juce::jlimit(1, 127, 
                        (int)(velocity + random * params_.velocitySpread * 127));
                }
                
                output.addEvent(juce::MidiMessage::noteOn(1, note, vel), 0);
                activeNotes_.insert(note);
            }
        }
    }
    
    void processNoteOff(int rootNote, juce::MidiBuffer& output) {
        // Send note offs for all active notes
        for (int note : activeNotes_) {
            output.addEvent(juce::MidiMessage::noteOff(1, note), 0);
        }
        activeNotes_.clear();
    }
    
    void allNotesOff(juce::MidiBuffer& output) {
        for (int note : activeNotes_) {
            output.addEvent(juce::MidiMessage::noteOff(1, note), 0);
        }
        activeNotes_.clear();
    }
    
private:
    std::vector<int> getIntervals(ChordType type) {
        switch (type) {
            case ChordType::Major:       return {0, 4, 7};
            case ChordType::Minor:       return {0, 3, 7};
            case ChordType::Diminished:  return {0, 3, 6};
            case ChordType::Augmented:   return {0, 4, 8};
            case ChordType::Major7:      return {0, 4, 7, 11};
            case ChordType::Minor7:      return {0, 3, 7, 10};
            case ChordType::Dominant7:   return {0, 4, 7, 10};
            case ChordType::Suspended2:  return {0, 2, 7};
            case ChordType::Suspended4:  return {0, 5, 7};
            case ChordType::Power5:      return {0, 7, 12};
            case ChordType::Major9:      return {0, 4, 7, 11, 14};
            case ChordType::Minor9:      return {0, 3, 7, 10, 14};
            default:                     return {0, 4, 7};
        }
    }
    
    Parameters params_;
    std::set<int> activeNotes_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordGenerator)
};

/**
 * @class ScaleMapper
 * @brief Mapea notas a una escala específica
 */
class ScaleMapper {
public:
    enum class Scale {
        Chromatic,
        Major,
        Minor,
        Dorian,
        Phrygian,
        Lydian,
        Mixolydian,
        Locrian,
        HarmonicMinor,
        MelodicMinor,
        PentatonicMajor,
        PentatonicMinor,
        Blues,
        WholeTone,
        Diminished
    };
    
    void setScale(Scale scale, int rootNote = 60) {
        scale_ = scale;
        rootNote_ = rootNote;
        generateScaleNotes();
    }
    
    int mapToScale(int inputNote) {
        int octave = inputNote / 12;
        int chromaticPitch = inputNote % 12;
        
        // Find closest note in scale
        int closestInterval = 0;
        int minDistance = 12;
        
        for (int interval : scaleIntervals_) {
            int distance = std::abs(chromaticPitch - interval);
            if (distance < minDistance) {
                minDistance = distance;
                closestInterval = interval;
            }
        }
        
        return octave * 12 + closestInterval;
    }
    
    void processMidiBuffer(juce::MidiBuffer& buffer) {
        juce::MidiBuffer mapped;
        
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            
            if (message.isNoteOnOrOff()) {
                int mappedNote = mapToScale(message.getNoteNumber());
                
                if (message.isNoteOn()) {
                    mapped.addEvent(juce::MidiMessage::noteOn(
                        message.getChannel(), mappedNote, message.getVelocity()
                    ), metadata.samplePosition);
                } else {
                    mapped.addEvent(juce::MidiMessage::noteOff(
                        message.getChannel(), mappedNote
                    ), metadata.samplePosition);
                }
            } else {
                mapped.addEvent(message, metadata.samplePosition);
            }
        }
        
        buffer.swapWith(mapped);
    }
    
private:
    void generateScaleNotes() {
        scaleIntervals_ = getScaleIntervals(scale_);
    }
    
    std::vector<int> getScaleIntervals(Scale scale) {
        switch (scale) {
            case Scale::Major:           return {0, 2, 4, 5, 7, 9, 11};
            case Scale::Minor:           return {0, 2, 3, 5, 7, 8, 10};
            case Scale::Dorian:          return {0, 2, 3, 5, 7, 9, 10};
            case Scale::Phrygian:        return {0, 1, 3, 5, 7, 8, 10};
            case Scale::Lydian:          return {0, 2, 4, 6, 7, 9, 11};
            case Scale::Mixolydian:      return {0, 2, 4, 5, 7, 9, 10};
            case Scale::Locrian:         return {0, 1, 3, 5, 6, 8, 10};
            case Scale::HarmonicMinor:   return {0, 2, 3, 5, 7, 8, 11};
            case Scale::MelodicMinor:    return {0, 2, 3, 5, 7, 9, 11};
            case Scale::PentatonicMajor: return {0, 2, 4, 7, 9};
            case Scale::PentatonicMinor: return {0, 3, 5, 7, 10};
            case Scale::Blues:           return {0, 3, 5, 6, 7, 10};
            case Scale::WholeTone:       return {0, 2, 4, 6, 8, 10};
            case Scale::Diminished:      return {0, 2, 3, 5, 6, 8, 9, 11};
            case Scale::Chromatic:
            default:                     return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
        }
    }
    
    Scale scale_ { Scale::Major };
    int rootNote_ { 60 };
    std::vector<int> scaleIntervals_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleMapper)
};

/**
 * @class NoteRepeat
 * @brief Repite notas a una velocidad específica (para finger drumming)
 */
class NoteRepeat {
public:
    struct Parameters {
        bool enabled { false };
        float rate { 0.125f };    // En beats (1/32 = 0.125)
        float gate { 0.7f };
        int velocityDecay { 0 };  // % decay per repeat
    };
    
    void setParameters(const Parameters& params) {
        params_ = params;
    }
    
    void process(juce::MidiBuffer& buffer, double currentTime, double tempo) {
        if (!params_.enabled) return;
        
        // TODO: Implementation
        // Similar to arpeggiator but repeats held notes
    }
    
private:
    Parameters params_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteRepeat)
};

/**
 * @class MIDIRandomizer
 * @brief Randomiza velocidades, timing, pitch
 */
class MIDIRandomizer {
public:
    struct Parameters {
        float velocityAmount { 0.0f };  // 0.0 - 1.0
        float timingAmount { 0.0f };    // In milliseconds
        int pitchRange { 0 };           // Semitones +/-
        float probability { 1.0f };     // 0.0 - 1.0 (nota probability)
    };
    
    void setParameters(const Parameters& params) {
        params_ = params;
    }
    
    void processMidiBuffer(juce::MidiBuffer& buffer, double sampleRate) {
        juce::MidiBuffer randomized;
        auto& rand = juce::Random::getSystemRandom();
        
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            
            // Probability check
            if (message.isNoteOn() && rand.nextFloat() > params_.probability) {
                continue;  // Skip this note
            }
            
            // Randomize velocity
            if (message.isNoteOn() && params_.velocityAmount > 0.0f) {
                int velOffset = (rand.nextFloat() - 0.5f) * params_.velocityAmount * 127;
                uint8_t newVel = juce::jlimit(1, 127, message.getVelocity() + velOffset);
                message = juce::MidiMessage::noteOn(
                    message.getChannel(), message.getNoteNumber(), newVel
                );
            }
            
            // Randomize pitch
            if (message.isNoteOnOrOff() && params_.pitchRange > 0) {
                int pitchOffset = rand.nextInt(juce::Range<int>(-params_.pitchRange, params_.pitchRange + 1));
                int newNote = juce::jlimit(0, 127, message.getNoteNumber() + pitchOffset);
                
                if (message.isNoteOn()) {
                    message = juce::MidiMessage::noteOn(
                        message.getChannel(), newNote, message.getVelocity()
                    );
                } else {
                    message = juce::MidiMessage::noteOff(
                        message.getChannel(), newNote
                    );
                }
            }
            
            // Randomize timing
            int newPosition = metadata.samplePosition;
            if (params_.timingAmount > 0.0f) {
                int sampleOffset = (rand.nextFloat() - 0.5f) * params_.timingAmount * 0.001 * sampleRate;
                newPosition = std::max(0, newPosition + sampleOffset);
            }
            
            randomized.addEvent(message, newPosition);
        }
        
        buffer.swapWith(randomized);
    }
    
private:
    Parameters params_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIRandomizer)
};

/**
 * @class MIDIEcho
 * @brief Echo/delay de MIDI
 */
class MIDIEcho {
public:
    struct Parameters {
        bool enabled { false };
        float delayTime { 0.25f };    // En beats
        int numRepeats { 3 };
        float feedback { 0.5f };      // 0.0 - 1.0
        float velocityDecay { 0.8f }; // Multiplier per repeat
    };
    
    void setParameters(const Parameters& params) {
        params_ = params;
    }
    
    void processMidiBuffer(juce::MidiBuffer& buffer, double tempo, double sampleRate) {
        if (!params_.enabled) return;
        
        juce::MidiBuffer echoed;
        
        double beatsPerSecond = tempo / 60.0;
        int delaySamples = (params_.delayTime / beatsPerSecond) * sampleRate;
        
        // Copy original
        for (const auto metadata : buffer) {
            echoed.addEvent(metadata.getMessage(), metadata.samplePosition);
            
            // Generate echoes
            if (metadata.getMessage().isNoteOn()) {
                uint8_t velocity = metadata.getMessage().getVelocity();
                
                for (int repeat = 1; repeat <= params_.numRepeats; ++repeat) {
                    velocity *= params_.velocityDecay;
                    if (velocity < 1) break;
                    
                    int echoPosition = metadata.samplePosition + delaySamples * repeat;
                    
                    echoed.addEvent(juce::MidiMessage::noteOn(
                        metadata.getMessage().getChannel(),
                        metadata.getMessage().getNoteNumber(),
                        (uint8_t)velocity
                    ), echoPosition);
                }
            }
        }
        
        buffer.swapWith(echoed);
    }
    
private:
    Parameters params_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIEcho)
};

} // namespace MIDIFX
} // namespace Sequencer
} // namespace OmegaStudio
