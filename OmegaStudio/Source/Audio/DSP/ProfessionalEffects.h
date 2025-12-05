/*
  ==============================================================================
    ProfessionalEffects.h
    
    Efectos profesionales built-in:
    - Reverb (algorithmic + convolution)
    - Delay (ping-pong, tape, stereo)
    - Saturation/Distortion
    - Limiter (mastering grade)
    - Multiband Compressor
    - Transient Shaper
    - Stereo Enhancer
    
    Professional audio effects
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

namespace OmegaStudio {

//==============================================================================
/** Reverb Profesional (Algorithmic) */
class ProReverbEffect {
public:
    ProReverbEffect();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setRoomSize(float size);         // 0-1
    void setDamping(float damping);       // 0-1
    void setWetLevel(float level);        // 0-1
    void setDryLevel(float level);        // 0-1
    void setWidth(float width);           // 0-1
    void setPreDelay(float ms);           // 0-500ms
    
    float getRoomSize() const { return roomSize; }
    float getDamping() const { return damping; }
    float getWetLevel() const { return wetLevel; }
    float getDryLevel() const { return dryLevel; }
    float getWidth() const { return width; }
    float getPreDelay() const { return preDelayMs; }
    
private:
    juce::Reverb reverb;
    juce::Reverb::Parameters params;
    
    float roomSize { 0.5f };
    float damping { 0.5f };
    float wetLevel { 0.33f };
    float dryLevel { 0.4f };
    float width { 1.0f };
    float preDelayMs { 0.0f };
    
    juce::dsp::DelayLine<float> preDelayLine;
    double sampleRate { 48000.0 };
    
    void updateParameters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProReverbEffect)
};

//==============================================================================
/** Delay Profesional */
class ProDelayEffect {
public:
    enum class Type {
        Stereo,
        PingPong,
        Tape,
        Ducking
    };
    
    ProDelayEffect();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setDelayType(Type type) { delayType = type; }
    Type getDelayType() const { return delayType; }
    
    void setDelayTime(float ms);          // 1-2000ms
    void setFeedback(float feedback);     // 0-0.95
    void setMix(float mix);               // 0-1
    void setSyncToTempo(bool sync);
    void setTempoSync(float beats);       // 1/4, 1/8, etc.
    
    // Tape simulation
    void setWow(float amount);            // 0-1
    void setFlutter(float amount);        // 0-1
    void setSaturation(float amount);     // 0-1
    
private:
    Type delayType { Type::Stereo };
    
    juce::dsp::DelayLine<float> delayLineLeft { 96000 };
    juce::dsp::DelayLine<float> delayLineRight { 96000 };
    
    float delayTimeMs { 250.0f };
    float feedback { 0.4f };
    float mix { 0.3f };
    bool tempoSync { false };
    float tempoSyncBeats { 0.25f };
    
    // Tape effects
    float wow { 0.0f };
    float flutter { 0.0f };
    float saturation { 0.0f };
    
    double sampleRate { 48000.0 };
    float phase { 0.0f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProDelayEffect)
};

//==============================================================================
/** Saturación/Distorsión */
class SaturationEffect {
public:
    enum class Type {
        Soft,           // Soft clipping
        Hard,           // Hard clipping
        Tape,           // Tape saturation
        Tube,           // Tube warmth
        Transistor,     // Transistor distortion
        Bitcrusher      // Digital distortion
    };
    
    SaturationEffect();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setSaturationType(Type type) { saturationType = type; }
    Type getSaturationType() const { return saturationType; }
    
    void setDrive(float drive);           // 0-1 (0-24dB)
    void setMix(float mix);               // 0-1
    void setBias(float bias);             // -1 to +1 (asymmetry)
    
    // Bitcrusher specific
    void setBitDepth(int bits);           // 1-16
    void setSampleRateReduction(float factor); // 1-100
    
private:
    Type saturationType { Type::Soft };
    
    float drive { 0.5f };
    float mix { 1.0f };
    float bias { 0.0f };
    
