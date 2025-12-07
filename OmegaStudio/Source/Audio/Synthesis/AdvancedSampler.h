#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <map>

namespace OmegaStudio {

/**
 * @brief Professional Advanced Sampler with Multi-Layer Support
 * 
 * Features:
 * - Multi-sample mapping with velocity layers
 * - Loop modes: Forward, Reverse, Ping-pong, One-shot
 * - Time-stretching and pitch-shifting
 * - Multi-filter with modulation
 * - ADSR envelopes for Amp, Filter, Pitch
 * - Sample start/end offset with modulation
 * - Cross-fade looping
 * - Round-robin sample rotation
 */
class AdvancedSampler : public juce::Synthesiser {
public:
    //==============================================================================
    enum class LoopMode {
        None,
        Forward,
        Reverse,
        PingPong,
        OneShot
    };
    
    enum class PlayMode {
        Normal,
        RoundRobin,
        Random,
        VelocitySwitch
    };
    
    //==============================================================================
    struct Sample {
        juce::String name;
        juce::AudioBuffer<float> buffer;
        double sampleRate = 44100.0;
        
        // Mapping
        int rootNote = 60;        // C4
        int keyLow = 0;           // C-1
        int keyHigh = 127;        // G9
        int velLow = 0;
        int velHigh = 127;
        
        // Loop points
        LoopMode loopMode = LoopMode::None;
        int loopStart = 0;
        int loopEnd = -1;         // -1 = end of sample
        float crossfadeLength = 0.01f; // seconds
        
        // Tuning
        int transpose = 0;        // semitones
        int fineTune = 0;         // cents
        
        bool loaded = false;
    };
    
    //==============================================================================
    struct Layer {
        juce::String name;
        std::vector<std::shared_ptr<Sample>> samples;
        PlayMode playMode = PlayMode::Normal;
        int roundRobinIndex = 0;
        
        // Layer parameters
        float volume = 1.0f;
        float pan = 0.0f;
        int transpose = 0;
        int fineTune = 0;
        
        // Filter
        bool filterEnabled = false;
        float filterCutoff = 10000.0f;
        float filterResonance = 0.0f;
        
        // Envelope
        struct Envelope {
            float attack = 0.001f;
            float decay = 0.1f;
            float sustain = 1.0f;
            float release = 0.1f;
        } ampEnv, filterEnv, pitchEnv;
    };
    
    //==============================================================================
    struct SamplerParams {
        std::vector<Layer> layers;
        
        // Master
        float masterVolume = 0.8f;
        int maxVoices = 64;
        
        // Time-stretching
        bool timeStretchEnabled = false;
        float timeStretchRatio = 1.0f; // 0.5-2.0
        
        // Pitch
        bool formantPreserve = false;
        
        // Effects
        bool reverbEnabled = false;
        float reverbMix = 0.2f;
    };
    
    //==============================================================================
    AdvancedSampler();
    ~AdvancedSampler() override;
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        const juce::MidiBuffer& midiMessages,
                        int startSample, int numSamples);
    
    // Sample management
    bool loadSample(const juce::File& file, int layerIndex = 0);
    void addLayer(const Layer& layer);
    void clearAllSamples();
    
    // Parameters
    void setParameters(const SamplerParams& params);
    SamplerParams& getParameters() { return params; }
    
private:
    SamplerParams params;
    juce::dsp::ProcessSpec currentSpec;
    
    class SamplerVoice;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedSampler)
};

} // namespace OmegaStudio
