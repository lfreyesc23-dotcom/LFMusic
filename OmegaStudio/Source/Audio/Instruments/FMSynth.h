#pragma once

#include <JuceHeader.h>
#include <array>

namespace OmegaStudio {
namespace Audio {

/**
 * @brief FM Synthesizer (Frequency Modulation)
 * 6-operator FM synthesis like DX7
 */
class FMSynth : public juce::Synthesiser {
public:
    static constexpr int NUM_OPERATORS = 6;
    
    struct Operator {
        float ratio{1.0f};          // Frequency ratio
        float level{1.0f};          // Output level
        float feedback{0.0f};       // Self-modulation
        
        // ADSR
        juce::ADSR::Parameters adsr;
        
        // LFO
        bool lfoEnabled{false};
        float lfoRate{5.0f};
        float lfoDepth{0.0f};
        
        Operator() {
            adsr.attack = 0.01f;
            adsr.decay = 0.3f;
            adsr.sustain = 0.7f;
            adsr.release = 0.5f;
        }
    };
    
    enum class Algorithm {
        // Common FM algorithms
        Stack_6,        // All in series
        Parallel_6,     // All parallel
        DX7_1,          // Classic DX7 algorithm 1
        DX7_2,
        DX7_4,
        DX7_32,         // Most versatile
        Custom
    };
    
    class Voice : public juce::SynthesiserVoice {
    public:
        Voice(FMSynth& parent) : parent_(parent) {
            for (auto& adsr : adsrEnvelopes_) {
                adsr.setSampleRate(44100.0);
            }
        }
        
        bool canPlaySound(juce::SynthesiserSound*) override { return true; }
        
        void startNote(int midiNoteNumber, float velocity,
                      juce::SynthesiserSound*, int) override {
            baseFrequency_ = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
            velocity_ = velocity;
            
            // Reset operators
            for (int i = 0; i < NUM_OPERATORS; ++i) {
                phases_[i] = 0.0f;
                feedbackSample_[i] = 0.0f;
                
                adsrEnvelopes_[i].setSampleRate(getSampleRate());
                adsrEnvelopes_[i].setParameters(parent_.operators_[i].adsr);
                adsrEnvelopes_[i].noteOn();
            }
        }
        
        void stopNote(float, bool allowTailOff) override {
            if (allowTailOff) {
                for (auto& adsr : adsrEnvelopes_) {
                    adsr.noteOff();
                }
            } else {
                clearCurrentNote();
                for (auto& adsr : adsrEnvelopes_) {
                    adsr.reset();
                }
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
                float sample = processAlgorithm();
                sample *= velocity_;
                
                leftBuffer[i] += sample;
                if (rightBuffer) {
                    rightBuffer[i] += sample;
                }
                
                // Check if all envelopes are done
                bool allDone = true;
                for (const auto& adsr : adsrEnvelopes_) {
                    if (adsr.isActive()) {
                        allDone = false;
                        break;
                    }
                }
                
                if (allDone) {
                    clearCurrentNote();
                    break;
                }
            }
        }
        
    private:
        FMSynth& parent_;
        double baseFrequency_{440.0};
        float velocity_{1.0f};
        
        std::array<float, NUM_OPERATORS> phases_{};
        std::array<float, NUM_OPERATORS> feedbackSample_{};
        std::array<juce::ADSR, NUM_OPERATORS> adsrEnvelopes_;
        
        float processAlgorithm() {
            float output = 0.0f;
            
            switch (parent_.algorithm_) {
                case Algorithm::Stack_6:
                    output = processStack();
                    break;
                case Algorithm::Parallel_6:
                    output = processParallel();
                    break;
                case Algorithm::DX7_1:
                    output = processDX7Algorithm1();
                    break;
                case Algorithm::DX7_32:
                    output = processDX7Algorithm32();
                    break;
                default:
                    output = processParallel();
                    break;
            }
            
            return output;
        }
        
        float processStack() {
            // Op6 -> Op5 -> Op4 -> Op3 -> Op2 -> Op1 -> Out
            float mod = generateOperator(5, 0.0f);
            mod = generateOperator(4, mod);
            mod = generateOperator(3, mod);
            mod = generateOperator(2, mod);
            mod = generateOperator(1, mod);
            return generateOperator(0, mod);
        }
        
