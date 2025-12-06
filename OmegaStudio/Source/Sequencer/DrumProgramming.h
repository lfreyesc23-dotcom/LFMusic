//==============================================================================
// DrumProgramming.h - Drum Programming Avanzado (FL Studio Style)
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <array>

namespace OmegaStudio {

//==============================================================================
/** Drum Programming con Swing, Groove, Choke Groups y Roll Tool
 *  - Swing/Groove templates (MPC, J Dilla, etc.)
 *  - Drum randomizer (velocity, timing, pitch)
 *  - Choke groups (hi-hat open/close)
 *  - Roll tool (FL Studio signature feature)
 *  - Step LFO por step (per-step modulation)
 *  - Drum kit manager con preview
 */
class DrumProgrammingEngine {
public:
    //==========================================================================
    struct DrumStep {
        bool enabled = false;
        float velocity = 0.8f;          // 0.0 - 1.0
        float probability = 1.0f;       // 0.0 - 1.0 (chance de trigger)
        int retriggers = 0;             // 0 = normal, >0 = roll
        float retriggerRate = 0.25f;    // En beats
        
        // Per-step modulation
        float pitchOffset = 0.0f;       // Semitones
        float panOffset = 0.0f;         // -1.0 (L) a 1.0 (R)
        float filterCutoff = 0.5f;      // 0.0 - 1.0
        float decay = 0.5f;             // 0.0 - 1.0
        
        // Humanization
        float timingOffset = 0.0f;      // En ticks (-100 a 100)
        float velocityRandom = 0.0f;    // 0.0 - 1.0 (amount of randomization)
    };
    
    //==========================================================================
    struct DrumChannel {
        juce::String name = "Kick";
        juce::String samplePath;
        int midiNote = 36;              // C1
        
        std::array<DrumStep, 32> steps; // 32 steps (2 bars)
        
        bool muted = false;
        bool solo = false;
        float volume = 1.0f;
        float pan = 0.5f;
        
        int chokeGroup = -1;            // -1 = none, 0+ = group ID
        
        // Per-channel settings
        float swing = 0.0f;             // 0.0 - 1.0
        float humanize = 0.0f;          // 0.0 - 1.0
    };
    
    //==========================================================================
    enum class GrooveTemplate {
        None,
        MPC60,                  // Classic MPC swing
        MPC3000,                // Tighter MPC swing
        JDilla,                 // Legendary Dilla feel
        Trap,                   // Modern trap groove
        DrumnBass,              // DnB shuffle
        House,                  // Four-on-floor groove
        Reggaeton,              // Reggaeton dembow
        Swing16th,              // 16th note swing
        Swing32nd,              // 32nd note swing
        Triplet,                // Triplet feel
        Custom                  // Usuario define
    };
    
    //==========================================================================
    DrumProgrammingEngine();
    ~DrumProgrammingEngine() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void setTempo(double bpm);
    
    // Channels
    void addChannel(const DrumChannel& channel);
    void removeChannel(int index);
    int getNumChannels() const { return static_cast<int>(channels_.size()); }
    
    DrumChannel& getChannel(int index) { return channels_[index]; }
    const DrumChannel& getChannel(int index) const { return channels_[index]; }
    
    // Pattern editing
    void setStep(int channelIndex, int stepIndex, bool enabled);
    void setStepVelocity(int channelIndex, int stepIndex, float velocity);
    void setStepRetriggers(int channelIndex, int stepIndex, int retriggers);
    
    void clearChannel(int channelIndex);
    void clearAll();
    
    // Roll Tool (FL Studio signature)
    void applyRoll(int channelIndex, int startStep, int endStep, 
                   float rate);  // rate en beats (0.25 = 1/16, 0.125 = 1/32)
    
    // Groove & Swing
    void setGrooveTemplate(GrooveTemplate template_);
    void setCustomGroove(const std::vector<float>& timingOffsets);  // 16 valores
    GrooveTemplate getGrooveTemplate() const { return grooveTemplate_; }
    
    void setGlobalSwing(float swing);  // 0.0 - 1.0
    float getGlobalSwing() const { return globalSwing_; }
    
    // Randomization
    struct RandomizeSettings {
        float timingAmount = 0.1f;      // 0.0 - 1.0
        float velocityAmount = 0.2f;    // 0.0 - 1.0
        float pitchAmount = 0.0f;       // 0.0 - 1.0
        float probabilityAmount = 0.0f; // 0.0 - 1.0
    };
    
