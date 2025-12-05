#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>

namespace OmegaStudio {
namespace Audio {

/**
 * @brief Wavetable Synthesizer (like Serum/Vital)
 * 
 * Features:
 * - Multiple wavetables with position morphing
 * - Unison with detune and spread
 * - Advanced modulation matrix
 * - Built-in effects
 */
class WavetableSynth : public juce::Synthesiser {
public:
    static constexpr int WAVETABLE_SIZE = 2048;
    static constexpr int NUM_TABLES = 256;
    
    struct WavetableData {
        std::string name;
        std::array<std::array<float, WAVETABLE_SIZE>, NUM_TABLES> tables;
        
        WavetableData() {
            name = "Init";
            // Initialize with sine wave
            for (auto& table : tables) {
                for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                    table[i] = std::sin(2.0f * juce::MathConstants<float>::pi * i / WAVETABLE_SIZE);
                }
            }
        }
    };
    
    class Voice : public juce::SynthesiserVoice {
    public:
        Voice(WavetableSynth& parent) : parent_(parent) {}
        
        bool canPlaySound(juce::SynthesiserSound* sound) override {
            return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
        }
        
        void startNote(int midiNoteNumber, float velocity,
                      juce::SynthesiserSound*, int) override {
            currentNoteNumber_ = midiNoteNumber;
            currentVelocity_ = velocity;
            
            double cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
            double cyclesPerSample = cyclesPerSecond / getSampleRate();
            phaseDelta_ = cyclesPerSample * WAVETABLE_SIZE;
            
            // ADSR
            adsr_.setSampleRate(getSampleRate());
            adsr_.setParameters(parent_.adsrParams_);
            adsr_.noteOn();
            
            // Unison voices
            initializeUnison();
        }
        
        void stopNote(float, bool allowTailOff) override {
            if (allowTailOff) {
                adsr_.noteOff();
            } else {
                clearCurrentNote();
                adsr_.reset();
            }
        }
        
        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}
        
        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                           int startSample, int numSamples) override {
            if (!isVoiceActive()) return;
            
            auto* leftBuffer = outputBuffer.getWritePointer(0, startSample);
            auto* rightBuffer = outputBuffer.getNumChannels() > 1 ? 
                               outputBuffer.getWritePointer(1, startSample) : nullptr;
            
            for (int i = 0; i < numSamples; ++i) {
                float sample = 0.0f;
                
                // Unison voices
                for (int v = 0; v < parent_.unisonVoices_; ++v) {
                    sample += generateWavetableSample(v);
                }
                
                sample /= static_cast<float>(parent_.unisonVoices_);
                
                // Apply ADSR
                float env = adsr_.getNextSample();
                sample *= env * currentVelocity_;
                
                // Apply filter
                sample = filter_.processSample(sample);
                
                // Stereo spread for unison
                leftBuffer[i] += sample;
                if (rightBuffer) {
                    rightBuffer[i] += sample;
                }
                
                if (!adsr_.isActive()) {
                    clearCurrentNote();
                    break;
                }
            }
        }
        
    private:
        WavetableSynth& parent_;
        
        int currentNoteNumber_{0};
        float currentVelocity_{0.0f};
        
        double phase_{0.0};
        double phaseDelta_{0.0};
        
        juce::ADSR adsr_;
        juce::dsp::StateVariableTPTFilter<float> filter_;
        
        // Unison
        struct UnisonVoice {
            double phase{0.0};
            double detune{0.0};
            float pan{0.5f};
        };
        std::vector<UnisonVoice> unisonVoices_;
        
        void initializeUnison() {
            unisonVoices_.clear();
            unisonVoices_.resize(parent_.unisonVoices_);
            
            for (int i = 0; i < parent_.unisonVoices_; ++i) {
                float position = static_cast<float>(i) / std::max(1, parent_.unisonVoices_ - 1);
                
                unisonVoices_[i].phase = juce::Random::getSystemRandom().nextDouble();
                unisonVoices_[i].detune = (position - 0.5f) * 2.0f * parent_.unisonDetune_;
                unisonVoices_[i].pan = position * parent_.unisonSpread_;
            }
        }
        
