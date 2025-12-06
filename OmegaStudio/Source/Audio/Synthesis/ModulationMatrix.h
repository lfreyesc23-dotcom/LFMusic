//==============================================================================
// ModulationMatrix.h - Modulation Matrix 16x16 (Serum-style)
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <map>

namespace OmegaStudio {

//==============================================================================
/** Modulation Matrix Profesional 16x16
 *  - 16 fuentes de modulación (LFOs, Envelopes, MIDI CC, etc.)
 *  - 16+ destinos (Filter, Pitch, Pan, Wavetable Position, etc.)
 *  - Bipolar/Unipolar mode por routing
 *  - Amount ajustable por conexión
 *  - Curvas de modulación customizables
 */
class ModulationMatrix {
public:
    //==========================================================================
    enum class ModSource {
        // LFOs
        LFO1, LFO2, LFO3, LFO4,
        
        // Envelopes
        Env1, Env2, Env3,
        
        // MIDI
        Velocity,
        Aftertouch,
        ModWheel,
        PitchBend,
        
        // Macro controls
        Macro1, Macro2, Macro3, Macro4,
        
        // Random
        Random,
        
        Count
    };
    
    enum class ModDestination {
        // Oscillator
        Osc1Pitch,
        Osc1WavetablePos,
        Osc1Level,
        Osc1Pan,
        
        Osc2Pitch,
        Osc2WavetablePos,
        Osc2Level,
        Osc2Pan,
        
        // Filter
        FilterCutoff,
        FilterResonance,
        FilterDrive,
        FilterMix,
        
        // Effects
        ChorusDepth,
        ChorusRate,
        DelayTime,
        DelayFeedback,
        ReverbMix,
        
        // Global
        MasterPitch,
        MasterVolume,
        MasterPan,
        
        Count
    };
    
    //==========================================================================
    struct ModConnection {
        ModSource source = ModSource::LFO1;
        ModDestination destination = ModDestination::FilterCutoff;
        
        float amount = 0.0f;            // -1.0 a 1.0
        bool bipolar = false;           // false = 0-1, true = -1 a 1
        bool enabled = true;
        
        // Curve shaping
        float curvature = 0.0f;         // -1 (exp) a 1 (log), 0 = linear
    };
    
    //==========================================================================
    ModulationMatrix();
    ~ModulationMatrix() = default;
    
    // Setup
    void prepare(double sampleRate);
    void reset();
    
    // Connection Management
    int addConnection(ModSource source, ModDestination dest, float amount = 0.5f);
    void removeConnection(int index);
    void clearAllConnections();
    
    int getNumConnections() const { return static_cast<int>(connections_.size()); }
    const ModConnection& getConnection(int index) const { return connections_[index]; }
    
    void setConnectionAmount(int index, float amount);
    void setConnectionEnabled(int index, bool enabled);
    void setConnectionBipolar(int index, bool bipolar);
    void setConnectionCurvature(int index, float curvature);
    
    // Source Values (actualizar desde externa)
    void setSourceValue(ModSource source, float value);
    float getSourceValue(ModSource source) const;
    
    // Calculate modulation para un destino específico
    float getModulationFor(ModDestination dest) const;
    
    // Get all modulation values
    std::map<ModDestination, float> getAllModulationValues() const;
    
    // Presets
    void loadPreset(const juce::String& presetName);
    juce::StringArray getPresetList() const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::vector<ModConnection> connections_;
    std::array<float, static_cast<size_t>(ModSource::Count)> sourceValues_{};
    
    double sampleRate_ = 48000.0;
    
    // Curve shaping
    float applyCurve(float value, float curvature) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrix)
};

//==============================================================================
/** Drawable LFO - LFO con forma dibujable */
class DrawableLFO {
public:
    //==========================================================================
    enum class WaveShape {
        Sine,
        Triangle,
        Sawtooth,
        Square,
        Random,
        Custom          // Usuario dibuja
    };
    
    //==========================================================================
    DrawableLFO();
    ~DrawableLFO() = default;
    
    // Setup
    void prepare(double sampleRate);
    void reset();
    
    // Shape
    void setWaveShape(WaveShape shape);
    WaveShape getWaveShape() const { return shape_; }
    
    // Custom wave (512 samples)
    void setCustomWaveform(const std::vector<float>& waveform);
    const std::vector<float>& getCustomWaveform() const { return customWaveform_; }
    
    // Parameters
    void setFrequency(float hz);
    void setPhase(float phase);          // 0.0 - 1.0
    void setAmplitude(float amplitude);   // 0.0 - 1.0
    void setOffset(float offset);         // 0.0 - 1.0
    
