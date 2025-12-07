#pragma once
#include <JuceHeader.h>

namespace OmegaStudio {
namespace Effects {

/**
 * @brief Emphasis - Multi-stage Mastering Compressor/Limiter (FL Studio 2025)
 */
class Emphasis : public juce::AudioProcessor
{
public:
    Emphasis();
    ~Emphasis() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "Emphasis"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::dsp::Compressor<float> stage1, stage2, stage3;
    juce::dsp::Limiter<float> limiter;
    
    float threshold = -6.0f;
    float ratio = 4.0f;
    float attack = 10.0f;
    float release = 100.0f;
    float outputGain = 0.0f;
};

/**
 * @brief Emphasizer - Presence/Clarity Enhancer (FL Studio 2025.2)
 */
class Emphasizer : public juce::AudioProcessor
{
public:
    Emphasizer();
    ~Emphasizer() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "Emphasizer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> midFilter, highFilter;
    float midGain = 0.0f;
    float highGain = 0.0f;
};

/**
 * @brief LuxeVerb - High-Quality Reverb (FL Studio 2025)
 */
class LuxeVerb : public juce::AudioProcessor
{
public:
    LuxeVerb();
    ~LuxeVerb() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "LuxeVerb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters params;
};

/**
 * @brief Gross Beat - Time and Volume Effects (FL Studio)
 */
class GrossBeat : public juce::AudioProcessor
{
public:
    GrossBeat();
    ~GrossBeat() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "Gross Beat"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    std::vector<float> buffer;
    int writePosition = 0;
    float playbackSpeed = 1.0f;
    float volumeModulation = 1.0f;
};

/**
 * @brief Vocodex - Advanced Vocoder (FL Studio)
 */
class Vocodex : public juce::AudioProcessor
{
public:
    Vocodex();
    ~Vocodex() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "Vocodex"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    static constexpr int NumBands = 100;
    juce::dsp::FFT fft{11};
    std::vector<float> fftData;
    std::array<juce::dsp::IIR::Filter<float>, NumBands> carrierFilters;
    std::array<juce::dsp::IIR::Filter<float>, NumBands> modulatorFilters;
};

/**
 * @brief Maximus - Multiband Compressor/Maximizer (FL Studio)
 */
class Maximus : public juce::AudioProcessor
{
public:
    Maximus();
    ~Maximus() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "Maximus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::dsp::Compressor<float> lowBandComp, midBandComp, highBandComp;
    juce::dsp::LinkwitzRileyFilter<float> lowHighCrossover, midHighCrossover;
    float lowFreq = 120.0f;
    float highFreq = 6000.0f;
};

/**
 * @brief Pitcher - Real-time Pitch Correction (FL Studio)
 */
class Pitcher : public juce::AudioProcessor
{
public:
    Pitcher();
    ~Pitcher() override;
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return "Pitcher"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::String targetKey = "C";
    juce::String targetScale = "Major";
    float correctionSpeed = 50.0f;
    bool autoTuneEnabled = true;
};

} // namespace Effects
} // namespace OmegaStudio
