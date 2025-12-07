#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include <memory>

namespace OmegaStudio {

/**
 * @brief Professional Wavetable Synthesizer
 * 
 * Features:
 * - 2048 samples per wavetable frame
 * - 256 frames per wavetable with morphing
 * - Up to 8 voice unison with detune & stereo spread
 * - Multi-mode filter (LP/HP/BP/Notch, 12/24dB)
 * - 2 LFOs with multiple waveforms
 * - 2 ADSR envelopes (amp + filter)
 * - Built-in effects: Chorus, Distortion
 * - Preset system with factory wavetables
 */
class WavetableSynth : public juce::Synthesiser {
public:
    static constexpr int WAVETABLE_SIZE = 2048;
    static constexpr int MAX_FRAMES = 256;
    static constexpr int MAX_UNISON_VOICES = 8;
    
    //==============================================================================
    // Wavetable Data Structure
    struct Wavetable {
        std::string name;
        std::vector<std::array<float, WAVETABLE_SIZE>> frames; // Multiple frames for morphing
        int frameCount = 1;
        
        Wavetable() : frameCount(1) {
            frames.resize(1);
            frames[0].fill(0.0f);
        }
        
        void loadFromBuffer(const juce::AudioBuffer<float>& buffer);
        void generateBasicWaveforms(const juce::String& type);
    };
    
    //==============================================================================
    // Filter Types
    enum class FilterType {
        LowPass12dB,
        LowPass24dB,
        HighPass12dB,
        HighPass24dB,
        BandPass12dB,
        BandPass24dB,
        Notch,
        AllPass
    };
    
    //==============================================================================
    // LFO Configuration
    enum class LFOWaveform {
        Sine,
        Triangle,
        Sawtooth,
        Square,
        Random,
        SampleAndHold
    };
    
    struct LFOParams {
        LFOWaveform waveform = LFOWaveform::Sine;
        float rate = 1.0f;          // Hz or sync to tempo
        float depth = 0.5f;         // 0-1
        bool tempoSync = false;
        float phase = 0.0f;
        bool bipolar = true;        // -1 to 1 or 0 to 1
    };
    
    //==============================================================================
    // ADSR Envelope
    struct EnvelopeParams {
        float attack = 0.01f;       // seconds
        float decay = 0.1f;
        float sustain = 0.7f;       // level 0-1
        float release = 0.3f;
        float curve = 0.0f;         // -1 (exponential) to 1 (logarithmic)
    };
    
    //==============================================================================
    // Oscillator Parameters
    struct OscillatorParams {
        std::shared_ptr<Wavetable> wavetable;
        float position = 0.0f;       // Frame position for morphing (0-1)
        int unisonVoices = 1;        // 1-8
        float unisonDetune = 0.1f;   // Cents
        float unisonSpread = 0.5f;   // Stereo spread 0-1
        float gain = 1.0f;
        float pan = 0.0f;            // -1 to 1
        int octave = 0;              // -2 to +2
        int semitone = 0;            // -12 to +12
        int cents = 0;               // -100 to +100
    };
    
    //==============================================================================
    // Filter Parameters
    struct FilterParams {
        FilterType type = FilterType::LowPass24dB;
        float cutoff = 20000.0f;     // Hz
        float resonance = 0.0f;      // 0-1
        float keyTracking = 0.0f;    // 0-1 (how much note pitch affects cutoff)
        float envAmount = 0.0f;      // -1 to 1
        float lfoAmount = 0.0f;      // -1 to 1
    };
    
    //==============================================================================
    // Main Synth Parameters
    struct SynthParams {
        // Oscillators (up to 3)
        std::array<OscillatorParams, 3> oscillators;
        std::array<bool, 3> oscEnabled = {true, false, false};
        
        // Filter
        FilterParams filter;
        
        // Envelopes
        EnvelopeParams ampEnvelope;
        EnvelopeParams filterEnvelope;
        
        // LFOs
        std::array<LFOParams, 2> lfos;
        
        // Master
        float masterVolume = 0.8f;
        float pitchBend = 0.0f;      // -1 to 1 (±2 semitones)
        int voices = 8;              // Max polyphony
        
        // Effects
        bool chorusEnabled = false;
        float chorusMix = 0.3f;
        bool distortionEnabled = false;
        float distortionAmount = 0.0f;
    };

    //==============================================================================
    WavetableSynth();
    ~WavetableSynth() override;
    
    // Core synthesis
    void prepare(const juce::dsp::ProcessSpec& spec);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, 
                        const juce::MidiBuffer& midiMessages,
                        int startSample, int numSamples);
    
    // Parameter control
    void setParameters(const SynthParams& params);
    SynthParams& getParameters() { return params; }
    const SynthParams& getParameters() const { return params; }
    
    // Wavetable management
    void loadWavetable(int oscIndex, std::shared_ptr<Wavetable> wt);
    std::shared_ptr<Wavetable> createWavetable(const juce::String& type);
    
    // Factory wavetables
    static std::vector<std::string> getFactoryWavetableNames();
    std::shared_ptr<Wavetable> loadFactoryWavetable(const juce::String& name);
    
    // Preset system
    struct Preset {
        juce::String name;
        juce::String category;
        SynthParams params;
    };
    
    void loadPreset(const Preset& preset);
    Preset getCurrentPreset() const;
    static std::vector<Preset> getFactoryPresets();
    
    // Voice management
    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override;
    void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;
    void allNotesOff(int midiChannel, bool allowTailOff) override;
    
