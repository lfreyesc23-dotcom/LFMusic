#pragma once
#include <JuceHeader.h>
#include <array>
#include <memory>

namespace OmegaStudio {

/**
 * @brief Professional Virtual Analog Synthesizer
 * 
 * Features:
 * - 3 oscillators (Saw, Square, Triangle, Sine, Noise, PWM)
 * - Sub-oscillator
 * - Multi-mode filter (LP/HP/BP/Notch 12/24/36dB)
 * - 3 ADSR envelopes (Amp, Filter, Mod)
 * - 2 LFOs with tempo sync
 * - Modulation matrix (8 sources x 8 destinations)
 * - Unison mode per oscillator
 * - Built-in effects (Chorus, Phaser, Delay)
 * - Arpeggiator
 */
class VirtualAnalogSynth : public juce::Synthesiser {
public:
    static constexpr int MAX_VOICES = 32;
    static constexpr int NUM_OSCILLATORS = 3;
    
    //==============================================================================
    // Oscillator Types
    enum class OscType {
        Saw,
        Square,
        Triangle,
        Sine,
        Noise,
        PWM          // Pulse Width Modulation
    };
    
    //==============================================================================
    // Filter Types & Modes
    enum class FilterType {
        LowPass12,
        LowPass24,
        LowPass36,
        HighPass12,
        HighPass24,
        HighPass36,
        BandPass12,
        BandPass24,
        Notch12,
        Notch24,
        AllPass
    };
    
    //==============================================================================
    // Oscillator Parameters
    struct OscillatorParams {
        OscType type = OscType::Saw;
        bool enabled = true;
        
        // Tuning
        int octave = 0;              // -3 to +3
        int semitone = 0;            // -12 to +12
        int cents = 0;               // -100 to +100
        
        // Waveform-specific
        float pulseWidth = 0.5f;     // For PWM (0.05-0.95)
        
        // Unison
        int unisonVoices = 1;        // 1-8
        float unisonDetune = 0.1f;   // 0-1 (cents)
        float unisonSpread = 0.5f;   // Stereo spread
        float unisonBlend = 0.5f;    // Mix with main osc
        
        // Level & Pan
        float level = 1.0f;          // 0-1
        float pan = 0.0f;            // -1 to +1
        
        // Phase
        float phaseOffset = 0.0f;    // 0-1
        bool freeRunning = false;    // vs. reset on note-on
    };
    
    //==============================================================================
    // Sub-Oscillator
    struct SubOscParams {
        bool enabled = false;
        enum Type { Sine, Square, Triangle } type = Sine;
        int octave = -1;             // Relative to osc 1
        float level = 0.5f;
    };
    
    //==============================================================================
    // Filter Parameters
    struct FilterParams {
        FilterType type = FilterType::LowPass24;
        float cutoff = 10000.0f;     // Hz
        float resonance = 0.0f;      // 0-1
        float drive = 0.0f;          // 0-1 (filter saturation)
        
        // Modulation
        float envAmount = 0.5f;      // -1 to +1
        float lfo1Amount = 0.0f;
        float lfo2Amount = 0.0f;
        float velocityAmount = 0.0f;
        float keyTrack = 0.0f;       // 0-1
    };
    
    //==============================================================================
    // ADSR Envelope
    struct EnvelopeParams {
        float attack = 0.01f;
        float decay = 0.1f;
        float sustain = 0.7f;
        float release = 0.3f;
        float attackCurve = 0.0f;    // -1 (log) to +1 (exp)
        float decayCurve = 0.0f;
        float releaseCurve = 0.0f;
    };
    
    //==============================================================================
    // LFO Parameters
    struct LFOParams {
        enum Waveform { Sine, Triangle, Saw, Square, SampleHold, Random };
        Waveform waveform = Sine;
        
        float rate = 1.0f;           // Hz
        bool tempoSync = false;
        float syncRate = 0.25f;      // 1/16, 1/8, 1/4, etc.
        
        float amount = 0.5f;
        float phase = 0.0f;
        bool bipolar = true;
        bool oneShot = false;        // Trigger once per note
        bool freeRunning = true;     // vs. reset on note
    };
    
    //==============================================================================
    // Modulation Matrix
    enum class ModSource {
        None,
        LFO1,
        LFO2,
        ModEnv,
        Velocity,
        ModWheel,
        Aftertouch,
        KeyTrack
    };
    
    enum class ModDest {
        None,
        Osc1Pitch,
        Osc2Pitch,
        Osc3Pitch,
        Osc1PW,
        FilterCutoff,
        FilterRes,
        Pan,
        Volume
    };
    
    struct ModSlot {
        ModSource source = ModSource::None;
        ModDest dest = ModDest::None;
        float amount = 0.0f;         // -1 to +1
    };
    
