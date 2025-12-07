#pragma once
#include <JuceHeader.h>

namespace OmegaStudio {
namespace Instruments {

/**
 * @brief Sytrus - FM/Additive/Subtractive Synthesizer (FL Studio)
 */
class Sytrus : public juce::Synthesiser
{
public:
    Sytrus();
    ~Sytrus() override;
    
    struct Operator {
        enum class Waveform { Sine, Saw, Square, Triangle, Noise, Custom };
        
        Waveform waveform = Waveform::Sine;
        float level = 1.0f;
        float ratio = 1.0f;
        float fine = 0.0f;
        float feedback = 0.0f;
        
        juce::ADSR::Parameters envelope;
        
        // FM modulation matrix
        std::array<float, 6> modulation = {0.0f};
    };
    
    void setOperator(int index, const Operator& op);
    Operator getOperator(int index) const;
    
    void setAlgorithm(int algorithm);
    int getAlgorithm() const { return currentAlgorithm; }
    
    static constexpr int NumOperators = 6;
    static constexpr int NumAlgorithms = 32;

private:
    std::array<Operator, NumOperators> operators;
    int currentAlgorithm = 0;
    double currentSampleRate = 44100.0;
};

/**
 * @brief Harmor - Advanced Additive Synthesizer (FL Studio)
 */
class Harmor : public juce::Synthesiser
{
public:
    Harmor();
    ~Harmor() override;
    
    void setResynthesisEnabled(bool enabled) { resynthesis = enabled; }
    bool getResynthesisEnabled() const { return resynthesis; }
    
    void setHarmonicCount(int count) { harmonicCount = juce::jlimit(1, 516, count); }
    int getHarmonicCount() const { return harmonicCount; }
    
    void setHarmonicLevel(int harmonic, float level);
    float getHarmonicLevel(int harmonic) const;
    
    void setSampleForResynthesis(const juce::AudioBuffer<float>& sample);

private:
    bool resynthesis = false;
    int harmonicCount = 256;
    std::array<float, 516> harmonicLevels;
    juce::AudioBuffer<float> resynthBuffer;
};

/**
 * @brief FLEX - Preset-Based Synthesizer (FL Studio)
 */
class FLEX : public juce::Synthesiser
{
public:
    FLEX();
    ~FLEX() override;
    
    struct Preset {
        juce::String name;
        juce::String category;
        juce::ValueTree data;
    };
    
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    std::vector<juce::String> getPresetCategories() const;
    std::vector<juce::String> getPresetsInCategory(const juce::String& category) const;
    
    // Macro controls
    void setMacro(int macroNumber, float value);
    float getMacro(int macroNumber) const;
    static constexpr int NumMacros = 8;

private:
    std::map<juce::String, Preset> presets;
    std::array<float, NumMacros> macros;
    
    void initializePresets();
};

/**
 * @brief Sakura - Physical Modeling String Synthesizer (FL Studio)
 */
class Sakura : public juce::Synthesiser
{
public:
    Sakura();
    ~Sakura() override;
    
    enum class StringType {
        Guitar,
        Bass,
        Violin,
        Cello,
        Koto,
        Sitar,
        Harp
    };
    
    void setStringType(StringType type);
    StringType getStringType() const { return stringType; }
    
    void setPluckPosition(float position) { pluckPosition = juce::jlimit(0.0f, 1.0f, position); }
    float getPluckPosition() const { return pluckPosition; }
    
    void setStringTension(float tension) { stringTension = juce::jlimit(0.0f, 1.0f, tension); }
    float getStringTension() const { return stringTension; }

private:
    StringType stringType = StringType::Guitar;
    float pluckPosition = 0.5f;
    float stringTension = 0.5f;
    
    // Physical modeling parameters
    struct String {
        std::vector<float> delayLine;
        int writePos = 0;
        float damping = 0.995f;
    };
    std::array<String, 128> strings;  // One per MIDI note
};

/**
 * @brief Transistor Bass - Analog Bass Synthesizer (FL Studio)
 */
class TransistorBass : public juce::Synthesiser
{
public:
    TransistorBass();
    ~TransistorBass() override;
    
