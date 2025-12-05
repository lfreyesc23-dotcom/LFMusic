#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace Audio {
namespace Effects {

/**
 * @brief Flanger Effect
 */
class Flanger {
public:
    Flanger() {
        delayBuffer_.setSize(2, 44100);  // 1 second max delay
    }
    
    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        delayBuffer_.setSize(2, static_cast<int>(sampleRate_ * 0.05));  // 50ms max
        delayBuffer_.clear();
        writePos_ = 0;
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int channel = 0; channel < numChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayData = delayBuffer_.getWritePointer(channel);
            const int delayBufferSize = delayBuffer_.getNumSamples();
            
            for (int i = 0; i < numSamples; ++i) {
                // LFO modulation
                float lfo = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase_);
                float delayTime = (depth_ * 0.5f * (lfo + 1.0f) + minDelay_) * 0.001f * sampleRate_;
                
                int delayInSamples = static_cast<int>(delayTime);
                float frac = delayTime - delayInSamples;
                
                // Read from delay buffer with interpolation
                int readPos1 = (writePos_ - delayInSamples + delayBufferSize) % delayBufferSize;
                int readPos2 = (readPos1 + 1) % delayBufferSize;
                
                float delayed = delayData[readPos1] + frac * (delayData[readPos2] - delayData[readPos1]);
                
                // Mix
                float input = channelData[i];
                float output = input + delayed * feedback_;
                channelData[i] = input * (1.0f - mix_) + output * mix_;
                
                // Write to delay buffer
                delayData[writePos_] = input + delayed * feedback_;
                
                // Advance write position
                writePos_ = (writePos_ + 1) % delayBufferSize;
                
                // Advance LFO
                lfoPhase_ += rate_ / sampleRate_;
                if (lfoPhase_ >= 1.0f) {
                    lfoPhase_ -= 1.0f;
                }
            }
        }
    }
    
    void setRate(float hz) { rate_ = hz; }
    void setDepth(float ms) { depth_ = ms; }
    void setFeedback(float amount) { feedback_ = juce::jlimit(-0.95f, 0.95f, amount); }
    void setMix(float amount) { mix_ = juce::jlimit(0.0f, 1.0f, amount); }
    void setMinDelay(float ms) { minDelay_ = ms; }
    
private:
    juce::AudioBuffer<float> delayBuffer_;
    int writePos_{0};
    double sampleRate_{44100.0};
    
    float rate_{0.5f};        // Hz
    float depth_{5.0f};       // ms
    float feedback_{0.3f};
    float mix_{0.5f};
    float minDelay_{1.0f};    // ms
    float lfoPhase_{0.0f};
};

/**
 * @brief Phaser Effect
 */
class Phaser {
public:
    static constexpr int NUM_STAGES = 6;
    
    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        for (auto& filter : filters_) {
            filter.reset();
        }
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int channel = 0; channel < numChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            
            for (int i = 0; i < numSamples; ++i) {
                // LFO modulation
                float lfo = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase_);
                float frequency = centerFreq_ + depth_ * lfo * centerFreq_;
                
                // Process through allpass filters
                float sample = channelData[i];
                float filtered = sample;
                
                for (int stage = 0; stage < NUM_STAGES; ++stage) {
                    filtered = processAllpass(filtered, frequency * std::pow(2.0f, stage * 0.5f), stage);
                }
                
                // Mix with feedback
                channelData[i] = sample + filtered * feedback_ * mix_;
                
                // Advance LFO
                lfoPhase_ += rate_ / sampleRate_;
                if (lfoPhase_ >= 1.0f) {
                    lfoPhase_ -= 1.0f;
                }
            }
        }
    }
    
    void setRate(float hz) { rate_ = hz; }
    void setDepth(float amount) { depth_ = juce::jlimit(0.0f, 1.0f, amount); }
    void setFeedback(float amount) { feedback_ = juce::jlimit(-0.95f, 0.95f, amount); }
    void setCenterFrequency(float hz) { centerFreq_ = hz; }
    void setMix(float amount) { mix_ = juce::jlimit(0.0f, 1.0f, amount); }
    