    void randomizeChannel(int channelIndex, const RandomizeSettings& settings);
    void humanizeChannel(int channelIndex, float amount);
    
    // Choke Groups
    void setChokeGroup(int channelIndex, int groupId);
    int getChokeGroup(int channelIndex) const;
    
    // Playback
    void setPlayheadPosition(double beats);
    double getPlayheadPosition() const { return playheadPosition_; }
    
    void setLoopEnabled(bool enabled) { loopEnabled_ = enabled; }
    void setLoopRange(double startBeats, double endBeats);
    
    // MIDI Output
    juce::MidiBuffer generateMidiForRange(double startBeats, double endBeats);
    
    // Presets
    void loadDrumKit(const juce::String& kitName);
    void loadPattern(const juce::String& patternName);
    
    juce::StringArray getAvailableKits() const;
    juce::StringArray getAvailablePatterns() const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::vector<DrumChannel> channels_;
    
    double sampleRate_ = 48000.0;
    double tempo_ = 120.0;
    
    GrooveTemplate grooveTemplate_ = GrooveTemplate::None;
    std::vector<float> customGrooveTiming_;
    float globalSwing_ = 0.0f;
    
    double playheadPosition_ = 0.0;
    bool loopEnabled_ = true;
    double loopStart_ = 0.0;
    double loopEnd_ = 8.0;  // 2 bars
    
    // Choke group state
    std::map<int, int> chokeGroupLastChannel_;  // groupId -> channelIndex
    
    // Groove calculations
    float calculateSwingOffset(int stepIndex, float swingAmount) const;
    float getGrooveOffsetForStep(int stepIndex) const;
    
    // Helpers
    juce::Random random_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumProgrammingEngine)
};

//==============================================================================
/** Groove Templates - Timing offsets por estilo */
class GrooveTemplates {
public:
    static std::vector<float> getGrooveOffsets(DrumProgrammingEngine::GrooveTemplate template_) {
        std::vector<float> offsets(16, 0.0f);  // 16 steps
        
        switch (template_) {
            case DrumProgrammingEngine::GrooveTemplate::MPC60:
                // MPC60: Heavy swing en 16ths
                for (int i = 0; i < 16; ++i) {
                    if (i % 2 == 1) {
                        offsets[i] = 0.08f;  // Delay even 16ths
                    }
                }
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::MPC3000:
                // MPC3000: Tighter swing
                for (int i = 0; i < 16; ++i) {
                    if (i % 2 == 1) {
                        offsets[i] = 0.05f;
                    }
                }
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::JDilla:
                // J Dilla: Loose, behind-the-beat feel
                offsets = {0.0f, 0.12f, -0.02f, 0.10f,
                           0.0f, 0.12f, -0.02f, 0.10f,
                           0.0f, 0.12f, -0.02f, 0.10f,
                           0.0f, 0.12f, -0.02f, 0.10f};
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::Trap:
                // Trap: Hi-hats on-grid, kicks slightly ahead
                offsets = {-0.01f, 0.0f, 0.0f, 0.0f,
                           -0.01f, 0.0f, 0.0f, 0.0f,
                           -0.01f, 0.0f, 0.0f, 0.0f,
                           -0.01f, 0.0f, 0.0f, 0.0f};
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::DrumnBass:
                // DnB: Breakbeat shuffle
                offsets = {0.0f, -0.03f, 0.05f, 0.02f,
                           0.0f, 0.03f, -0.02f, 0.05f,
                           0.0f, -0.03f, 0.05f, 0.02f,
                           0.0f, 0.03f, -0.02f, 0.05f};
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::House:
                // House: Tight 4/4
                for (int i = 0; i < 16; ++i) {
                    if (i % 4 == 0) {
                        offsets[i] = -0.01f;  // Kicks slightly ahead
                    }
                }
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::Reggaeton:
                // Reggaeton dembow: Characteristic bounce
                offsets = {0.0f, 0.0f, 0.0f, 0.03f,
                           0.0f, 0.0f, 0.05f, 0.0f,
                           0.0f, 0.0f, 0.0f, 0.03f,
                           0.0f, 0.0f, 0.05f, 0.0f};
                break;
            
            case DrumProgrammingEngine::GrooveTemplate::Swing16th:
                // Classic 16th swing (66.6%)
                for (int i = 0; i < 16; ++i) {
                    if (i % 2 == 1) {
                        offsets[i] = 0.0667f;  // 1/3 of 16th
                    }
                }
                break;
            
            default:
                break;
        }
        
        return offsets;
    }
};

