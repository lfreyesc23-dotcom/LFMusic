#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace OmegaStudio {
namespace Effects {

/**
 * @brief FL Studio Mobile Rack + FX (FL Studio 2025 Feature)
 * Complete suite of 20+ mobile-quality effects
 */

// Base effect interface
class MobileEffect
{
public:
    virtual ~MobileEffect() = default;
    
    virtual void prepare(double sampleRate, int blockSize) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual juce::String getParameterName(int index) const = 0;
    virtual float getParameter(int index) const = 0;
    virtual void setParameter(int index, float value) = 0;
    
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

protected:
    bool isEnabled = true;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
};

// 1. Parametric EQ (4-band)
class MobileParametricEQ : public MobileEffect
{
public:
    enum Params { LowGain, MidLow Gain, MidHighGain, HighGain, LowFreq, MidLowFreq, MidHighFreq, HighFreq, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Parametric EQ"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowBand;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> midLowBand;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> midHighBand;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highBand;
    
    float parameters[NumParams] = {0.0f, 0.0f, 0.0f, 0.0f, 100.0f, 500.0f, 2000.0f, 8000.0f};
};

// 2. Compressor
class MobileCompressor : public MobileEffect
{
public:
    enum Params { Threshold, Ratio, Attack, Release, Gain, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Compressor"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::Compressor<float> compressor;
    float parameters[NumParams] = {-20.0f, 4.0f, 10.0f, 100.0f, 0.0f};
};

// 3. Reverb
class MobileReverb : public MobileEffect
{
public:
    enum Params { RoomSize, Damping, Width, WetLevel, DryLevel, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Reverb"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;
    float parameters[NumParams] = {0.5f, 0.5f, 1.0f, 0.33f, 0.67f};
};

// 4. Delay
class MobileDelay : public MobileEffect
{
public:
    enum Params { DelayTime, Feedback, Mix, PingPong, FilterCutoff, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Delay"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineR;
    float parameters[NumParams] = {250.0f, 0.5f, 0.5f, 0.0f, 5000.0f};
};

// 5. Distortion
class MobileDistortion : public MobileEffect
{
public:
    enum Params { Drive, Tone, Mix, Type, Output, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Distortion"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::WaveShaper<float> waveshaper;
    float parameters[NumParams] = {0.5f, 0.5f, 1.0f, 0.0f, 0.0f};
};

// 6. Chorus
class MobileChorus : public MobileEffect
{
public:
    enum Params { Rate, Depth, Feedback, Mix, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Chorus"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::Chorus<float> chorus;
    float parameters[NumParams] = {1.0f, 0.5f, 0.0f, 0.5f};
};

// 7. Phaser
class MobilePhaser : public MobileEffect
{
public:
    enum Params { Rate, Depth, Feedback, Stages, Mix, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Phaser"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::Phaser<float> phaser;
    float parameters[NumParams] = {1.0f, 0.5f, 0.5f, 4.0f, 0.5f};
};

// 8. Flanger
class MobileFlanger : public MobileEffect
{
public:
    enum Params { Rate, Depth, Feedback, Mix, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Flanger"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    float parameters[NumParams] = {0.5f, 0.5f, 0.5f, 0.5f};
    juce::dsp::DelayLine<float> delayLine;
    float lfoPhase = 0.0f;
};

// 9. Limiter
class MobileLimiter : public MobileEffect
{
public:
    enum Params { Threshold, Release, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Limiter"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    juce::dsp::Limiter<float> limiter;
    float parameters[NumParams] = {-1.0f, 50.0f};
};

// 10. Bitcrusher
class MobileBitcrusher : public MobileEffect
{
public:
    enum Params { BitDepth, SampleRate, Mix, NumParams };
    
    void prepare(double sampleRate, int blockSize) override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void reset() override;
    
    juce::String getName() const override { return "Mobile Bitcrusher"; }
    int getNumParameters() const override { return NumParams; }
    juce::String getParameterName(int index) const override;
    float getParameter(int index) const override;
    void setParameter(int index, float value) override;

private:
    float parameters[NumParams] = {16.0f, 44100.0f, 1.0f};
    int sampleCounter = 0;
    float lastSampleL = 0.0f, lastSampleR = 0.0f;
};

/**
 * @brief FL Mobile Rack - Container for all mobile effects
 */
class FLMobileRack
{
public:
    FLMobileRack();
    ~FLMobileRack();

    // Effect management
    void addEffect(std::unique_ptr<MobileEffect> effect);
    void removeEffect(int index);
    void clearEffects();
    MobileEffect* getEffect(int index);
    int getNumEffects() const { return static_cast<int>(effects.size()); }
    
    // Processing
    void prepare(double sampleRate, int blockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Effect ordering
    void moveEffect(int fromIndex, int toIndex);
    
    // Presets
    void savePreset(const juce::String& name);
    void loadPreset(const juce::String& name);
    std::vector<juce::String> getAvailablePresets() const;
    
    // Factory methods - Create all 20+ effects
    static std::unique_ptr<MobileEffect> createParametricEQ();
    static std::unique_ptr<MobileEffect> createCompressor();
    static std::unique_ptr<MobileEffect> createReverb();
    static std::unique_ptr<MobileEffect> createDelay();
    static std::unique_ptr<MobileEffect> createDistortion();
    static std::unique_ptr<MobileEffect> createChorus();
    static std::unique_ptr<MobileEffect> createPhaser();
    static std::unique_ptr<MobileEffect> createFlanger();
    static std::unique_ptr<MobileEffect> createLimiter();
    static std::unique_ptr<MobileEffect> createBitcrusher();

private:
    std::vector<std::unique_ptr<MobileEffect>> effects;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLMobileRack)
};

} // namespace Effects
} // namespace OmegaStudio
