#pragma once

#include "PianoRoll.h"
#include <JuceHeader.h>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @brief Scale definitions for scale highlighting
 */
enum class Scale {
    Chromatic,
    Major,
    Minor,
    HarmonicMinor,
    MelodicMinor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Pentatonic,
    Blues,
    Whole Tone,
    Diminished
};

/**
 * @brief Scale helper - gets notes in scale
 */
class ScaleHelper {
public:
    static std::vector<int> getScaleNotes(int rootNote, Scale scale) {
        std::vector<int> intervals;
        
        switch (scale) {
            case Scale::Major:
                intervals = {0, 2, 4, 5, 7, 9, 11};
                break;
            case Scale::Minor:
                intervals = {0, 2, 3, 5, 7, 8, 10};
                break;
            case Scale::HarmonicMinor:
                intervals = {0, 2, 3, 5, 7, 8, 11};
                break;
            case Scale::Pentatonic:
                intervals = {0, 2, 4, 7, 9};
                break;
            case Scale::Blues:
                intervals = {0, 3, 5, 6, 7, 10};
                break;
            case Scale::Dorian:
                intervals = {0, 2, 3, 5, 7, 9, 10};
                break;
            default:
                // Chromatic - all notes
                for (int i = 0; i < 12; ++i) {
                    intervals.push_back(i);
                }
                break;
        }
        
        std::vector<int> notes;
        for (int octave = 0; octave < 11; ++octave) {
            for (int interval : intervals) {
                int note = rootNote + (octave * 12) + interval;
                if (note >= 0 && note < 128) {
                    notes.push_back(note);
                }
            }
        }
        
        return notes;
    }
    
    static bool isNoteInScale(int note, int rootNote, Scale scale) {
        auto scaleNotes = getScaleNotes(rootNote, scale);
        return std::find(scaleNotes.begin(), scaleNotes.end(), note % 128) != scaleNotes.end();
    }
};

/**
 * @brief Chord generator for chordize tool
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
        Sus2,
        Sus4,
        Add9,
        Power
    };
    
    static std::vector<int> getChordIntervals(ChordType type) {
        switch (type) {
            case ChordType::Major:        return {0, 4, 7};
            case ChordType::Minor:        return {0, 3, 7};
            case ChordType::Diminished:   return {0, 3, 6};
            case ChordType::Augmented:    return {0, 4, 8};
            case ChordType::Major7:       return {0, 4, 7, 11};
            case ChordType::Minor7:       return {0, 3, 7, 10};
            case ChordType::Dominant7:    return {0, 4, 7, 10};
            case ChordType::Sus2:         return {0, 2, 7};
            case ChordType::Sus4:         return {0, 5, 7};
            case ChordType::Add9:         return {0, 4, 7, 14};
            case ChordType::Power:        return {0, 7};
            default:                      return {0};
        }
    }
    
    static std::vector<MIDINote> createChord(const MIDINote& rootNote, ChordType type, int voicing = 0) {
        std::vector<MIDINote> chord;
        auto intervals = getChordIntervals(type);
        
        for (size_t i = 0; i < intervals.size(); ++i) {
            MIDINote note = rootNote;
            note.noteNumber += intervals[i] + (voicing * 12);
            note.id = juce::Uuid();
            
            // Voice velocity slightly
            note.velocity = static_cast<uint8_t>(
                juce::jlimit(1, 127, rootNote.velocity - static_cast<int>(i) * 5)
            );
            
            if (note.noteNumber >= 0 && note.noteNumber < 128) {
                chord.push_back(note);
            }
        }
        
        return chord;
    }
};

/**
 * @brief Arpeggiator
 */
class Arpeggiator {
public:
    enum class Pattern {
        Up,
        Down,
        UpDown,
        DownUp,
        Random,
        AsPlayed
    };
    
    struct Settings {
        Pattern pattern{Pattern::Up};
        double noteLength{0.25};  // In beats
        int octaves{1};
        bool includeRoot{true};
    };
    