    int bitDepth { 16 };
    float sampleRateReduction { 1.0f };
    
    float processSample(float sample);
    float softClip(float sample);
    float hardClip(float sample);
    float tapeSaturation(float sample);
    float tubeSaturation(float sample);
    float transistorDistortion(float sample);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationEffect)
};

//==============================================================================
/** Limiter (Mastering Grade) */
class MasteringLimiter {
public:
    MasteringLimiter();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Parameters
    void setThreshold(float thresholdDb);  // -20 to 0 dB
    void setRelease(float releaseMs);      // 1-1000ms
    void setCeiling(float ceilingDb);      // -1 to 0 dB
    void setLookahead(float lookaheadMs);  // 0-10ms
    
    // Metering
    float getGainReduction() const { return gainReductionDb; }
    
private:
    float thresholdDb { -0.1f };
    float releaseMs { 100.0f };
    float ceilingDb { -0.1f };
    float lookaheadMs { 5.0f };
    
    double sampleRate { 48000.0 };
    float gainReductionDb { 0.0f };
    float envelope { 0.0f };
    
    juce::dsp::DelayLine<float> lookaheadDelayLeft { 960 };
    juce::dsp::DelayLine<float> lookaheadDelayRight { 960 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringLimiter)
};

//==============================================================================
/** Multiband Compressor */
class MultibandCompressor {
public:
    MultibandCompressor();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    
    // Band parameters (Low, Mid, High)
    struct BandSettings {
        bool enabled { true };
        float threshold { -20.0f };     // dB
        float ratio { 4.0f };           // 1:1 to 20:1
        float attack { 10.0f };         // ms
        float release { 100.0f };       // ms
        float gain { 0.0f };            // makeup gain dB
    };
    
    void setBandSettings(int bandIndex, const BandSettings& settings);
    BandSettings getBandSettings(int bandIndex) const;
    
    // Crossover frequencies
    void setLowMidCrossover(float freq);    // 200-800 Hz
    void setMidHighCrossover(float freq);   // 2k-8k Hz
    
private:
    static constexpr int numBands = 3;
    std::array<BandSettings, numBands> bandSettings;
    
    float lowMidCrossover { 400.0f };
    float midHighCrossover { 4000.0f };
    
    // Filters (Linkwitz-Riley 4th order)
    juce::dsp::IIR::Filter<float> lowPassLeft, lowPassRight;
    juce::dsp::IIR::Filter<float> bandPassLeft, bandPassRight;
    juce::dsp::IIR::Filter<float> highPassLeft, highPassRight;
    
    double sampleRate { 48000.0 };
    
    void updateFilters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultibandCompressor)
};

//==============================================================================
/** Transient Shaper */
class TransientShaper {
public:
    TransientShaper();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Parameters
    void setAttack(float amount);      // -100% to +100%
    void setSustain(float amount);     // -100% to +100%
    void setSmooth(float smooth);      // 0-1
    
private:
    float attackAmount { 0.0f };
    float sustainAmount { 0.0f };
    float smooth { 0.5f };
    
    double sampleRate { 48000.0 };
    
    // Envelope followers
    float attackEnvelope { 0.0f };
    float sustainEnvelope { 0.0f };
    float previousSample { 0.0f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientShaper)
};

//==============================================================================
/** Stereo Enhancer */
class StereoEnhancer {
public:
    StereoEnhancer();
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setWidth(float width);        // 0-2 (0=mono, 1=normal, 2=wide)
    void setMonoBass(bool mono);       // Mono low frequencies
    void setMonoFreq(float freq);      // Frequency for mono bass (20-200 Hz)
    
private:
    float width { 1.0f };
    bool monoBass { true };
    float monoFreq { 120.0f };
    
    double sampleRate { 48000.0 };
    
    // Filters for mono bass
    juce::dsp::IIR::Filter<float> lowPassLeft, lowPassRight;
    juce::dsp::IIR::Filter<float> highPassLeft, highPassRight;
    
    void updateFilters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoEnhancer)
};

} // namespace OmegaStudio
