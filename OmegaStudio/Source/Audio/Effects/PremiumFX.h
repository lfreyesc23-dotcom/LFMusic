#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace FX {

struct EmphasisSettings {
    float inputGainDb = 0.0f;
    float lowThresholdDb = -24.0f;
    float midThresholdDb = -18.0f;
    float highThresholdDb = -12.0f;
    float ratio = 3.0f;
    float outputCeilingDb = -0.5f;
};

class Emphasis {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(const EmphasisSettings& s) { settings = s; update(); }
    void process(juce::AudioBuffer<float>& buffer);
private:
    EmphasisSettings settings;
    juce::dsp::Compressor<float> low, mid, high;
    juce::dsp::LadderFilter<float> lowX, highX;
    juce::dsp::Gain<float> inGain, outGain;
    void update();
};

struct EmphasizerSettings {
    float tiltDb = 2.0f; // + bright
    float airShelfDb = 1.5f;
};

class Emphasizer {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(const EmphasizerSettings& s) { settings = s; update(); }
    void process(juce::AudioBuffer<float>& buffer);
private:
    EmphasizerSettings settings;
    juce::dsp::IIR::Filter<float> tilt;
    juce::dsp::IIR::Filter<float> air;
    void update();
};

struct LuxeVerbSettings {
    float roomSize = 0.35f;
    float damping = 0.3f;
    float wet = 0.2f;
    float dry = 0.8f;
};

class LuxeVerb {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(const LuxeVerbSettings& s) { settings = s; update(); }
    void process(juce::AudioBuffer<float>& buffer);
private:
    LuxeVerbSettings settings;
    juce::dsp::Reverb reverb;
    juce::dsp::Gain<float> wetGain, dryGain;
    juce::AudioBuffer<float> wetBuffer;
    void update();
};

struct PitchShifterSettings {
    float semitones = 0.0f;
};

class PitchShifter {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(const PitchShifterSettings& s) { settings = s; }
    void process(juce::AudioBuffer<float>& buffer);
private:
    PitchShifterSettings settings;
    juce::LagrangeInterpolator interpolators[2];
    double sampleRate = 44100.0;
};

struct TransientProcessorSettings {
    float attack = 1.2f;  // >1 boosts attacks
    float sustain = 0.9f; // <1 reduces tails
    float sensitivity = 0.5f;
};

class TransientProcessor {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setParameters(const TransientProcessorSettings& s) { settings = s; }
    void process(juce::AudioBuffer<float>& buffer);
private:
    TransientProcessorSettings settings;
    std::vector<float> envelope;
    double sampleRate = 44100.0;
};

struct GrossBeatSettings {
    float gateDepth = 0.7f;
    float rate = 1.0f; // playback rate
};

class GrossBeatLite {
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void setPattern(const std::vector<float>& patternBeats); // values 0..1 per step
    void setParameters(const GrossBeatSettings& s) { settings = s; }
    void process(juce::AudioBuffer<float>& buffer, double bpm);
private:
    GrossBeatSettings settings;
    std::vector<float> pattern; // one bar
    double sampleRate = 44100.0;
};

} // namespace FX
} // namespace OmegaStudio