private:
    double sampleRate_{44100.0};
    float rate_{0.5f};
    float depth_{0.5f};
    float feedback_{0.5f};
    float centerFreq_{1000.0f};
    float mix_{0.5f};
    float lfoPhase_{0.0f};
    
    struct AllpassFilter {
        float z1{0.0f};
        
        void reset() { z1 = 0.0f; }
    };
    
    std::array<AllpassFilter, NUM_STAGES> filters_;
    
    float processAllpass(float input, float frequency, int stage) {
        float a = (std::tan(juce::MathConstants<float>::pi * frequency / sampleRate_) - 1.0f) /
                 (std::tan(juce::MathConstants<float>::pi * frequency / sampleRate_) + 1.0f);
        
        float output = -input + a * (input - filters_[stage].z1);
        filters_[stage].z1 = input + a * output;
        
        return output;
    }
};

/**
 * @brief Chorus Effect
 */
class Chorus {
public:
    Chorus() {
        delayBuffer_.setSize(2, 44100);
    }
    
    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        delayBuffer_.setSize(2, static_cast<int>(sampleRate_ * 0.1));  // 100ms max
        delayBuffer_.clear();
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int channel = 0; channel < numChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayData = delayBuffer_.getWritePointer(channel);
            const int delayBufferSize = delayBuffer_.getNumSamples();
            
            for (int i = 0; i < numSamples; ++i) {
                float input = channelData[i];
                float output = input;
                
                // Multiple delay taps for richer chorus
                for (int voice = 0; voice < numVoices_; ++voice) {
                    float voicePhase = lfoPhase_ + (voice / static_cast<float>(numVoices_));
                    if (voicePhase >= 1.0f) voicePhase -= 1.0f;
                    
                    float lfo = std::sin(2.0f * juce::MathConstants<float>::pi * voicePhase);
                    float delayTime = (baseDelay_ + depth_ * lfo) * 0.001f * sampleRate_;
                    
                    int delayInSamples = static_cast<int>(delayTime);
                    float frac = delayTime - delayInSamples;
                    
                    int readPos1 = (writePos_ - delayInSamples + delayBufferSize) % delayBufferSize;
                    int readPos2 = (readPos1 + 1) % delayBufferSize;
                    
                    float delayed = delayData[readPos1] + frac * (delayData[readPos2] - delayData[readPos1]);
                    output += delayed / numVoices_;
                }
                
                channelData[i] = input * (1.0f - mix_) + output * mix_;
                
                delayData[writePos_] = input;
                writePos_ = (writePos_ + 1) % delayBufferSize;
                
                lfoPhase_ += rate_ / sampleRate_;
                if (lfoPhase_ >= 1.0f) {
                    lfoPhase_ -= 1.0f;
                }
            }
        }
    }
    
    void setRate(float hz) { rate_ = hz; }
    void setDepth(float ms) { depth_ = ms; }
    void setMix(float amount) { mix_ = juce::jlimit(0.0f, 1.0f, amount); }
    void setNumVoices(int voices) { numVoices_ = juce::jlimit(1, 8, voices); }
    void setBaseDelay(float ms) { baseDelay_ = ms; }
    
private:
    juce::AudioBuffer<float> delayBuffer_;
    int writePos_{0};
    double sampleRate_{44100.0};
    
    float rate_{0.5f};
    float depth_{10.0f};
    float mix_{0.5f};
    float baseDelay_{20.0f};
    int numVoices_{3};
    float lfoPhase_{0.0f};
};

/**
 * @brief Vocoder Effect
 */
class Vocoder {
public:
    static constexpr int NUM_BANDS = 16;
    
    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        
        // Initialize band filters
        float minFreq = 100.0f;
        float maxFreq = 8000.0f;
        float ratio = std::pow(maxFreq / minFreq, 1.0f / NUM_BANDS);
        