    // CPU optimization
    void setMaxPolyphony(int voices);
    int getActiveVoiceCount() const;
    double getCPUUsage() const { return cpuUsage.load(); }

private:
    //==============================================================================
    // Voice class for polyphony
    class WavetableVoice : public juce::SynthesiserVoice {
    public:
        WavetableVoice(WavetableSynth& owner);
        
        bool canPlaySound(juce::SynthesiserSound*) override { return true; }
        void startNote(int midiNoteNumber, float velocity, 
                      juce::SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int newPitchWheelValue) override;
        void controllerMoved(int controllerNumber, int newControllerValue) override;
        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                           int startSample, int numSamples) override;
        
    private:
        WavetableSynth& synth;
        
        // Oscillator state
        struct OscState {
            std::array<float, MAX_UNISON_VOICES> phase = {0.0f};
            std::array<float, MAX_UNISON_VOICES> detuneAmount = {0.0f};
            std::array<float, MAX_UNISON_VOICES> panAmount = {0.0f};
        };
        std::array<OscState, 3> oscStates;
        
        // Envelope state
        struct EnvState {
            enum Stage { Attack, Decay, Sustain, Release, Idle };
            Stage stage = Idle;
            float level = 0.0f;
            float releaseLevel = 0.0f;
        };
        EnvState ampEnv, filterEnv;
        
        // LFO state
        struct LFOState {
            float phase = 0.0f;
            float value = 0.0f;
        };
        std::array<LFOState, 2> lfoStates;
        
        // Voice parameters
        int noteNumber = 0;
        float velocity = 0.0f;
        float currentPitch = 0.0f;
        double sampleRate = 44100.0;
        
        // Filter state (per-voice state-variable filter)
        struct FilterState {
            float ic1eq = 0.0f, ic2eq = 0.0f; // integrator states
            float v0 = 0.0f, v1 = 0.0f, v2 = 0.0f, v3 = 0.0f;
        };
        FilterState filterState;
        
        // Helper methods
        float renderOscillator(int oscIndex, float pitch);
        float getWavetableSample(const Wavetable* wt, float phase, float position);
        float processEnvelope(EnvState& env, const EnvelopeParams& params, float dt);
        float processLFO(int lfoIndex, float dt);
        float processFilter(float input, float cutoff, float resonance);
        void updateLFOs(float dt);
    };
    
    //==============================================================================
    // State-variable filter (Chamberlin)
    class SVFilter {
    public:
        void setType(FilterType type) { this->type = type; }
        void setSampleRate(double sr) { sampleRate = sr; }
        void setParams(float cutoff, float resonance);
        float process(float input, float& ic1eq, float& ic2eq);
        
    private:
        FilterType type = FilterType::LowPass24dB;
        double sampleRate = 44100.0;
        float g = 0.0f, k = 0.0f;
    };
    
    //==============================================================================
    // Built-in effects
    class ChorusEffect {
    public:
        void prepare(double sampleRate, int maxBlockSize);
        void process(juce::AudioBuffer<float>& buffer, float mix);
        void reset();
        
    private:
        juce::dsp::Chorus<float> chorus;
        juce::AudioBuffer<float> dryBuffer;
    };
    
    class DistortionEffect {
    public:
        void prepare(double sampleRate);
        void process(juce::AudioBuffer<float>& buffer, float amount);
        
    private:
        float processSample(float input, float drive);
    };
    
    //==============================================================================
    SynthParams params;
    juce::dsp::ProcessSpec currentSpec;
    
    // Built-in effects
    ChorusEffect chorus;
    DistortionEffect distortion;
    
    // Performance monitoring
    std::atomic<double> cpuUsage{0.0};
    juce::Time lastCPUCheck;
    
    // Factory content
    void initializeFactoryWavetables();
    void initializeFactoryPresets();
    std::map<juce::String, std::shared_ptr<Wavetable>> factoryWavetables;
    std::vector<Preset> factoryPresets;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSynth)
};

//==============================================================================
// Wavetable Generator Utilities
class WavetableGenerator {
public:
    // Basic waveforms
    static std::shared_ptr<WavetableSynth::Wavetable> createSine();
    static std::shared_ptr<WavetableSynth::Wavetable> createSawtooth();
    static std::shared_ptr<WavetableSynth::Wavetable> createSquare();
    static std::shared_ptr<WavetableSynth::Wavetable> createTriangle();
    
    // Advanced wavetables
    static std::shared_ptr<WavetableSynth::Wavetable> createHarmonic(
        const std::vector<float>& harmonicAmplitudes);
    static std::shared_ptr<WavetableSynth::Wavetable> createFromFormula(
        std::function<float(float)> formula, int frames = 64);
    static std::shared_ptr<WavetableSynth::Wavetable> createMorphing(
        const std::vector<std::shared_ptr<WavetableSynth::Wavetable>>& tables);
    
    // Serum-style wavetables
    static std::shared_ptr<WavetableSynth::Wavetable> createBasic64();
    static std::shared_ptr<WavetableSynth::Wavetable> createPWM();
    static std::shared_ptr<WavetableSynth::Wavetable> createVoicelike();
    static std::shared_ptr<WavetableSynth::Wavetable> createDigital();
    static std::shared_ptr<WavetableSynth::Wavetable> createAnalog();
    
    // Load from audio file
    static std::shared_ptr<WavetableSynth::Wavetable> loadFromAudioFile(
        const juce::File& file);
};

} // namespace OmegaStudio
