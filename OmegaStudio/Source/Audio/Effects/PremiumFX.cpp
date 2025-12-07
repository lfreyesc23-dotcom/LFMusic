#include "PremiumFX.h"

namespace OmegaStudio {
namespace FX {

void Emphasis::prepare(const juce::dsp::ProcessSpec& spec) {
    inGain.prepare(spec);
    outGain.prepare(spec);
    low.prepare(spec);
    mid.prepare(spec);
    high.prepare(spec);
    lowX.prepare(spec);
    highX.prepare(spec);
    lowX.setMode(juce::dsp::LadderFilter<float>::Mode::LPF24);
    highX.setMode(juce::dsp::LadderFilter<float>::Mode::HPF24);
    update();
}

void Emphasis::update() {
    inGain.setGainDecibels(settings.inputGainDb);
    outGain.setGainDecibels(settings.outputCeilingDb);
    auto setComp = [&](juce::dsp::Compressor<float>& c, float threshold) {
        c.setThreshold(threshold);
        c.setRatio(settings.ratio);
        c.setAttack(5.0f);
        c.setRelease(50.0f);
    };
    setComp(low, settings.lowThresholdDb);
    setComp(mid, settings.midThresholdDb);
    setComp(high, settings.highThresholdDb);
    lowX.setCutoffFrequencyHz(200.0f);
    highX.setCutoffFrequencyHz(6000.0f);
}

void Emphasis::process(juce::AudioBuffer<float>& buffer) {
    juce::dsp::AudioBlock<float> block(buffer);
    inGain.process(juce::dsp::ProcessContextReplacing<float>(block));

    juce::AudioBuffer<float> lowBuf, midBuf, highBuf;
    lowBuf.makeCopyOf(buffer);
    midBuf.makeCopyOf(buffer);
    highBuf.makeCopyOf(buffer);

    juce::dsp::AudioBlock<float> lowBlock(lowBuf), midBlock(midBuf), highBlock(highBuf);
    lowX.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
    highX.process(juce::dsp::ProcessContextReplacing<float>(highBlock));

    low.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
    mid.process(juce::dsp::ProcessContextReplacing<float>(midBlock));
    high.process(juce::dsp::ProcessContextReplacing<float>(highBlock));

    buffer.clear();
    buffer.addFrom(0, 0, lowBuf, 0, 0, buffer.getNumSamples());
    buffer.addFrom(0, 0, midBuf, 0, 0, buffer.getNumSamples());
    buffer.addFrom(0, 0, highBuf, 0, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 1) {
        buffer.addFrom(1, 0, lowBuf, 1, 0, buffer.getNumSamples());
        buffer.addFrom(1, 0, midBuf, 1, 0, buffer.getNumSamples());
        buffer.addFrom(1, 0, highBuf, 1, 0, buffer.getNumSamples());
    }

    juce::dsp::AudioBlock<float> outBlock(buffer);
    outGain.process(juce::dsp::ProcessContextReplacing<float>(outBlock));
}

void Emphasizer::prepare(const juce::dsp::ProcessSpec& spec) {
    tilt.reset();
    air.reset();
    tilt.prepare(spec);
    air.prepare(spec);
    update();
}

void Emphasizer::update() {
    const double sr = 48000.0; // using static design; could expose
    tilt.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 500.0, 0.7f, juce::Decibels::decibelsToGain(settings.tiltDb));
    air.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 9000.0, 0.7f, juce::Decibels::decibelsToGain(settings.airShelfDb));
}

void Emphasizer::process(juce::AudioBuffer<float>& buffer) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    tilt.process(ctx);
    air.process(ctx);
}

void LuxeVerb::prepare(const juce::dsp::ProcessSpec& spec) {
    reverb.reset();
    reverb.prepare(spec);
    wetGain.prepare(spec);
    dryGain.prepare(spec);
    wetBuffer.setSize((int)spec.numChannels, (int)spec.maximumBlockSize);
    update();
}