        float processParallel() {
            // All operators in parallel
            float output = 0.0f;
            for (int i = 0; i < NUM_OPERATORS; ++i) {
                output += generateOperator(i, 0.0f);
            }
            return output / NUM_OPERATORS;
        }
        
        float processDX7Algorithm1() {
            // Op1 = carrier, Op2 modulates Op1, Op3 modulates Op2
            // Op4, Op5, Op6 are separate carriers
            float mod3 = generateOperator(2, 0.0f);
            float mod2 = generateOperator(1, mod3);
            float carrier1 = generateOperator(0, mod2);
            
            float carrier2 = generateOperator(3, 0.0f);
            float carrier3 = generateOperator(4, 0.0f);
            float carrier4 = generateOperator(5, 0.0f);
            
            return (carrier1 + carrier2 + carrier3 + carrier4) * 0.25f;
        }
        
        float processDX7Algorithm32() {
            // Most versatile: (Op1+Op2) -> Op3, Op4 -> Op5, Op6 separate
            float mod1 = generateOperator(0, 0.0f);
            float mod2 = generateOperator(1, 0.0f);
            float carrier1 = generateOperator(2, (mod1 + mod2) * 0.5f);
            
            float mod4 = generateOperator(3, 0.0f);
            float carrier2 = generateOperator(4, mod4);
            
            float carrier3 = generateOperator(5, 0.0f);
            
            return (carrier1 + carrier2 + carrier3) / 3.0f;
        }
        
        float generateOperator(int opIndex, float modulation) {
            const auto& op = parent_.operators_[opIndex];
            
            // Frequency with ratio
            float frequency = baseFrequency_ * op.ratio;
            
            // Phase increment
            float phaseIncrement = (2.0f * juce::MathConstants<float>::pi * frequency) / getSampleRate();
            
            // Apply modulation and feedback
            float phase = phases_[opIndex] + modulation + (feedbackSample_[opIndex] * op.feedback);
            
            // Generate sine
            float sample = std::sin(phase);
            
            // Apply envelope
            float envelope = adsrEnvelopes_[opIndex].getNextSample();
            sample *= envelope * op.level;
            
            // Store for feedback
            feedbackSample_[opIndex] = sample;
            
            // Advance phase
            phases_[opIndex] += phaseIncrement;
            if (phases_[opIndex] >= 2.0f * juce::MathConstants<float>::pi) {
                phases_[opIndex] -= 2.0f * juce::MathConstants<float>::pi;
            }
            
            return sample;
        }
    };
    
    FMSynth() {
        for (int i = 0; i < 16; ++i) {
            addVoice(new Voice(*this));
        }
        addSound(new juce::SynthesiserSound());
        
        // Initialize with basic algorithm
        setAlgorithm(Algorithm::DX7_1);
    }
    
    void setAlgorithm(Algorithm algo) {
        algorithm_ = algo;
    }
    
    void setOperatorRatio(int opIndex, float ratio) {
        if (opIndex >= 0 && opIndex < NUM_OPERATORS) {
            operators_[opIndex].ratio = ratio;
        }
    }
    
    void setOperatorLevel(int opIndex, float level) {
        if (opIndex >= 0 && opIndex < NUM_OPERATORS) {
            operators_[opIndex].level = juce::jlimit(0.0f, 1.0f, level);
        }
    }
    
    void setOperatorFeedback(int opIndex, float feedback) {
        if (opIndex >= 0 && opIndex < NUM_OPERATORS) {
            operators_[opIndex].feedback = juce::jlimit(0.0f, 1.0f, feedback);
        }
    }
    
    void setOperatorADSR(int opIndex, float attack, float decay, float sustain, float release) {
        if (opIndex >= 0 && opIndex < NUM_OPERATORS) {
            operators_[opIndex].adsr.attack = attack;
            operators_[opIndex].adsr.decay = decay;
            operators_[opIndex].adsr.sustain = sustain;
            operators_[opIndex].adsr.release = release;
        }
    }
    
    Operator& getOperator(int index) { return operators_[index]; }
    
private:
    std::array<Operator, NUM_OPERATORS> operators_;
    Algorithm algorithm_{Algorithm::DX7_1};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynth)
};

} // namespace Audio
} // namespace OmegaStudio