//==============================================================================
/** Drum Kit Manager - Presets de kits profesionales */
class DrumKitManager {
public:
    struct DrumKit {
        juce::String name;
        std::map<juce::String, juce::String> samples;  // channel name -> sample path
    };
    
    static std::vector<DrumKit> getBuiltInKits() {
        std::vector<DrumKit> kits;
        
        // Kit 1: 808
        DrumKit kit808;
        kit808.name = "808";
        kit808.samples = {
            {"Kick", "samples/808/kick.wav"},
            {"Snare", "samples/808/snare.wav"},
            {"Clap", "samples/808/clap.wav"},
            {"Hi-Hat Closed", "samples/808/hh_closed.wav"},
            {"Hi-Hat Open", "samples/808/hh_open.wav"},
            {"Tom Low", "samples/808/tom_low.wav"},
            {"Tom Mid", "samples/808/tom_mid.wav"},
            {"Tom High", "samples/808/tom_high.wav"}
        };
        kits.push_back(kit808);
        
        // Kit 2: Acoustic
        DrumKit kitAcoustic;
        kitAcoustic.name = "Acoustic";
        kitAcoustic.samples = {
            {"Kick", "samples/acoustic/kick.wav"},
            {"Snare", "samples/acoustic/snare.wav"},
            {"Hi-Hat Closed", "samples/acoustic/hh_closed.wav"},
            {"Hi-Hat Open", "samples/acoustic/hh_open.wav"},
            {"Crash", "samples/acoustic/crash.wav"},
            {"Ride", "samples/acoustic/ride.wav"}
        };
        kits.push_back(kitAcoustic);
        
        // Kit 3: Trap
        DrumKit kitTrap;
        kitTrap.name = "Trap";
        kitTrap.samples = {
            {"Kick", "samples/trap/kick.wav"},
            {"Snare", "samples/trap/snare.wav"},
            {"Hi-Hat", "samples/trap/hihat.wav"},
            {"Rim", "samples/trap/rim.wav"},
            {"Perc", "samples/trap/perc.wav"}
        };
        kits.push_back(kitTrap);
        
        return kits;
    }
};

//==============================================================================
/** Pattern Presets - Patterns comunes por género */
class DrumPatternPresets {
public:
    static void applyPattern(DrumProgrammingEngine& engine, const juce::String& patternName) {
        engine.clearAll();
        
        if (patternName == "Four On The Floor") {
            // Kick en cada beat
            for (int i = 0; i < 16; i += 4) {
                engine.setStep(0, i, true);  // Kick
            }
            // Hi-hat en 16ths
            for (int i = 0; i < 16; ++i) {
                engine.setStep(2, i, true);  // Closed HH
            }
            // Snare en beats 2 y 4
            engine.setStep(1, 4, true);
            engine.setStep(1, 12, true);
        }
        else if (patternName == "Trap") {
            // Kick en 1, 2.5, 3
            engine.setStep(0, 0, true);
            engine.setStep(0, 6, true);
            engine.setStep(0, 8, true);
            
            // Snare en 2 y 4
            engine.setStep(1, 4, true);
            engine.setStep(1, 12, true);
            
            // Hi-hat roll (32nds)
            for (int i = 0; i < 16; ++i) {
                engine.setStep(2, i, true);
                engine.setStepVelocity(2, i, 0.6f + (i % 2) * 0.2f);
            }
        }
        else if (patternName == "Breakbeat") {
            // Classic Amen break pattern (simplified)
            engine.setStep(0, 0, true);   // Kick
            engine.setStep(0, 10, true);
            
            engine.setStep(1, 4, true);   // Snare
            engine.setStep(1, 12, true);
            engine.setStep(1, 14, true);
            
            // Hi-hats
            for (int i = 0; i < 16; i += 2) {
                engine.setStep(2, i, true);
            }
        }
    }
};

} // namespace OmegaStudio