    // Sync
    void setTempoSync(bool enabled, double bpm, float divisions);  // divisions: 1.0 = quarter note
    
    // Processing
    float process();
    float getCurrentValue() const { return currentValue_; }
    
    // Visualization
    std::vector<float> getWaveformDisplay(int numSamples) const;
    
private:
    //==========================================================================
    WaveShape shape_ = WaveShape::Sine;
    std::vector<float> customWaveform_;
    
    double sampleRate_ = 48000.0;
    float frequency_ = 1.0f;
    float phase_ = 0.0f;
    float amplitude_ = 1.0f;
    float offset_ = 0.0f;
    
    bool tempoSync_ = false;
    double bpm_ = 120.0;
    float divisions_ = 1.0f;
    
    float currentPhase_ = 0.0f;
    float currentValue_ = 0.0f;
    
    // Wave generation
    float generateSample(float phase);
    float interpolateCustomWaveform(float phase) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrawableLFO)
};

//==============================================================================
/** Unison Engine - Super saws y unison avanzado */
class UnisonEngine {
public:
    //==========================================================================
    struct UnisonSettings {
        int numVoices = 7;              // 1-16
        float detune = 0.2f;            // 0.0 - 1.0 (cents)
        float spread = 0.5f;            // 0.0 - 1.0 (stereo width)
        float blend = 0.0f;             // 0.0 (stacked) - 1.0 (spread)
        
        // Phase randomization
        bool randomPhase = true;
        
        // Detune curve
        enum class DetuneMode {
            Linear,         // Detune lineal
            Exponential,    // Más detune en voces externas
            Power           // Custom power curve
        } detuneMode = DetuneMode::Exponential;
    };
    
    //==========================================================================
    UnisonEngine();
    ~UnisonEngine() = default;
    
    void prepare(double sampleRate, int samplesPerBlock);
    void setSettings(const UnisonSettings& settings);
    const UnisonSettings& getSettings() const { return settings_; }
    
    // Get detune amounts para cada voz
    std::vector<float> getVoiceDetunes() const;
    std::vector<float> getVoicePans() const;
    std::vector<float> getVoicePhases() const;
    
    // Presets
    void loadPreset(const juce::String& presetName);
    juce::StringArray getPresetList() const;
    
private:
    //==========================================================================
    UnisonSettings settings_;
    double sampleRate_ = 48000.0;
    
    juce::Random random_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnisonEngine)
};

//==============================================================================
/** Preset Morphing - Interpolación entre presets */
class PresetMorpher {
public:
    PresetMorpher() = default;
    ~PresetMorpher() = default;
    
    // Load two presets to morph between
    void setPresetA(const juce::ValueTree& presetA);
    void setPresetB(const juce::ValueTree& presetB);
    
    // Morph amount (0.0 = A, 1.0 = B)
    void setMorphAmount(float amount);
    float getMorphAmount() const { return morphAmount_; }
    
    // Get current morphed state
    juce::ValueTree getCurrentState() const;
    
    // Morph specific parameter
    float morphParameter(const juce::String& paramName) const;
    
private:
    juce::ValueTree presetA_;
    juce::ValueTree presetB_;
    float morphAmount_ = 0.0f;
    
    float interpolateValue(float valueA, float valueB, float amount) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetMorpher)
};

//==============================================================================
/** Harmonic Analyzer - Análisis armónico en tiempo real */
class HarmonicAnalyzer {
public:
    HarmonicAnalyzer();
    ~HarmonicAnalyzer() = default;
    
    void prepare(double sampleRate);
    void reset();
    
    // Analyze buffer
    void analyze(const juce::AudioBuffer<float>& buffer);
    
    // Get harmonic content (fundamental + harmonics)
    struct HarmonicData {
        float fundamental = 0.0f;       // Hz
        std::vector<float> harmonics;   // Magnitudes de cada armónico (hasta 16)
        float totalHarmonicDistortion = 0.0f;  // THD
        float inharmonicity = 0.0f;     // Medida de inharmonicidad
    };
    
    HarmonicData getHarmonicData() const { return harmonicData_; }
    
    // Visualization
    std::vector<float> getHarmonicSpectrum() const;
    
private:
    double sampleRate_ = 48000.0;
    
    static constexpr int fftOrder = 13;
    static constexpr int fftSize = 1 << fftOrder;
    