    struct Sequencer {
        std::array<bool, 16> steps;
        std::array<uint8, 16> accents;
        std::array<bool, 16> slides;
        int currentStep = 0;
    };
    
    void setSequencerEnabled(bool enabled) { sequencerEnabled = enabled; }
    bool getSequencerEnabled() const { return sequencerEnabled; }
    
    void setSequencerStep(int step, bool active, uint8 accent = 100, bool slide = false);
    Sequencer& getSequencer() { return sequencer; }
    
    void setDistortion(float amount) { distortion = juce::jlimit(0.0f, 1.0f, amount); }
    float getDistortion() const { return distortion; }
    
    void setResonance(float reso) { resonance = juce::jlimit(0.0f, 1.0f, reso); }
    float getResonance() const { return resonance; }

private:
    Sequencer sequencer;
    bool sequencerEnabled = false;
    float distortion = 0.0f;
    float resonance = 0.5f;
    
    juce::dsp::LadderFilter<float> filter;
    juce::dsp::Oscillator<float> oscillator;
};

/**
 * @brief Drumaxx - Physical Modeling Drum Synthesizer (FL Studio)
 */
class Drumaxx : public juce::Synthesiser
{
public:
    Drumaxx();
    ~Drumaxx() override;
    
    enum class DrumType {
        Kick,
        Snare,
        Tom,
        Cymbal,
        HiHat,
        Percussion
    };
    
    struct DrumPad {
        DrumType type = DrumType::Kick;
        float size = 0.5f;
        float tension = 0.5f;
        float damping = 0.5f;
        float material = 0.5f;  // Wood to metal
        int midiNote = 36;
    };
    
    void setPad(int padIndex, const DrumPad& pad);
    DrumPad getPad(int padIndex) const;
    static constexpr int NumPads = 16;

private:
    std::array<DrumPad, NumPads> pads;
    
    void generateKick(juce::AudioBuffer<float>& buffer, const DrumPad& pad);
    void generateSnare(juce::AudioBuffer<float>& buffer, const DrumPad& pad);
};

/**
 * @brief Toxic Biohazard - Hybrid FM/Subtractive Synth (FL Studio)
 */
class ToxicBiohazard : public juce::Synthesiser
{
public:
    ToxicBiohazard();
    ~ToxicBiohazard() override;
    
    struct Voice {
        enum class Mode { Subtractive, FM, Hybrid };
        Mode mode = Mode::Hybrid;
        
        // Oscillators
        float osc1Level = 1.0f;
        float osc2Level = 0.5f;
        float fmAmount = 0.0f;
        
        // Filter
        float cutoff = 1000.0f;
        float resonance = 0.0f;
        
        // Effects
        float distortion = 0.0f;
        float chorus = 0.0f;
    };
    
    void setVoice(const Voice& voice) { currentVoice = voice; }
    Voice getVoice() const { return currentVoice; }
    
    static constexpr int NumPresets = 1024;

private:
    Voice currentVoice;
};

/**
 * @brief Poizone - Subtractive Synthesizer (FL Studio)
 */
class Poizone : public juce::Synthesiser
{
public:
    Poizone();
    ~Poizone() override;
    
    void setUnison(int voices) { unisonVoices = juce::jlimit(1, 8, voices); }
    int getUnison() const { return unisonVoices; }
    
    void setDetune(float amount) { detune = juce::jlimit(0.0f, 1.0f, amount); }
    float getDetune() const { return detune; }

private:
    int unisonVoices = 1;
    float detune = 0.0f;
};

/**
 * @brief Morphine - Additive Synthesizer (FL Studio)
 */
class Morphine : public juce::Synthesiser
{
public:
    Morphine();
    ~Morphine() override;
    
    void setPartialCount(int count) { partialCount = juce::jlimit(1, 256, count); }
    int getPartialCount() const { return partialCount; }
    
    void setMorphing(float amount) { morphing = juce::jlimit(0.0f, 1.0f, amount); }
    float getMorphing() const { return morphing; }

private:
    int partialCount = 64;
    float morphing = 0.0f;
};

} // namespace Instruments
} // namespace OmegaStudio