        for (int i = 0; i < NUM_BANDS; ++i) {
            float freq = minFreq * std::pow(ratio, i);
            bandFilters_[i].setFrequency(freq);
            bandFilters_[i].setQ(5.0f);
            bandFilters_[i].prepare(sampleRate);
        }
    }
    
    void processCarrierModulator(juce::AudioBuffer<float>& carrier,
                                 const juce::AudioBuffer<float>& modulator) {
        const int numSamples = carrier.getNumSamples();
        
        juce::AudioBuffer<float> output;
        output.setSize(carrier.getNumChannels(), numSamples);
        output.clear();
        
        for (int band = 0; band < NUM_BANDS; ++band) {
            // Filter carrier
            juce::AudioBuffer<float> carrierBand(carrier);
            bandFilters_[band].process(carrierBand);
            
            // Filter modulator
            juce::AudioBuffer<float> modulatorBand(modulator);
            bandFilters_[band].process(modulatorBand);
            
            // Extract envelope from modulator
            for (int channel = 0; channel < output.getNumChannels(); ++channel) {
                auto* carrierData = carrierBand.getWritePointer(channel);
                auto* modulatorData = modulatorBand.getReadPointer(channel % modulator.getNumChannels());
                auto* outputData = output.getWritePointer(channel);
                
                for (int i = 0; i < numSamples; ++i) {
                    // Envelope follower
                    float modEnv = std::abs(modulatorData[i]);
                    envelopeStates_[band] = envelopeStates_[band] * 0.99f + modEnv * 0.01f;
                    
                    // Apply envelope to carrier
                    outputData[i] += carrierData[i] * envelopeStates_[band];
                }
            }
        }
        
        // Copy output back to carrier
        carrier.makeCopyOf(output);
    }
    
private:
    double sampleRate_{44100.0};
    
    struct BandFilter {
        juce::dsp::IIR::Filter<float> filter;
        
        void setFrequency(float freq) { centerFreq_ = freq; }
        void setQ(float q) { q_ = q; }
        
        void prepare(double sampleRate) {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
                sampleRate, centerFreq_, q_
            );
            filter.coefficients = coeffs;
        }
        
        void process(juce::AudioBuffer<float>& buffer) {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            filter.process(context);
        }
        
    private:
        float centerFreq_{1000.0f};
        float q_{5.0f};
    };
    
    std::array<BandFilter, NUM_BANDS> bandFilters_;
    std::array<float, NUM_BANDS> envelopeStates_{};
};

/**
 * @brief Stutter/Glitch Effect
 */
class StutterEffect {
public:
    void prepare(double sampleRate, int samplesPerBlock) {
        sampleRate_ = sampleRate;
        stutterBuffer_.setSize(2, static_cast<int>(sampleRate_ * 2.0));  // 2 second buffer
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        if (!isStuttering_) {
            // Normal passthrough, but record to buffer
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
                stutterBuffer_.copyFrom(channel, writePos_, buffer, channel, 0, buffer.getNumSamples());
            }
            writePos_ = (writePos_ + buffer.getNumSamples()) % stutterBuffer_.getNumSamples();
            return;
        }
        
        // Stutter effect active
        const int numSamples = buffer.getNumSamples();
        const int stutterLength = static_cast<int>(stutterLengthBeats_ * (60.0 / tempo_) * sampleRate_);
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            const auto* stutterData = stutterBuffer_.getReadPointer(channel);
            
            for (int i = 0; i < numSamples; ++i) {
                int readPos = (stutterStartPos_ + (stutterPos_ % stutterLength)) % stutterBuffer_.getNumSamples();
                channelData[i] = stutterData[readPos];
                stutterPos_++;
            }
        }
    }
    
    void trigger(float lengthInBeats = 0.125f) {
        isStuttering_ = true;
        stutterLengthBeats_ = lengthInBeats;
        stutterStartPos_ = (writePos_ - static_cast<int>(lengthInBeats * (60.0 / tempo_) * sampleRate_) + 
                           stutterBuffer_.getNumSamples()) % stutterBuffer_.getNumSamples();
        stutterPos_ = 0;
    }
    
    void stop() {
        isStuttering_ = false;
    }
    
    void setTempo(double bpm) { tempo_ = bpm; }
    
private:
    juce::AudioBuffer<float> stutterBuffer_;
    double sampleRate_{44100.0};
    double tempo_{120.0};
    
    int writePos_{0};
    int stutterStartPos_{0};
    int stutterPos_{0};
    float stutterLengthBeats_{0.125f};
    bool isStuttering_{false};
};

} // namespace Effects
} // namespace Audio
} // namespace OmegaStudio
