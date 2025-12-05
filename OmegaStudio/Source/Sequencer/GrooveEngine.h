#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @struct GrooveTemplate
 * @brief Template de groove con micro-timing y velocity
 */
struct GrooveTemplate {
    juce::String name;
    int steps { 16 };  // 16th notes por defecto
    std::vector<float> timing;     // Offset en ms (-100 a +100)
    std::vector<float> velocity;   // Velocity multiplier (0.0 - 2.0)
    float swing { 0.0f };          // Global swing (0.0 - 1.0)
    
    GrooveTemplate(const juce::String& n = "Straight", int numSteps = 16)
        : name(n), steps(numSteps) {
        timing.resize(numSteps, 0.0f);
        velocity.resize(numSteps, 1.0f);
    }
};

/**
 * @class GrooveLibrary
 * @brief Biblioteca de grooves predefinidos
 */
class GrooveLibrary {
public:
    static GrooveTemplate createJazz() {
        GrooveTemplate groove("Jazz Swing", 16);
        
        // Swing pattern: even beats early, odd beats late
        for (int i = 0; i < 16; ++i) {
            if (i % 2 == 1) {  // Odd steps (offbeats)
                groove.timing[i] = 30.0f;  // Delay by 30ms
                groove.velocity[i] = 0.7f;  // Softer
            } else {
                groove.timing[i] = 0.0f;
                groove.velocity[i] = 1.0f;
            }
        }
        
        groove.swing = 0.66f;
        return groove;
    }
    
    static GrooveTemplate createFunk() {
        GrooveTemplate groove("Funk Pocket", 16);
        
        // Funk: heavy on 1 and 3, lighter on 2 and 4
        for (int i = 0; i < 16; ++i) {
            int beat = i / 4;
            
            if (beat == 0 || beat == 2) {  // Beats 1 and 3
                groove.velocity[i] = 1.2f;
                groove.timing[i] = -5.0f;  // Slightly ahead
            } else if (beat == 1 || beat == 3) {  // Beats 2 and 4
                groove.velocity[i] = 1.0f;
                groove.timing[i] = 8.0f;  // Slightly behind
            } else {
                groove.velocity[i] = 0.8f;
                groove.timing[i] = 0.0f;
            }
        }
        
        groove.swing = 0.55f;
        return groove;
    }
    
    static GrooveTemplate createHipHop() {
        GrooveTemplate groove("Hip-Hop Boom Bap", 16);
        
        // Hip-hop: laid-back, heavy hits
        for (int i = 0; i < 16; ++i) {
            if (i == 0 || i == 8) {  // Kick positions
                groove.velocity[i] = 1.3f;
                groove.timing[i] = 15.0f;  // Laid back
            } else if (i == 4 || i == 12) {  // Snare positions
                groove.velocity[i] = 1.1f;
                groove.timing[i] = 20.0f;  // Very laid back
            } else {
                groove.velocity[i] = 0.75f;
                groove.timing[i] = 10.0f;
            }
        }
        
        groove.swing = 0.60f;
        return groove;
    }
    
    static GrooveTemplate createTrap() {
        GrooveTemplate groove("Trap", 32);  // 32nd notes for hi-hats
        groove.timing.resize(32);
        groove.velocity.resize(32);
        
        // Trap: triplet feel on hi-hats
        for (int i = 0; i < 32; ++i) {
            if (i % 3 == 0) {  // Every 3rd step
                groove.velocity[i] = 1.0f;
                groove.timing[i] = 0.0f;
            } else {
                groove.velocity[i] = 0.6f;
                groove.timing[i] = 5.0f;
            }
        }
        
        groove.swing = 0.66f;  // Triplet swing
        return groove;
    }
    
    static GrooveTemplate createDrill() {
        GrooveTemplate groove("UK Drill", 16);
        
        // Drill: syncopated, aggressive
        for (int i = 0; i < 16; ++i) {
            if (i % 4 == 0) {  // Downbeats
                groove.velocity[i] = 1.4f;
                groove.timing[i] = -8.0f;  // Pushed forward
            } else if (i % 4 == 2) {
                groove.velocity[i] = 1.1f;
                groove.timing[i] = 12.0f;  // Pulled back
            } else {
                groove.velocity[i] = 0.7f;
                groove.timing[i] = 0.0f;
            }
        }
        
        groove.swing = 0.50f;
        return groove;
    }
    
    static GrooveTemplate createReggaeton() {
        GrooveTemplate groove("Reggaeton/Dembow", 16);
        
        // Dembow pattern
        std::vector<int> accentSteps = {0, 3, 6, 10, 12};
        
        for (int i = 0; i < 16; ++i) {
            bool isAccent = std::find(accentSteps.begin(), accentSteps.end(), i) 
                           != accentSteps.end();
            
            if (isAccent) {
                groove.velocity[i] = 1.3f;
                groove.timing[i] = 5.0f;  // Slightly behind
            } else {
                groove.velocity[i] = 0.6f;
                groove.timing[i] = 0.0f;
            }
        }
        
        groove.swing = 0.52f;
        return groove;
    }
    
    static GrooveTemplate createStraight() {
        return GrooveTemplate("Straight", 16);
    }
    
    static GrooveTemplate createShuffle() {
        GrooveTemplate groove("Shuffle", 16);
        
        for (int i = 0; i < 16; ++i) {
            if (i % 2 == 1) {
                groove.timing[i] = 40.0f;  // Heavy shuffle
            }
        }
        
        groove.swing = 0.75f;
        return groove;
    }
};