void LuxeVerb::update() {
    juce::dsp::Reverb::Parameters p;
    p.roomSize = settings.roomSize;
    p.damping = settings.damping;
    p.wetLevel = settings.wet;
    p.dryLevel = 0.0f;
    p.freezeMode = 0.0f;
    p.width = 1.0f;
    reverb.setParameters(p);
    wetGain.setGainDecibels(juce::Decibels::gainToDecibels(settings.wet));
    dryGain.setGainDecibels(juce::Decibels::gainToDecibels(settings.dry));
}

void LuxeVerb::process(juce::AudioBuffer<float>& buffer) {
    wetBuffer.makeCopyOf(buffer);
    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    reverb.process(juce::dsp::ProcessContextReplacing<float>(wetBlock));
    wetGain.process(juce::dsp::ProcessContextReplacing<float>(wetBlock));
    juce::dsp::AudioBlock<float> dryBlock(buffer);
    dryGain.process(juce::dsp::ProcessContextReplacing<float>(dryBlock));
    buffer.addFrom(0, 0, wetBuffer, 0, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 1)
        buffer.addFrom(1, 0, wetBuffer, 1, 0, buffer.getNumSamples());
}

void PitchShifter::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate = spec.sampleRate;
    for (auto& i : interpolators) i.reset();
}

void PitchShifter::process(juce::AudioBuffer<float>& buffer) {
    const double ratio = std::pow(2.0, settings.semitones / 12.0);
    if (ratio == 1.0) return;
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        juce::AudioBuffer<float> tmp(1, numSamples);
        auto* src = buffer.getReadPointer(ch);
        auto* dst = tmp.getWritePointer(0);
        int outSamples = interpolators[ch].process(ratio, src, dst, numSamples);
        buffer.copyFrom(ch, 0, tmp, 0, 0, std::min(outSamples, numSamples));
        if (outSamples < numSamples)
            buffer.clear(ch, outSamples, numSamples - outSamples);
    }
}

void TransientProcessor::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate = spec.sampleRate;
    envelope.assign((size_t)spec.maximumBlockSize, 0.0f);
}

void TransientProcessor::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    const float sens = settings.sensitivity;
    const float atk = settings.attack;
    const float sus = settings.sustain;

    for (int i = 0; i < numSamples; ++i) {
        float env = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            env += std::abs(buffer.getSample(ch, i));
        env /= (float)numCh;
        envelope[(size_t)i] = 0.9f * (i > 0 ? envelope[(size_t)(i - 1)] : 0.0f) + 0.1f * env;
    }

    for (int i = 1; i < numSamples; ++i) {
        float delta = envelope[(size_t)i] - envelope[(size_t)(i - 1)];
        float gain = 1.0f;
        if (delta > sens * 0.001f)
            gain = atk;
        else
            gain = sus;
        for (int ch = 0; ch < numCh; ++ch) {
            buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
        }
    }
}

void GrossBeatLite::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate = spec.sampleRate;
    if (pattern.empty()) pattern = {1.0f, 0.5f, 1.0f, 0.0f};
}

void GrossBeatLite::setPattern(const std::vector<float>& patternBeats) {
    pattern = patternBeats;
}

void GrossBeatLite::process(juce::AudioBuffer<float>& buffer, double bpm) {
    if (pattern.empty()) return;
    const int numSamples = buffer.getNumSamples();
    const double samplesPerBeat = (60.0 / bpm) * sampleRate;
    for (int i = 0; i < numSamples; ++i) {
        double beatPos = (double)i / samplesPerBeat;
        int idx = (int)std::fmod(beatPos, (double)pattern.size());
        float gate = juce::jlimit(0.0f, 1.0f, pattern[(size_t)idx]);
        float g = 1.0f - settings.gateDepth * (1.0f - gate);
        g *= settings.rate;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, i, buffer.getSample(ch, i) * g);
    }
}

} // namespace FX
} // namespace OmegaStudio