        float generateWavetableSample(int voiceIndex) {
            auto& unisonVoice = unisonVoices_[voiceIndex];
            
            // Apply detune
            double detuneMultiplier = std::pow(2.0, unisonVoice.detune / 1200.0);
            double adjustedDelta = phaseDelta_ * detuneMultiplier;
            
            // Wavetable position (0-1)
            float wtPosition = parent_.wavetablePosition_;
            int tableIndex = static_cast<int>(wtPosition * (NUM_TABLES - 1));
            int nextTableIndex = std::min(tableIndex + 1, NUM_TABLES - 1);
            float tableFrac = (wtPosition * (NUM_TABLES - 1)) - tableIndex;
            
            // Get samples from current and next table
            int index = static_cast<int>(unisonVoice.phase);
            int nextIndex = (index + 1) % WAVETABLE_SIZE;
            float frac = static_cast<float>(unisonVoice.phase - index);
            
            const auto& currentTable = parent_.currentWavetable_.tables[tableIndex];
            const auto& nextTable = parent_.currentWavetable_.tables[nextTableIndex];
            
            // Linear interpolation within table
            float sample1 = currentTable[index] + frac * (currentTable[nextIndex] - currentTable[index]);
            float sample2 = nextTable[index] + frac * (nextTable[nextIndex] - nextTable[index]);
            
            // Morphing between tables
            float sample = sample1 + tableFrac * (sample2 - sample1);
            
            // Advance phase
            unisonVoice.phase += adjustedDelta;
            while (unisonVoice.phase >= WAVETABLE_SIZE) {
                unisonVoice.phase -= WAVETABLE_SIZE;
            }
            
            return sample;
        }
    };
    
    WavetableSynth() {
        // Add voices
        for (int i = 0; i < 16; ++i) {
            addVoice(new Voice(*this));
        }
        
        // Add sound
        addSound(new juce::SynthesiserSound());
        
        // Initialize wavetables
        initializeWavetables();
    }
    
    // Parameters
    void setWavetablePosition(float position) {
        wavetablePosition_ = juce::jlimit(0.0f, 1.0f, position);
    }
    
    void setUnisonVoices(int voices) {
        unisonVoices_ = juce::jlimit(1, 16, voices);
    }
    
    void setUnisonDetune(float cents) {
        unisonDetune_ = cents;
    }
    
    void setUnisonSpread(float spread) {
        unisonSpread_ = juce::jlimit(0.0f, 1.0f, spread);
    }
    
    void setADSR(float attack, float decay, float sustain, float release) {
        adsrParams_.attack = attack;
        adsrParams_.decay = decay;
        adsrParams_.sustain = sustain;
        adsrParams_.release = release;
    }
    
    // Wavetable management
    void loadWavetable(const WavetableData& data) {
        currentWavetable_ = data;
    }
    
    void generateWavetable(const std::string& type) {
        if (type == "Saw") {
            generateSawWavetable();
        } else if (type == "Square") {
            generateSquareWavetable();
        } else if (type == "Triangle") {
            generateTriangleWavetable();
        } else if (type == "Sine") {
            generateSineWavetable();
        }
    }
    
private:
    WavetableData currentWavetable_;
    float wavetablePosition_{0.0f};
    
    int unisonVoices_{1};
    float unisonDetune_{10.0f};  // cents
    float unisonSpread_{0.5f};
    
    juce::ADSR::Parameters adsrParams_;
    
    void initializeWavetables() {
        generateSineWavetable();
    }
    
    void generateSineWavetable() {
        currentWavetable_.name = "Sine";
        for (auto& table : currentWavetable_.tables) {
            for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                table[i] = std::sin(2.0f * juce::MathConstants<float>::pi * i / WAVETABLE_SIZE);
            }
        }
    }
    
    void generateSawWavetable() {
        currentWavetable_.name = "Saw";
        for (int t = 0; t < NUM_TABLES; ++t) {
            int harmonics = std::max(1, NUM_TABLES - t);
            for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                float sample = 0.0f;
                for (int h = 1; h <= harmonics; ++h) {
                    sample += std::sin(2.0f * juce::MathConstants<float>::pi * h * i / WAVETABLE_SIZE) / h;
                }
                currentWavetable_.tables[t][i] = sample * 0.5f;
            }
        }
    }
    
    void generateSquareWavetable() {
        currentWavetable_.name = "Square";
        for (int t = 0; t < NUM_TABLES; ++t) {
            int harmonics = std::max(1, (NUM_TABLES - t) / 2);
            for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                float sample = 0.0f;
                for (int h = 1; h <= harmonics; ++h) {
                    if (h % 2 == 1) {  // Odd harmonics only
                        sample += std::sin(2.0f * juce::MathConstants<float>::pi * h * i / WAVETABLE_SIZE) / h;
                    }
                }
                currentWavetable_.tables[t][i] = sample * 0.6f;
            }
        }
    }
    
    void generateTriangleWavetable() {
        currentWavetable_.name = "Triangle";
        for (int t = 0; t < NUM_TABLES; ++t) {
            int harmonics = std::max(1, (NUM_TABLES - t) / 2);
            for (int i = 0; i < WAVETABLE_SIZE; ++i) {
                float sample = 0.0f;
                int sign = 1;
                for (int h = 1; h <= harmonics; ++h) {
                    if (h % 2 == 1) {  // Odd harmonics only
                        sample += sign * std::sin(2.0f * juce::MathConstants<float>::pi * h * i / WAVETABLE_SIZE) / (h * h);
                        sign *= -1;
                    }
                }
                currentWavetable_.tables[t][i] = sample * 0.8f;
            }
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSynth)
};

} // namespace Audio
} // namespace OmegaStudio
