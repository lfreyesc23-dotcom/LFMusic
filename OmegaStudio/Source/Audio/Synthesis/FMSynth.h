#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include <memory>

namespace OmegaStudio {

/**
 * @brief Professional 6-Operator FM Synthesizer (DX7-Style)
 * 
 * Features:
 * - 6 operators with sine oscillators
 * - 32 classic DX7 algorithms
 * - Per-operator ADSR envelopes
 * - Keyboard rate scaling
 * - Velocity sensitivity
 * - LFO with multiple destinations
 * - Feedback loop support
 * - 128 preset slots
 * - Real-time parameter modulation
 */
class FMSynth : public juce::Synthesiser {
public:
    static constexpr int NUM_OPERATORS = 6;
    static constexpr int NUM_ALGORITHMS = 32;
    
    //==============================================================================
    // Operator Configuration
    struct OperatorParams {
        // Level & Tuning
        float outputLevel = 99.0f;      // 0-99 (DX7 style)
        float coarse = 1.0f;            // 0.5, 1, 2, 3, 4... 31
        float fine = 0.0f;              // -99 to +99 cents
        bool fixed = false;             // Fixed frequency mode
        float fixedFreq = 440.0f;       // Hz when in fixed mode
        
        // Envelope (6-stage DX7 style)
        struct Envelope {
            float rate1 = 99.0f;        // Attack rate
            float rate2 = 95.0f;        // Decay 1 rate
            float rate3 = 80.0f;        // Decay 2 rate
            float rate4 = 70.0f;        // Release rate
            float level1 = 99.0f;       // Attack level
            float level2 = 90.0f;       // Decay 1 level
            float level3 = 70.0f;       // Sustain level
            float level4 = 0.0f;        // Release level (always 0)
        } envelope;
        
        // Keyboard Scaling
        float rateScaling = 0.0f;       // 0-7 (how much key affects envelope)
        int breakpoint = 60;            // C4 (MIDI note where scaling = 0)
        float leftDepth = 0.0f;         // -99 to +99
        float rightDepth = 0.0f;        // -99 to +99
        int leftCurve = 0;              // -lin, -exp, +exp, +lin
        int rightCurve = 0;
        
        // Modulation
        float velocitySens = 0.0f;      // 0-7
        float ampModSens = 0.0f;        // 0-3 (sensitivity to LFO amplitude)
        
        bool enabled = true;
    };
    
    //==============================================================================
    // Algorithm Definition
    struct Algorithm {
        int id;
        juce::String name;
        
        // Routing matrix: [operator][modulator]
        // 1 = modulation connection, 0 = no connection
        std::array<std::array<int, NUM_OPERATORS>, NUM_OPERATORS> routing;
        
        // Which operators are carriers (output to audio)
        std::array<bool, NUM_OPERATORS> isCarrier;
        
        // Feedback path (which operator feeds back to itself)
        int feedbackOp = -1;
        
        Algorithm() {
            // Initialize with no connections
            for (auto& row : routing) {
                row.fill(0);
            }
            isCarrier.fill(false);
        }
    };
    
    //==============================================================================
    // LFO Parameters
    struct LFOParams {
        enum Waveform { Triangle, SawDown, SawUp, Square, Sine, SampleHold };
        
        Waveform waveform = Sine;
        float speed = 35.0f;            // 0-99
        float delay = 0.0f;             // 0-99 (delay before LFO starts)
        float pitchModDepth = 0.0f;     // 0-99 (pitch mod depth)
        float ampModDepth = 0.0f;       // 0-99 (amplitude mod depth)
        bool sync = false;              // Sync to key-on
        
        // PMD = Pitch Mod Depth, AMD = Amplitude Mod Depth
    };
    
    //==============================================================================
    // Synth Parameters
    struct SynthParams {
        juce::String name = "Init";
        
        // Operators
        std::array<OperatorParams, NUM_OPERATORS> operators;
        
        // Algorithm
        int algorithmId = 0;
        
        // Feedback
        float feedback = 0.0f;          // 0-7
        
        // LFO
        LFOParams lfo;
        
        // Global
        float pitchBendRange = 2.0f;    // Semitones
        int transpose = 0;              // -24 to +24
        