    juce::dsp::FFT fft_{fftOrder};
    juce::dsp::WindowingFunction<float> window_{fftSize, juce::dsp::WindowingFunction<float>::hann};
    
    std::array<float, fftSize * 2> fftData_{};
    HarmonicData harmonicData_;
    
    float detectFundamental() const;
    void extractHarmonics(float fundamental);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicAnalyzer)
};

//==============================================================================
/** Modulation Presets - Routing comunes */
class ModulationPresets {
public:
    static void applyPreset(ModulationMatrix& matrix, const juce::String& presetName) {
        matrix.clearAllConnections();
        
        if (presetName == "Classic Filter Sweep") {
            matrix.addConnection(ModulationMatrix::ModSource::LFO1, 
                                ModulationMatrix::ModDestination::FilterCutoff, 0.7f);
        }
        else if (presetName == "Wobble Bass") {
            matrix.addConnection(ModulationMatrix::ModSource::LFO1, 
                                ModulationMatrix::ModDestination::FilterCutoff, 0.9f);
            matrix.addConnection(ModulationMatrix::ModSource::LFO1, 
                                ModulationMatrix::ModDestination::FilterResonance, 0.5f);
        }
        else if (presetName == "Vibrato") {
            matrix.addConnection(ModulationMatrix::ModSource::LFO2, 
                                ModulationMatrix::ModDestination::MasterPitch, 0.05f);
            matrix.setConnectionBipolar(0, true);
        }
        else if (presetName == "Tremolo") {
            matrix.addConnection(ModulationMatrix::ModSource::LFO1, 
                                ModulationMatrix::ModDestination::MasterVolume, 0.6f);
        }
        else if (presetName == "Auto Pan") {
            matrix.addConnection(ModulationMatrix::ModSource::LFO1, 
                                ModulationMatrix::ModDestination::MasterPan, 0.8f);
            matrix.setConnectionBipolar(0, true);
        }
        else if (presetName == "Velocity Filter") {
            matrix.addConnection(ModulationMatrix::ModSource::Velocity, 
                                ModulationMatrix::ModDestination::FilterCutoff, 0.5f);
            matrix.addConnection(ModulationMatrix::ModSource::Velocity, 
                                ModulationMatrix::ModDestination::FilterResonance, -0.3f);
        }
        else if (presetName == "Mod Wheel Vibrato") {
            matrix.addConnection(ModulationMatrix::ModSource::ModWheel, 
                                ModulationMatrix::ModDestination::MasterPitch, 0.08f);
            matrix.setConnectionBipolar(0, true);
        }
    }
};

//==============================================================================
/** Unison Presets - Super saws clásicos */
class UnisonPresets {
public:
    static void applyPreset(UnisonEngine& unison, const juce::String& presetName) {
        UnisonEngine::UnisonSettings settings;
        
        if (presetName == "Super Saw") {
            settings.numVoices = 7;
            settings.detune = 0.25f;
            settings.spread = 0.7f;
            settings.blend = 0.5f;
            settings.detuneMode = UnisonEngine::UnisonSettings::DetuneMode::Exponential;
        }
        else if (presetName == "Hyper Saw") {
            settings.numVoices = 16;
            settings.detune = 0.4f;
            settings.spread = 0.9f;
            settings.blend = 0.7f;
            settings.detuneMode = UnisonEngine::UnisonSettings::DetuneMode::Exponential;
        }
        else if (presetName == "Trance Lead") {
            settings.numVoices = 9;
            settings.detune = 0.15f;
            settings.spread = 0.6f;
            settings.blend = 0.4f;
            settings.detuneMode = UnisonEngine::UnisonSettings::DetuneMode::Linear;
        }
        else if (presetName == "Pad") {
            settings.numVoices = 5;
            settings.detune = 0.1f;
            settings.spread = 0.5f;
            settings.blend = 0.3f;
            settings.detuneMode = UnisonEngine::UnisonSettings::DetuneMode::Linear;
        }
        else if (presetName == "Dirty") {
            settings.numVoices = 11;
            settings.detune = 0.6f;
            settings.spread = 0.8f;
            settings.blend = 0.9f;
            settings.detuneMode = UnisonEngine::UnisonSettings::DetuneMode::Power;
        }
        
        settings.randomPhase = true;
        unison.setSettings(settings);
    }
    
    static juce::StringArray getAllPresets() {
        return {
            "Super Saw",
            "Hyper Saw",
            "Trance Lead",
            "Pad",
            "Dirty",
            "Subtle",
            "Wide"
        };
    }
};

} // namespace OmegaStudio
