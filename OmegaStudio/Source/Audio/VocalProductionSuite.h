//==============================================================================
// VocalProductionSuite.h - Suite Profesional de Producción Vocal
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>

namespace OmegaStudio {

//==============================================================================
/** Vocal Tuner - Autotune y corrección de pitch profesional */
class VocalTuner {
public:
    //==========================================================================
    enum class Mode {
        Chromatic,      // Todas las notas
        Major,          // Escala mayor
        Minor,          // Escala menor
        Custom          // Escala personalizada
    };
    
    enum class Algorithm {
        Classic,        // Estilo Antares classic
        Natural,        // Corrección sutil
        Robot,          // T-Pain effect
        Retune          // Melodyne-style
    };
    
    //==========================================================================
    VocalTuner();
    ~VocalTuner() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setMode(Mode mode);
    void setAlgorithm(Algorithm algorithm);
    void setRetune(float retune);              // 0.0-1.0 (velocidad de corrección)
    void setAmount(float amount);              // 0.0-1.0 (mezcla wet/dry)
    void setHumanize(float humanize);          // 0.0-1.0 (imperfecciones humanas)
    
    // Scale
    void setKey(int root);                     // 0-11 (C=0)
    void setScale(const std::vector<bool>& enabledNotes);  // 12 bool
    
    // Formant
    void setFormantCorrection(bool enabled);
    void setFormantShift(float semitones);     // -12 to +12
    
    // Vibrato
    void setVibratoRate(float hz);
    void setVibratoDepth(float cents);
    void setVibratoShape(float shape);         // 0.0-1.0 (sine to triangle)
    
    // Analysis
    float getCurrentPitch() const;             // En Hz
    float getCurrentNote() const;              // MIDI note
    float getPitchConfidence() const;          // 0.0-1.0
    
    // Presets
    enum class Preset {
        Transparent, Subtle, Medium, Hard, Robot, 
        Natural, TrapVocals, PopVocals, RnBVocals
    };
    
    void loadPreset(Preset preset);
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    
    // Parameters
    Mode mode_ = Mode::Chromatic;
    Algorithm algorithm_ = Algorithm::Natural;
    float retune_ = 0.5f;
    float amount_ = 1.0f;
    float humanize_ = 0.0f;
    
    int key_ = 0;
    std::array<bool, 12> scale_;
    
    // Pitch detection (YIN algorithm)
    struct PitchDetector {
        std::vector<float> buffer;
        int bufferSize = 2048;
        float detectedPitch = 0.0f;
        float confidence = 0.0f;
        
        void detectPitch(const float* input, int numSamples, double sampleRate);
    } pitchDetector_;
    
    // Formant preservation
    struct FormantPreserver {
        bool enabled = true;
        float shift = 0.0f;
        
        std::vector<float> envelope;
        void extractFormants(const float* input, int numSamples);
        void applyFormants(float* output, int numSamples);
    } formantPreserver_;
    
    // Vibrato
    struct VibratoGenerator {
        float rate = 5.0f;
        float depth = 0.0f;
        float shape = 0.0f;
        float phase = 0.0f;
        
        float getModulation(double sampleRate);
    } vibratoGen_;
    
    // Pitch shifter
    void shiftPitch(float* buffer, int numSamples, float semitones);
    
    // Scale quantization
    float quantizePitch(float pitch) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalTuner)
};

//==============================================================================
/** Formant Shifter - Cambio independiente de formantes */
class FormantShifter {
public:
    FormantShifter();
    ~FormantShifter() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setFormantShift(float semitones);     // -12 to +12
    void setPitchShift(float semitones);       // -12 to +12 (independiente)
    void setGender(float amount);              // -1.0 to +1.0 (male to female)
    void setAge(float amount);                 // -1.0 to +1.0 (young to old)
    
    // Presets
    enum class Preset {
        MaleToFemale, FemaleToMale,
        ChildVoice, ElderVoice,
        Robot, Monster, Chipmunk,
        DeepVoice, HighVoice
    };
    
    void loadPreset(Preset preset);
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    
    float formantShift_ = 0.0f;
    float pitchShift_ = 0.0f;
    
    // LPC formant analysis
    struct FormantAnalyzer {
        static constexpr int order = 12;
        std::array<float, order> lpcCoefficients;
        
        void analyze(const float* input, int numSamples);
        void synthesize(float* output, int numSamples, float shift);
    } formantAnalyzer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FormantShifter)
};

//==============================================================================
/** Vocal Preset Chain - Cadenas de presets por género */
class VocalPresetChain {
public:
    //==========================================================================
    enum class Genre {
        Pop, Rock, RnB, HipHop, Electronic,
        Country, Jazz, Metal, Indie, Gospel
    };
    
    struct ProcessorSettings {
        juce::String type;              // "EQ", "Compressor", "DeEsser", etc
        juce::ValueTree parameters;
        bool enabled = true;
    };
    
    struct PresetChain {
        juce::String name;
        Genre genre;
        juce::String description;
        std::vector<ProcessorSettings> processors;
    };
    