/**
 * @class GrooveEngine
 * @brief Motor que aplica grooves a MIDI y audio
 */
class GrooveEngine {
public:
    GrooveEngine() {
        loadDefaultGrooves();
    }
    
    void setGroove(const juce::String& grooveName) {
        if (grooves_.count(grooveName) > 0) {
            currentGroove_ = grooves_[grooveName];
            enabled_ = true;
        }
    }
    
    void setCustomGroove(const GrooveTemplate& groove) {
        currentGroove_ = groove;
        enabled_ = true;
    }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    void setAmount(float amount) { amount_ = juce::jlimit(0.0f, 1.0f, amount); }
    float getAmount() const { return amount_; }
    
    /**
     * Aplica groove a MIDI buffer
     */
    void processMIDI(juce::MidiBuffer& buffer, double sampleRate, double tempo) {
        if (!enabled_ || amount_ <= 0.0f) return;
        
        juce::MidiBuffer processed;
        
        double beatsPerSample = tempo / (60.0 * sampleRate);
        double samplesPerStep = (60.0 / tempo) * sampleRate / (currentGroove_.steps / 4.0);
        
        for (const auto metadata : buffer) {
            auto message = metadata.getMessage();
            int newPosition = metadata.samplePosition;
            
            if (message.isNoteOnOrOff()) {
                // Calculate which step we're on
                double beatPos = metadata.samplePosition * beatsPerSample;
                int step = std::fmod(beatPos * currentGroove_.steps, currentGroove_.steps);
                
                // Apply timing offset
                float timingOffset = currentGroove_.timing[step] * 0.001 * sampleRate;  // ms to samples
                newPosition += timingOffset * amount_;
                
                // Apply velocity adjustment
                if (message.isNoteOn()) {
                    float velMultiplier = currentGroove_.velocity[step];
                    uint8_t newVel = juce::jlimit(1, 127, 
                        (int)(message.getVelocity() * (1.0f + (velMultiplier - 1.0f) * amount_)));
                    
                    message = juce::MidiMessage::noteOn(
                        message.getChannel(),
                        message.getNoteNumber(),
                        newVel
                    );
                }
            }
            
            processed.addEvent(message, std::max(0, newPosition));
        }
        
        buffer.swapWith(processed);
    }
    
    /**
     * Extrae groove de audio mediante onset detection
     */
    GrooveTemplate extractGrooveFromAudio(const juce::AudioBuffer<float>& buffer, 
                                         double sampleRate, 
                                         double estimatedTempo) {
        GrooveTemplate extracted("Extracted", 16);
        
        // Simple onset detection
        std::vector<int> onsets = detectOnsets(buffer, sampleRate);
        
        if (onsets.size() < 2) return extracted;
        
        // Calculate average IOI
        double avgIOI = 0.0;
        for (size_t i = 1; i < onsets.size(); ++i) {
            avgIOI += onsets[i] - onsets[i-1];
        }
        avgIOI /= (onsets.size() - 1);
        
        // Map onsets to 16th note grid
        double samplesPerSixteenth = (60.0 / estimatedTempo) * sampleRate / 4.0;
        
        for (size_t i = 0; i < onsets.size() && i < 16; ++i) {
            int expectedPos = i * samplesPerSixteenth;
            int actualPos = onsets[i];
            float offset = (actualPos - expectedPos) / sampleRate * 1000.0;  // ms
            
            int step = i % 16;
            extracted.timing[step] = offset;
        }
        
        return extracted;
    }
    
    std::vector<juce::String> getAvailableGrooves() const {
        std::vector<juce::String> names;
        for (const auto& [name, _] : grooves_) {
            names.push_back(name);
        }
        return names;
    }
    
private:
    void loadDefaultGrooves() {
        grooves_["Straight"] = GrooveLibrary::createStraight();
        grooves_["Jazz Swing"] = GrooveLibrary::createJazz();
        grooves_["Funk Pocket"] = GrooveLibrary::createFunk();
        grooves_["Hip-Hop Boom Bap"] = GrooveLibrary::createHipHop();
        grooves_["Trap"] = GrooveLibrary::createTrap();
        grooves_["UK Drill"] = GrooveLibrary::createDrill();
        grooves_["Reggaeton"] = GrooveLibrary::createReggaeton();
        grooves_["Shuffle"] = GrooveLibrary::createShuffle();
    }
    
    std::vector<int> detectOnsets(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        std::vector<int> onsets;
        
        const int hopSize = 512;
        const float threshold = 0.3f;
        
        // Simple energy-based detection
        for (int pos = 0; pos < buffer.getNumSamples() - hopSize; pos += hopSize) {
            float energy = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < hopSize; ++i) {
                    energy += std::abs(data[pos + i]);
                }
            }
            
            energy /= (hopSize * buffer.getNumChannels());
            
            if (energy > threshold) {
                onsets.push_back(pos);
            }
        }
        
        return onsets;
    }
    
    std::map<juce::String, GrooveTemplate> grooves_;
    GrooveTemplate currentGroove_;
    bool enabled_ { false };
    float amount_ { 1.0f };  // 0.0 - 1.0
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrooveEngine)
};

} // namespace Sequencer
} // namespace OmegaStudio
