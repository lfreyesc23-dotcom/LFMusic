/*
  ==============================================================================
    Instruments.h
    
    Built-in professional instruments:
    - ProSampler: Multi-sample playback con loop, pitch, filtros
    - ProSynth: Synth con osciladores, filtros, envolventes, LFOs
    - DrumMachine: 16 pads con samples y síntesis
    
    Enterprise-grade software instruments
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>

namespace OmegaStudio {

//==============================================================================
/** ProSampler - Sampler profesional multi-sample */
class ProSampler : public juce::Synthesiser {
public:
    ProSampler();
    ~ProSampler() override;
    
    // Sample loading
    bool loadSample(const juce::File& file, int rootNote = 60);
    bool loadMultiSamples(const std::vector<juce::File>& files);
    void clearAllSamples();
    
    // Playback parameters
    void setLoopMode(bool enabled);
    bool getLoopMode() const { return loopEnabled; }
    
    void setLoopStart(double beats);
    void setLoopEnd(double beats);
    
    // ADSR
    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);
    void setRelease(float seconds);
    
    // Filter
    void setFilterCutoff(float frequency);
    void setFilterResonance(float q);
    void setFilterType(int type);  // 0=LP, 1=HP, 2=BP
    
    // Pitch
    void setPitchBend(int cents);  // -100 to +100
    void setFineTune(int cents);
    
    // Velocity
    void setVelocitySensitivity(float amount);  // 0-1
    
    // Stats
    int getSampleCount() const { return getNumSounds(); }
    
private:
    bool loopEnabled { false };
    double loopStart { 0.0 };
    double loopEnd { 1.0 };
    
    juce::ADSR::Parameters adsrParams;
    
    float filterCutoff { 20000.0f };
    float filterResonance { 0.7f };
    int filterType { 0 };
    
    int pitchBend { 0 };
    int fineTune { 0 };
    float velocitySensitivity { 0.8f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSampler)
};

//==============================================================================
/** ProSynth - Synthesizer profesional */
class ProSynth : public juce::AudioProcessor {
public:
    ProSynth();
    ~ProSynth() override;
    
    // Oscillators
    enum class OscillatorType {
        Sine,
        Saw,
        Square,
        Triangle,
        Noise
    };
    
    void setOscillator1Type(OscillatorType type);
    void setOscillator2Type(OscillatorType type);
    void setOscillator1Level(float level);
    void setOscillator2Level(float level);
    void setOscillator2Detune(float cents);
    void setOscillatorMix(float balance);  // 0=osc1, 1=osc2
    
    // Filter
    void setFilterCutoff(float frequency);
    void setFilterResonance(float q);
    void setFilterEnvelopeAmount(float amount);
    void setFilterType(int type);
    
    // Envelopes
    void setAmpAttack(float seconds);
    void setAmpDecay(float seconds);
    void setAmpSustain(float level);
    void setAmpRelease(float seconds);
    
    void setFilterAttack(float seconds);
    void setFilterDecay(float seconds);
    void setFilterSustain(float level);
    void setFilterRelease(float seconds);
    
    // LFO
    void setLFORate(float hz);
    void setLFODepth(float amount);
    void setLFOTarget(int target);  // 0=pitch, 1=filter, 2=amp
    
    // Unison
    void setUnisonVoices(int numVoices);
    void setUnisonDetune(float cents);
    
