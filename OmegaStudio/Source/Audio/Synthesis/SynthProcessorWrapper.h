//==============================================================================
// SynthProcessorWrapper.h
// Wrappers para integrar los nuevos sintetizadores con JUCE AudioProcessor
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "WavetableSynth.h"
#include "FMSynth.h"
#include "VirtualAnalogSynth.h"
#include "AdvancedSampler.h"

namespace OmegaStudio {

//==============================================================================
// Wrapper base para convertir Synthesiser en AudioProcessor
//==============================================================================
class SynthProcessorBase : public juce::AudioProcessor {
public:
    SynthProcessorBase(const juce::String& name) 
        : AudioProcessor(BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
        setName(name);
    }
    
    virtual ~SynthProcessorBase() override = default;
    
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = 2;
        prepareSynth(spec);
    }
    
    void releaseResources() override {}
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        buffer.clear();
        renderSynth(buffer, midiMessages, 0, buffer.getNumSamples());
    }
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    const juce::String getName() const override { return name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    
protected:
    virtual void prepareSynth(const juce::dsp::ProcessSpec& spec) = 0;
    virtual void renderSynth(juce::AudioBuffer<float>& buffer, 
                            juce::MidiBuffer& midi,
                            int startSample, 
                            int numSamples) = 0;
    
    juce::String name;
};

//==============================================================================
// WavetableSynth Processor Wrapper
//==============================================================================
class WavetableSynthProcessor : public SynthProcessorBase {
public:
    WavetableSynthProcessor() : SynthProcessorBase("Omega Wavetable") {}
    
    WavetableSynth& getSynth() { return synth; }
    
protected:
    void prepareSynth(const juce::dsp::ProcessSpec& spec) override {
        synth.prepare(spec);
    }
    
    void renderSynth(juce::AudioBuffer<float>& buffer, 
                    juce::MidiBuffer& midi,
                    int startSample, 
                    int numSamples) override {
        synth.renderNextBlock(buffer, midi, startSample, numSamples);
    }
    
private:
    WavetableSynth synth;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSynthProcessor)
};

//==============================================================================
// FMSynth Processor Wrapper
//==============================================================================
class FMSynthProcessor : public SynthProcessorBase {
public:
    FMSynthProcessor() : SynthProcessorBase("Omega FM") {}
    
    FMSynth& getSynth() { return synth; }
    
protected:
    void prepareSynth(const juce::dsp::ProcessSpec& spec) override {
        synth.prepare(spec);
    }
    
    void renderSynth(juce::AudioBuffer<float>& buffer, 
                    juce::MidiBuffer& midi,
                    int startSample, 
                    int numSamples) override {
        synth.renderNextBlock(buffer, midi, startSample, numSamples);
    }
    
private:
    FMSynth synth;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynthProcessor)
};

//==============================================================================
// VirtualAnalogSynth Processor Wrapper
//==============================================================================
class VirtualAnalogSynthProcessor : public SynthProcessorBase {
public:
    VirtualAnalogSynthProcessor() : SynthProcessorBase("Omega Analog") {}
    
    VirtualAnalogSynth& getSynth() { return synth; }
    
protected:
    void prepareSynth(const juce::dsp::ProcessSpec& spec) override {
        synth.prepare(spec);
    }
    
    void renderSynth(juce::AudioBuffer<float>& buffer, 
                    juce::MidiBuffer& midi,
                    int startSample, 
                    int numSamples) override {
        synth.renderNextBlock(buffer, midi, startSample, numSamples);
    }
    
private:
    VirtualAnalogSynth synth;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualAnalogSynthProcessor)
};

//==============================================================================
// AdvancedSampler Processor Wrapper
//==============================================================================
class AdvancedSamplerProcessor : public SynthProcessorBase {
public:
    AdvancedSamplerProcessor() : SynthProcessorBase("Omega Sampler") {}
    
    AdvancedSampler& getSampler() { return sampler; }
    
protected:
    void prepareSynth(const juce::dsp::ProcessSpec& spec) override {
        sampler.prepare(spec);
    }
    
    void renderSynth(juce::AudioBuffer<float>& buffer, 
                    juce::MidiBuffer& midi,
                    int startSample, 
                    int numSamples) override {
        sampler.renderNextBlock(buffer, midi, startSample, numSamples);
    }
    
private:
    AdvancedSampler sampler;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedSamplerProcessor)
};

} // namespace OmegaStudio