        // Performance
        struct PitchEnv {
            float rate1 = 99.0f, rate2 = 99.0f, rate3 = 99.0f, rate4 = 99.0f;
            float level1 = 50.0f, level2 = 50.0f, level3 = 50.0f, level4 = 50.0f;
        } pitchEnv;
        
        float masterVolume = 0.8f;
        int maxPolyphony = 16;
    };
    
    //==============================================================================
    FMSynth();
    ~FMSynth() override;
    
    // Setup
    void prepare(const juce::dsp::ProcessSpec& spec);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        const juce::MidiBuffer& midiMessages,
                        int startSample, int numSamples);
    
    // Parameters
    void setParameters(const SynthParams& params);
    SynthParams& getParameters() { return params; }
    const SynthParams& getParameters() const { return params; }
    
    // Algorithms
    static std::vector<Algorithm> getAllAlgorithms();
    static Algorithm getAlgorithm(int id);
    void setAlgorithm(int algorithmId);
    
    // Presets
    struct Preset {
        juce::String name;
        juce::String category;
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
    class FMVoice : public juce::SynthesiserVoice {
    public:
        FMVoice(FMSynth& owner);
        
        bool canPlaySound(juce::SynthesiserSound*) override { return true; }
        void startNote(int midiNoteNumber, float velocity,
                      juce::SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote(float velocity, bool allowTailOff) override;
        void pitchWheelMoved(int newPitchWheelValue) override;
        void controllerMoved(int controllerNumber, int newControllerValue) override;
        void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                           int startSample, int numSamples) override;
        
    private:
        FMSynth& synth;
        
        // Operator state
        struct OpState {
            float phase = 0.0f;
            float output = 0.0f;
            float feedbackSample = 0.0f;
            
            // Envelope state
            enum EnvStage { R1, R2, R3, R4, Off };
            EnvStage envStage = Off;
            float envLevel = 0.0f;
            float envTarget = 0.0f;
            float envRate = 0.0f;
            
            bool isActive = false;
        };
        std::array<OpState, NUM_OPERATORS> opStates;
        
        // LFO state
        struct LFOState {
            float phase = 0.0f;
            float delayCounter = 0.0f;
            float value = 0.0f;
            bool active = false;
        } lfoState;
        
        // Pitch envelope state
        struct PitchEnvState {
            enum Stage { R1, R2, R3, R4, Off };
            Stage stage = Off;
            float level = 0.0f;
        } pitchEnvState;
        
        // Voice state
        int noteNumber = 0;
        float velocity = 0.0f;
        float pitchBend = 0.0f;
        double sampleRate = 44100.0;
        
        // Processing
        float processOperator(int opIndex, float modulation, float dt);
        void updateEnvelope(int opIndex, float dt);
        void updateLFO(float dt);
        void updatePitchEnvelope(float dt);
        float getOperatorFrequency(int opIndex, float basePitch);
        float scaleCurve(float input, int curveType);
        float dxLevelToLinear(float dxLevel); // 0-99 to 0-1
        float dxRateToTime(float rate, float keyScale); // 0-99 to seconds
    };
    
    //==============================================================================
    SynthParams params;
    juce::dsp::ProcessSpec currentSpec;
    std::atomic<double> cpuUsage{0.0};
    
    // Algorithm database
    static void initializeAlgorithms();
    static std::vector<Algorithm> algorithms;
    
    // Preset system
    void initializeFactoryPresets();
    std::vector<Preset> factoryPresets;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynth)
};

//==============================================================================
// Helper Functions
class FMHelpers {
public:
    // DX7 conversion utilities
    static float dxLevelToLinear(float dxLevel);
    static float dxRateToSeconds(float rate);
    static float linearToDxLevel(float linear);
    static float secondsToDxRate(float seconds);
    
    // Frequency calculation
    static float calculateOperatorFreq(float baseFreq, float coarse, float fine, bool fixed);
    
    // Envelope curves
    static float applyScalingCurve(float input, int curveType);
    
    // Algorithm visualization
    static juce::String getAlgorithmDescription(int algorithmId);
    static juce::String getAlgorithmDiagram(int algorithmId);
};

} // namespace OmegaStudio
