#pragma once
#include <JuceHeader.h>
#include <array>

namespace OmegaStudio {

/**
 * @brief Professional Effects Suite: Delay, Reverb, Flanger, Phaser, Chorus, Vocoder
 */

//==============================================================================
// DELAY CREATIVO
//==============================================================================
class CreativeDelay {
public:
    struct Params {
        float delayTimeL = 0.25f;    // seconds or tempo-synced
        float delayTimeR = 0.5f;
        float feedback = 0.3f;
        float mix = 0.3f;
        bool tempoSync = true;
        float syncRateL = 0.25f;     // 1/16 = 0.0625, 1/4 = 0.25
        float syncRateR = 0.5f;
        
        // Modulation
        float modRate = 0.5f;
        float modDepth = 0.0f;
        
        // Filtering
        float lowCut = 200.0f;
        float highCut = 8000.0f;
        
        // Stereo
        float stereoWidth = 1.0f;
        bool pingPong = false;
    };
    
    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer, const Params& params, double bpm);
    void reset();
    
private:
    juce::dsp::DelayLine<float> delayLineL{48000 * 4};
    juce::dsp::DelayLine<float> delayLineR{48000 * 4};
    double sampleRate = 44100.0;
};

//==============================================================================
// REVERB ALGORÍTMICA
//==============================================================================
class AlgorithmicReverb {
public:
    struct Params {
        float roomSize = 0.7f;       // 0-1
        float damping = 0.5f;        // High frequency absorption
        float width = 1.0f;          // Stereo width
        float preDelay = 0.02f;      // seconds
        float earlyReflections = 0.3f;
        float diffusion = 0.8f;
        float density = 0.8f;
        float mix = 0.25f;
        
        // EQ
        float lowShelfFreq = 200.0f;
        float lowShelfGain = 0.0f;   // dB
        float highShelfFreq = 8000.0f;
        float highShelfGain = -3.0f;
    };
    
    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer, const Params& params);
    void reset();
    
private:
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> dryBuffer;
};

//==============================================================================
// FLANGER
//==============================================================================
class Flanger {
public:
    struct Params {
        float rate = 0.5f;           // LFO rate Hz
        float depth = 0.7f;          // 0-1
        float feedback = 0.3f;       // -1 to 1
        float mix = 0.5f;
        float delay = 0.005f;        // Base delay in seconds
        bool stereo = true;          // Stereo phase offset
    };
    
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer, const Params& params);
    void reset();
    
private:
    juce::dsp::DelayLine<float> delayLine{48000};
    float lfoPhase = 0.0f;
    double sampleRate = 44100.0;
};

//==============================================================================
// PHASER
//==============================================================================
class Phaser {
public:
    struct Params {
        float rate = 0.5f;           // LFO rate Hz
        float depth = 0.7f;
        float feedback = 0.5f;
        float mix = 0.5f;
        int stages = 4;              // 2, 4, 6, 8, 12
        float centerFreq = 1000.0f;  // Hz
    };
    
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer, const Params& params);
    void reset();
    
private:
    static constexpr int MAX_STAGES = 12;
    struct AllPassFilter {
        float a1 = 0.0f, zm1 = 0.0f;
    };
    std::array<AllPassFilter, MAX_STAGES> filters;
    float lfoPhase = 0.0f;
    double sampleRate = 44100.0;
};

//==============================================================================
// CHORUS
//==============================================================================
class AdvancedChorus {
public:
    struct Params {
        float rate = 0.8f;           // LFO rate Hz
        float depth = 0.3f;
        float mix = 0.5f;
        int voices = 3;              // 1-8
        float voiceSpread = 0.5f;    // Stereo spread per voice
        float feedback = 0.0f;
    };
    
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer, const Params& params);
    void reset();
    
private:
    static constexpr int MAX_VOICES = 8;
    std::array<juce::dsp::DelayLine<float>, MAX_VOICES> delayLines;
    std::array<float, MAX_VOICES> lfoPhases;
    double sampleRate = 44100.0;
};

//==============================================================================
// VOCODER
//==============================================================================
class Vocoder {
public:
    struct Params {
        int bands = 16;              // 8, 16, 32
        float lowFreq = 100.0f;
        float highFreq = 8000.0f;
        float bandWidth = 1.0f;      // Q factor
        float attack = 0.01f;
        float release = 0.1f;
        float mix = 1.0f;
        bool formantShift = false;
        float shiftAmount = 0.0f;    // semitones
    };
    
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& modulator,    // Voice input
                juce::AudioBuffer<float>& carrier,       // Synth input
                juce:: AudioBuffer<float>& output,
                const Params& params);
    void reset();
    
private:
    static constexpr int MAX_BANDS = 32;
    
    struct Band {
        juce::dsp::IIR::Filter<float> modulatorFilter;
        juce::dsp::IIR::Filter<float> carrierFilter;
        float envelope = 0.0f;
    };
    
    std::array<Band, MAX_BANDS> bands;
    double sampleRate = 44100.0;
    
    void calculateBandFrequencies(const Params& params);
};

//==============================================================================
// DISTORTION SUITE
//==============================================================================
class DistortionSuite {
public:
    enum class Type {
        SoftClip,
        HardClip,
        Tube,
        Foldback,
        Bitcrush,
        Waveshaper
    };
    
    struct Params {
        Type type = Type::SoftClip;
        float drive = 1.0f;          // 1-20x
        float mix = 1.0f;
        float tone = 0.5f;           // Pre-filter
        float outputGain = 1.0f;
        
        // Bitcrusher specific
        int bitDepth = 16;           // 1-16
        float sampleRateReduction = 1.0f; // 1.0 = no reduction
    };
    
    void prepare(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer, const Params& params);
    
private:
    float processSample(float input, const Params& params);
    float softClip(float x);
    float hardClip(float x);
    float tubeDistortion(float x);
    float foldback(float x);
    
    double sampleRate = 44100.0;
    float lastSample = 0.0f;
};

} // namespace OmegaStudio