    //==========================================================================
    VocalPresetChain();
    ~VocalPresetChain() = default;
    
    // Get presets
    std::vector<PresetChain> getPresetsForGenre(Genre genre) const;
    const PresetChain& getPreset(int index) const;
    int getNumPresets() const;
    
    // Apply chain
    void applyChain(const PresetChain& chain);
    
    // Presets específicos
    PresetChain getPopVocalChain() const;
    PresetChain getRnBVocalChain() const;
    PresetChain getHipHopVocalChain() const;
    PresetChain getRockVocalChain() const;
    PresetChain getElectronicVocalChain() const;
    
private:
    //==========================================================================
    std::vector<PresetChain> presets_;
    
    void initializePresets();
    
    // Helper para crear cadenas típicas
    PresetChain createChain(const juce::String& name, Genre genre,
                           const juce::String& description);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalPresetChain)
};

//==============================================================================
/** Sibilance Analyzer - Análisis y control de sibilancia en tiempo real */
class SibilanceAnalyzer {
public:
    //==========================================================================
    struct SibilanceInfo {
        float level;                    // 0.0-1.0
        float frequency;                // Frecuencia dominante (Hz)
        bool detected;                  // ¿Sibilancia detectada?
        
        std::vector<float> spectrum;    // Espectro para visualización
    };
    
    //==========================================================================
    SibilanceAnalyzer();
    ~SibilanceAnalyzer() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Analysis
    SibilanceInfo analyze(const juce::AudioBuffer<float>& buffer);
    
    // De-essing
    void deEss(juce::AudioBuffer<float>& buffer, float threshold, float ratio);
    
    // Parameters
    void setSensitivity(float sensitivity);    // 0.0-1.0
    void setFrequencyRange(float lowHz, float highHz);
    
    // Real-time monitoring
    float getCurrentSibilanceLevel() const { return currentLevel_; }
    bool isSibilanceDetected() const { return sibilanceDetected_; }
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    
    float sensitivity_ = 0.5f;
    float lowFreq_ = 4000.0f;
    float highFreq_ = 10000.0f;
    
    float currentLevel_ = 0.0f;
    bool sibilanceDetected_ = false;
    
    // FFT analysis
    juce::dsp::FFT fft_{11};  // 2048 samples
    std::vector<float> fftData_;
    
    // High-pass filter para aislar sibilancia
    juce::dsp::IIR::Filter<float> highPassFilter_;
    
    // Envelope follower
    float envelopeFollower_ = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SibilanceAnalyzer)
};

//==============================================================================
/** Breath Controller - Control de respiraciones vocales */
class BreathController {
public:
    //==========================================================================
    enum class Mode {
        Remove,         // Eliminar completamente
        Reduce,         // Reducir nivel
        Enhance,        // Enfatizar (para efecto artístico)
        Replace         // Reemplazar con samples
    };
    
    //==========================================================================
    BreathController();
    ~BreathController() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Parameters
    void setMode(Mode mode);
    void setSensitivity(float sensitivity);    // 0.0-1.0
    void setReduction(float dB);               // -60 to 0 dB
    
    // Detection
    bool isBreathDetected() const { return breathDetected_; }
    float getBreathLevel() const { return breathLevel_; }
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    
    Mode mode_ = Mode::Reduce;
    float sensitivity_ = 0.5f;
    float reduction_ = -12.0f;
    
    bool breathDetected_ = false;
    float breathLevel_ = 0.0f;
    
    // Low-pass filter para aislar respiraciones
    juce::dsp::IIR::Filter<float> lowPassFilter_;
    
    // Noise gate
    float gateThreshold_ = 0.1f;
    float gateAttack_ = 0.001f;
    float gateRelease_ = 0.05f;
    float gateEnvelope_ = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BreathController)
};

//==============================================================================
/** Vocal Doubler - Doblaje automático de voces */
class VocalDoubler {
public:
    VocalDoubler();
    ~VocalDoubler() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(const juce::AudioBuffer<float>& input, 
                juce::AudioBuffer<float>& output);
    
    // Parameters
    void setNumVoices(int num);                // 1-4 voces adicionales
    void setSpread(float amount);              // 0.0-1.0 (stereo width)
    void setTiming(float ms);                  // 0-50ms (delay variation)
    void setPitch(float cents);                // 0-50 cents (pitch variation)
    void setTone(float amount);                // 0.0-1.0 (filter variation)
    
    // Mix
    void setDryWet(float mix);                 // 0.0-1.0
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    
    int numVoices_ = 2;
    float spread_ = 0.5f;
    float timing_ = 20.0f;
    float pitch_ = 10.0f;
    float tone_ = 0.3f;
    float dryWet_ = 0.5f;
    
    // Delay lines para timing variation
    struct Voice {
        std::vector<float> delayBuffer;
        int delayTime = 0;
        float pitchShift = 0.0f;
        float pan = 0.0f;
        
        juce::dsp::IIR::Filter<float> toneFilter;
    };
    
    std::vector<Voice> voices_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalDoubler)
};

} // namespace OmegaStudio