    static std::vector<MIDINote> arpeggiate(const std::vector<MIDINote>& chord, const Settings& settings) {
        if (chord.empty()) return {};
        
        std::vector<MIDINote> arpeggio;
        std::vector<int> noteNumbers;
        
        // Collect unique note numbers
        for (const auto& note : chord) {
            if (std::find(noteNumbers.begin(), noteNumbers.end(), note.noteNumber) == noteNumbers.end()) {
                noteNumbers.push_back(note.noteNumber);
            }
        }
        
        std::sort(noteNumbers.begin(), noteNumbers.end());
        
        // Extend for octaves
        std::vector<int> extended;
        for (int oct = 0; oct < settings.octaves; ++oct) {
            for (int note : noteNumbers) {
                extended.push_back(note + (oct * 12));
            }
        }
        
        // Apply pattern
        std::vector<int> sequence;
        switch (settings.pattern) {
            case Pattern::Up:
                sequence = extended;
                break;
            case Pattern::Down:
                sequence = extended;
                std::reverse(sequence.begin(), sequence.end());
                break;
            case Pattern::UpDown:
                sequence = extended;
                for (int i = static_cast<int>(extended.size()) - 2; i >= 0; --i) {
                    sequence.push_back(extended[i]);
                }
                break;
            case Pattern::Random:
                sequence = extended;
                std::random_shuffle(sequence.begin(), sequence.end());
                break;
            default:
                sequence = extended;
                break;
        }
        
        // Create arpeggio notes
        double time = chord[0].startTime;
        for (int noteNum : sequence) {
            MIDINote note;
            note.noteNumber = noteNum;
            note.startTime = time;
            note.duration = settings.noteLength;
            note.velocity = chord[0].velocity;
            note.channel = chord[0].channel;
            
            arpeggio.push_back(note);
            time += settings.noteLength;
        }
        
        return arpeggio;
    }
};

/**
 * @brief Strumming tool for guitar-like patterns
 */
class StrummingTool {
public:
    struct Settings {
        double strumTime{0.05};    // Time between notes in seconds
        bool downStrum{true};       // Down (low to high) or up
        double humanize{0.0};       // 0-1, random variation
    };
    
    static void applyStrum(std::vector<MIDINote>& chord, const Settings& settings) {
        if (chord.size() <= 1) return;
        
        // Sort by pitch
        std::sort(chord.begin(), chord.end(), 
            [](const MIDINote& a, const MIDINote& b) {
                return a.noteNumber < b.noteNumber;
            });
        
        if (!settings.downStrum) {
            std::reverse(chord.begin(), chord.end());
        }
        
        double baseTime = chord[0].startTime;
        
        for (size_t i = 0; i < chord.size(); ++i) {
            double offset = i * settings.strumTime;
            
            // Add humanization
            if (settings.humanize > 0.0) {
                float random = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
                offset += random * settings.humanize * settings.strumTime;
            }
            
            chord[i].startTime = baseTime + offset;
        }
    }
};

/**
 * @brief Ghost notes - show notes from other tracks
 */
class GhostNoteManager {
public:
    struct GhostTrack {
        int trackId{-1};
        juce::String trackName;
        juce::Colour colour{juce::Colours::grey};
        float opacity{0.3f};
        bool visible{true};
    };
    
    void addGhostTrack(int trackId, const juce::String& name, juce::Colour colour) {
        GhostTrack track;
        track.trackId = trackId;
        track.trackName = name;
        track.colour = colour;
        ghostTracks_.push_back(track);
    }
    
    void removeGhostTrack(int trackId) {
        ghostTracks_.erase(
            std::remove_if(ghostTracks_.begin(), ghostTracks_.end(),
                [trackId](const GhostTrack& t) { return t.trackId == trackId; }),
            ghostTracks_.end()
        );
    }
    
    void setGhostTrackVisible(int trackId, bool visible) {
        for (auto& track : ghostTracks_) {
            if (track.trackId == trackId) {
                track.visible = visible;
                break;
            }
        }
    }
    