    // AudioProcessor interface
    const juce::String getName() const override { return "ProSynth"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    
private:
    // Voice state
    struct Voice {
        bool active { false };
        int noteNumber { 0 };
        float frequency { 440.0f };
        float velocity { 1.0f };
        
        // Oscillator phases
        float phase1 { 0.0f };
        float phase2 { 0.0f };
        
        // Envelopes
        juce::ADSR ampEnvelope;
        juce::ADSR filterEnvelope;
    };
    
    std::vector<Voice> voices;
    int maxVoices { 16 };
    
    // Parameters
    OscillatorType osc1Type { OscillatorType::Saw };
    OscillatorType osc2Type { OscillatorType::Saw };
    float osc1Level { 1.0f };
    float osc2Level { 0.0f };
    float osc2Detune { 0.0f };
    float oscMix { 0.5f };
    
    float filterCutoff { 1000.0f };
    float filterResonance { 0.7f };
    float filterEnvAmount { 0.5f };
    int filterType { 0 };
    
    juce::ADSR::Parameters ampADSR;
    juce::ADSR::Parameters filterADSR;
    
    float lfoRate { 5.0f };
    float lfoDepth { 0.0f };
    int lfoTarget { 0 };
    float lfoPhase { 0.0f };
    
    int unisonVoices { 1 };
    float unisonDetune { 10.0f };
    
    double sampleRate { 44100.0 };
    
    // Processing
    Voice* findFreeVoice();
    float generateOscillator(OscillatorType type, float phase);
    void processVoice(Voice& voice, juce::AudioBuffer<float>& buffer, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynth)
};

//==============================================================================
/** DrumMachine - Drum machine con 16 pads */
class DrumMachine : public juce::AudioProcessor {
public:
    DrumMachine();
    ~DrumMachine() override;
    
    // Pad management (0-15)
    bool loadSampleToPad(int padIndex, const juce::File& file);
    void clearPad(int padIndex);
    void clearAllPads();
    
    // Pad parameters
    void setPadVolume(int padIndex, float volume);
    void setPadPan(int padIndex, float pan);
    void setPadPitch(int padIndex, float semitones);
    void setPadAttack(int padIndex, float ms);
    void setPadRelease(int padIndex, float ms);
    
    // Pad synthesis (drum synth mode)
    void setPadSynthMode(int padIndex, bool enabled);
    void setPadSynthFrequency(int padIndex, float hz);
    void setPadSynthDecay(int padIndex, float ms);
    void setPadSynthNoise(int padIndex, float amount);
    
    // Pattern sequencer
    void setStepActive(int padIndex, int step, bool active);
    bool isStepActive(int padIndex, int step) const;
    void clearPattern();
    
    void setPatternLength(int steps);
    int getPatternLength() const { return patternLength; }
    
    void setTempo(double bpm);
    double getTempo() const { return tempo; }
    
    void setSwing(float amount);  // 0-1
    
    // Playback
    void startPattern();
    void stopPattern();
    bool isPlaying() const { return playing; }
    
    // AudioProcessor interface
    const juce::String getName() const override { return "DrumMachine"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    
private:
    static constexpr int numPads = 16;
    static constexpr int maxSteps = 64;
    
    struct Pad {
        // Sample playback
        juce::AudioBuffer<float> sampleBuffer;
        bool hasSample { false };
        int samplePosition { 0 };
        bool isPlaying { false };
        
        // Parameters
        float volume { 1.0f };
        float pan { 0.0f };
        float pitch { 0.0f };
        float attack { 0.0f };
        float release { 100.0f };
        
        // Synthesis
        bool synthMode { false };
        float synthFrequency { 100.0f };
        float synthDecay { 200.0f };
        float synthNoise { 0.3f };
        float synthPhase { 0.0f };
        float synthEnvelope { 0.0f };
        
        // Pattern
        std::array<bool, maxSteps> pattern{};
    };
    
    std::array<Pad, numPads> pads;
    
    // Sequencer
    int patternLength { 16 };
    int currentStep { 0 };
    double tempo { 120.0 };
    float swing { 0.0f };
    bool playing { false };
    
    double sampleRate { 44100.0 };
    double samplesPerStep { 0.0 };
    double stepProgress { 0.0 };
    
    void advanceStep();
    void triggerPad(int padIndex);
    void processPad(Pad& pad, juce::AudioBuffer<float>& buffer, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumMachine)
};

} // namespace OmegaStudio