    //==============================================================================
    // Arpeggiator
    struct ArpParams {
        bool enabled = false;
        enum Mode { Up, Down, UpDown, Random, Played } mode = Up;
        float rate = 0.125f;         // 1/8 note
        int octaves = 1;             // 1-4
        int gateLength = 80;         // % of note length
    };
    
    //==============================================================================
    // Main Synth Parameters
    struct SynthParams {
        // Oscillators
        std::array<OscillatorParams, NUM_OSCILLATORS> oscillators;
        SubOscParams subOsc;
        
        // Mixer
        std::array<float, NUM_OSCILLATORS> oscMix = {1.0f, 0.0f, 0.0f};
        float subMix = 0.0f;
        float noiseMix = 0.0f;
        
        // Filter
        FilterParams filter;
        
        // Envelopes
        EnvelopeParams ampEnv;
        EnvelopeParams filterEnv;
        EnvelopeParams modEnv;
        
        // LFOs
        std::array<LFOParams, 2> lfos;
        
        // Modulation Matrix
        std::array<ModSlot, 8> modMatrix;
        
        // Arpeggiator
        ArpParams arp;
        
        // Master
        float masterVolume = 0.8f;
        float masterTune = 0.0f;     // Cents
        int voiceMode = 0;           // 0=Poly, 1=Mono, 2=Legato
        int maxVoices = 8;
        float portamento = 0.0f;     // Glide time
        
        // Effects
        bool chorusEnabled = false;
        float chorusMix = 0.3f;
        bool phaserEnabled = false;
        float phaserMix = 0.3f;
    };
    
    //==============================================================================
    VirtualAnalogSynth();
    ~VirtualAnalogSynth() override;
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        const juce::MidiBuffer& midiMessages,
                        int startSample, int numSamples);
    
    // Parameters
    void setParameters(const SynthParams& params);
    SynthParams& getParameters() { return params; }
    const SynthParams& getParameters() const { return params; }
    
    // Preset system
    struct Preset {
        juce::String name;
        juce::String category;
        juce::String description;
        SynthParams params;
    };
    
    void loadPreset(const Preset& preset);
    Preset getCurrentPreset() const;
    static std::vector<Preset> getFactoryPresets();
    
    // Performance
    int getActiveVoiceCount() const;
    double getCPUUsage() const { return cpuUsage.load(); }
    
private:
    //==============================================================================
    // Voice Implementation
    class AnalogVoice : public juce::SynthesiserVoice {
    public:
        AnalogVoice(VirtualAnalogSynth& owner);
        
        bool canPlaySound(juce::SynthesiserSound*) override { return true; }
        void startNote(int midiNoteNumber, float velocity,
                      juce::SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int newPitchWheelValue) override;
        void controllerMoved(int controllerNumber, int newControllerValue) override;
        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                           int startSample, int numSamples) override;
        
    private:
        VirtualAnalogSynth& synth;
        
        // Oscillator state
        struct OscState {
            std::array<float, 8> phases = {0.0f};     // Unison phases
            std::array<float, 8> detuneAmounts = {0.0f};
            float lastOutput = 0.0f;                   // For bandlimited step
        };
        std::array<OscState, NUM_OSCILLATORS> oscStates;
        OscState subOscState;
        
        // Envelope states
        struct EnvState {
            enum Stage { Attack, Decay, Sustain, Release, Idle };
            Stage stage = Idle;
            float level = 0.0f;
            float releaseLevel = 0.0f;
        };
        EnvState ampEnv, filterEnv, modEnv;
        
        // LFO states
        struct LFOState {
            float phase = 0.0f;
            float value = 0.0f;
            float randomValue = 0.0f;
        };
        std::array<LFOState, 2> lfoStates;
        
        // Filter state (4-pole state-variable)
        struct FilterState {
            std::array<float, 4> ic1eq = {0.0f};
            std::array<float, 4> ic2eq = {0.0f};
        } filterState;
        
        // Voice state
        int noteNumber = 0;
        float velocity = 0.0f;
        float pitchBend = 0.0f;
        float modWheel = 0.0f;
        float aftertouch = 0.0f;
        double sampleRate = 44100.0;
        
        // Portamento
        float targetPitch = 0.0f;
        float currentPitch = 0.0f;
        
        // Processing methods
        float renderOscillator(int oscIndex);
        float renderSubOscillator();
        float generateWaveform(OscType type, float phase, float pw);
        float processFilter(float input);
        float processEnvelope(EnvState& env, const EnvelopeParams& params, float dt);
        void updateLFOs(float dt);
        void updateModulation();
        void updatePortamento(float dt);
        
        // Band-limited oscillators
        float polyBLEP(float phase, float phaseInc);
    };
    
    //==============================================================================
    SynthParams params;
    juce::dsp::ProcessSpec currentSpec;
    std::atomic<double> cpuUsage{0.0};
    
    // Factory content
    void initializeFactoryPresets();
    std::vector<Preset> factoryPresets;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualAnalogSynth)
};

} // namespace OmegaStudio