    const std::vector<GhostTrack>& getGhostTracks() const {
        return ghostTracks_;
    }
    
private:
    std::vector<GhostTrack> ghostTracks_;
};

/**
 * @brief Performance mode keyboard (play and record in real-time)
 */
class PerformanceKeyboard : public juce::Component,
                            public juce::MidiKeyboardStateListener {
public:
    PerformanceKeyboard() {
        keyboardState_.addListener(this);
        
        keyboard_ = std::make_unique<juce::MidiKeyboardComponent>(
            keyboardState_,
            juce::MidiKeyboardComponent::horizontalKeyboard
        );
        
        addAndMakeVisible(keyboard_.get());
        
        // Recording controls
        recordButton_ = std::make_unique<juce::TextButton>("Record");
        recordButton_->onClick = [this] { toggleRecording(); };
        addAndMakeVisible(recordButton_.get());
        
        playButton_ = std::make_unique<juce::TextButton>("Play");
        addAndMakeVisible(playButton_.get());
        
        metronomeButton_ = std::make_unique<juce::ToggleButton>("Metronome");
        addAndMakeVisible(metronomeButton_.get());
        
        countInBox_ = std::make_unique<juce::ComboBox>();
        countInBox_->addItem("No count-in", 1);
        countInBox_->addItem("1 bar", 2);
        countInBox_->addItem("2 bars", 3);
        countInBox_->addItem("4 bars", 4);
        countInBox_->setSelectedId(1);
        addAndMakeVisible(countInBox_.get());
    }
    
    ~PerformanceKeyboard() override {
        keyboardState_.removeListener(this);
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        
        // Controls at top
        auto controls = bounds.removeFromTop(40);
        recordButton_->setBounds(controls.removeFromLeft(80).reduced(5));
        playButton_->setBounds(controls.removeFromLeft(80).reduced(5));
        metronomeButton_->setBounds(controls.removeFromLeft(100).reduced(5));
        countInBox_->setBounds(controls.removeFromLeft(120).reduced(5));
        
        // Keyboard
        keyboard_->setBounds(bounds);
    }
    
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, 
                     int midiNoteNumber, float velocity) override {
        if (isRecording_) {
            recordNote(midiNoteNumber, velocity, true);
        }
        
        if (onNoteOn) {
            onNoteOn(midiChannel, midiNoteNumber, velocity);
        }
    }
    
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, 
                      int midiNoteNumber, float velocity) override {
        if (isRecording_) {
            recordNote(midiNoteNumber, velocity, false);
        }
        
        if (onNoteOff) {
            onNoteOff(midiChannel, midiNoteNumber, velocity);
        }
    }
    
    void startRecording(double currentTime) {
        isRecording_ = true;
        recordStartTime_ = currentTime;
        activeNotes_.clear();
        recordedNotes_.clear();
        recordButton_->setButtonText("Stop");
        recordButton_->setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    }
    
    void stopRecording() {
        isRecording_ = false;
        
        // Finalize any active notes
        for (auto& pair : activeNotes_) {
            pair.second.duration = (juce::Time::getMillisecondCounterHiRes() / 1000.0) - pair.second.startTime;
            recordedNotes_.push_back(pair.second);
        }
        activeNotes_.clear();
        
        recordButton_->setButtonText("Record");
        recordButton_->setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    }
    
    std::vector<MIDINote> getRecordedNotes() const {
        return recordedNotes_;
    }
    
    void clearRecording() {
        recordedNotes_.clear();
    }
    
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState_; }
    
    std::function<void(int, int, float)> onNoteOn;
    std::function<void(int, int, float)> onNoteOff;
    std::function<void(const std::vector<MIDINote>&)> onRecordingComplete;
    
private:
    juce::MidiKeyboardState keyboardState_;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboard_;
    
    std::unique_ptr<juce::TextButton> recordButton_;
    std::unique_ptr<juce::TextButton> playButton_;
    std::unique_ptr<juce::ToggleButton> metronomeButton_;
    std::unique_ptr<juce::ComboBox> countInBox_;
    
    bool isRecording_{false};
    double recordStartTime_{0.0};
    std::map<int, MIDINote> activeNotes_;  // noteNumber -> Note
    std::vector<MIDINote> recordedNotes_;
    
    void toggleRecording() {
        if (isRecording_) {
            stopRecording();
            if (onRecordingComplete) {
                onRecordingComplete(recordedNotes_);
            }
        } else {
            startRecording(juce::Time::getMillisecondCounterHiRes() / 1000.0);
        }
    }
    
    void recordNote(int noteNumber, float velocity, bool isNoteOn) {
        double currentTime = (juce::Time::getMillisecondCounterHiRes() / 1000.0) - recordStartTime_;
        
        if (isNoteOn) {
            MIDINote note;
            note.noteNumber = noteNumber;
            note.startTime = currentTime;
            note.velocity = static_cast<uint8_t>(velocity * 127.0f);
            note.duration = 0.0;
            
            activeNotes_[noteNumber] = note;
        } else {
            auto it = activeNotes_.find(noteNumber);
            if (it != activeNotes_.end()) {
                it->second.duration = currentTime - it->second.startTime;
                recordedNotes_.push_back(it->second);
                activeNotes_.erase(it);
            }
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceKeyboard)
};

/**
 * @brief Enhanced Piano Roll with advanced features
 */
class PianoRollAdvanced : public juce::Component {
public:
    PianoRollAdvanced();
    ~PianoRollAdvanced() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Scale highlighting
    void setScale(int rootNote, Scale scale);
    void setScaleVisible(bool visible) { scaleHighlightEnabled_ = visible; repaint(); }
    
    // Ghost notes
    void addGhostTrack(int trackId, const juce::String& name, juce::Colour colour);
    void removeGhostTrack(int trackId);
    void setGhostNotesVisible(bool visible) { ghostNotesEnabled_ = visible; repaint(); }
    
    // Tools
    void enableChordizeTool(bool enable) { chordizeEnabled_ = enable; }
    void setChordType(ChordGenerator::ChordType type) { currentChordType_ = type; }
    void enableArpeggiator(bool enable) { arpeggiatorEnabled_ = enable; }
    void setArpeggiatorSettings(const Arpeggiator::Settings& settings) { arpSettings_ = settings; }
    void enableStrumming(bool enable) { strummingEnabled_ = enable; }
    void setStrummingSettings(const StrummingTool::Settings& settings) { strumSettings_ = settings; }
    
    // Performance mode
    void setPerformanceModeEnabled(bool enable);
    bool isPerformanceModeEnabled() const { return performanceModeEnabled_; }
    
    // Clip management
    void setClip(MIDIClip* clip) { currentClip_ = clip; repaint(); }
    MIDIClip* getClip() { return currentClip_; }
    
private:
    MIDIClip* currentClip_{nullptr};
    
    // Scale highlighting
    bool scaleHighlightEnabled_{false};
    int scaleRootNote_{60};  // C
    Scale currentScale_{Scale::Major};
    
    // Ghost notes
    bool ghostNotesEnabled_{false};
    GhostNoteManager ghostNoteManager_;
    
    // Tools
    bool chordizeEnabled_{false};
    ChordGenerator::ChordType currentChordType_{ChordGenerator::ChordType::Major};
    
    bool arpeggiatorEnabled_{false};
    Arpeggiator::Settings arpSettings_;
    
    bool strummingEnabled_{false};
    StrummingTool::Settings strumSettings_;
    
    // Performance mode
    bool performanceModeEnabled_{false};
    std::unique_ptr<PerformanceKeyboard> performanceKeyboard_;
    
    // Rendering
    void drawScaleHighlighting(juce::Graphics& g);
    void drawGhostNotes(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollAdvanced)
};

} // namespace Sequencer
} // namespace OmegaStudio
