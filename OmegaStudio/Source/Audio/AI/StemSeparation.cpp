/*
  ==============================================================================

    StemSeparation.cpp

  ==============================================================================
*/

#include "StemSeparation.h"
#include "OnnxRuntimeWrapper.h"

#include <cmath>

namespace omega {
namespace AI {

namespace {
inline float clamp01(float v) { return juce::jlimit(0.0f, 1.0f, v); }
}

StemSeparator::StemSeparator() = default;

bool StemSeparator::separateStems(const juce::AudioBuffer<float>& input,
                 std::map<StemType, juce::AudioBuffer<float>>& outputs,
                 std::function<void(float)> progressCallback) {
  const int numSamples = input.getNumSamples();
  const int numChannels = input.getNumChannels();
  if (numSamples == 0 || numChannels == 0)
    return false;

  progress_.store(0.0f);

  const float vocalQ = (config_.quality == QualityMode::Best) ? 0.8f : (config_.quality == QualityMode::Balanced ? 1.1f : 1.4f);
  const float bandQ = (config_.quality == QualityMode::Best) ? 0.9f : 1.2f;

  auto updateProgress = [&](float v) {
    progress_.store(clamp01(v));
    if (progressCallback)
      progressCallback(progress_.load());
  };

  // Try ONNX Runtime path first if available
#ifdef ENABLE_ORT
  if (config_.preferOnnx && config_.modelPath.existsAsFile()) {
    if (!onnxSession_)
      onnxSession_ = std::make_unique<OnnxStemSession>();
    if (!onnxSession_->isLoaded())
      onnxSession_->loadModel(config_.modelPath);

    if (onnxSession_->isLoaded()) {
      auto res = onnxSession_->run(input, progressCallback);
      if (res.success) {
        for (const auto& [name, buf] : res.stems) {
          auto toStem = [](const juce::String& n) {
            if (n == "vocals") return StemType::Vocals;
            if (n == "drums") return StemType::Drums;
            if (n == "bass") return StemType::Bass;
            if (n == "guitar") return StemType::Guitar;
            if (n == "keys") return StemType::Keys;
            return StemType::Other;
          };
          outputs[toStem(name)] = buf;
        }
        updateProgress(1.0f);
        return true;
      }
    }
  }
#endif

  // Fallback DSP-only separation
  // Vocals: band-pass 200-8k focus
  juce::AudioBuffer<float> vocals(numChannels, numSamples);
  processBandPass(input, vocals, 2500.0f, vocalQ);
  if (config_.normalizeOutput)
    normalizeBuffer(vocals);
  outputs[StemType::Vocals] = vocals;
  updateProgress(0.25f);

  // Drums: transient emphasis + high-pass to reduce rumble
  juce::AudioBuffer<float> drums(numChannels, numSamples);
  drums.makeCopyOf(input);
  processHighPass(drums, drums, 80.0f);
  applyTransientEmphasis(drums);
  if (config_.normalizeOutput)
    normalizeBuffer(drums);
  outputs[StemType::Drums] = drums;
  updateProgress(0.5f);

  // Bass: low-pass around 180-250 Hz
  juce::AudioBuffer<float> bass(numChannels, numSamples);
  processLowPass(input, bass, 220.0f);
  if (config_.normalizeOutput)
    normalizeBuffer(bass);
  outputs[StemType::Bass] = bass;
  updateProgress(0.7f);

  // Guitar/Keys mid band
  juce::AudioBuffer<float> guitars(numChannels, numSamples);
  processBandPass(input, guitars, 1600.0f, bandQ);
  if (config_.normalizeOutput)
    normalizeBuffer(guitars);
  outputs[StemType::Guitar] = guitars;

  juce::AudioBuffer<float> keys(numChannels, numSamples);
  processBandPass(input, keys, 1200.0f, bandQ);
  if (config_.normalizeOutput)
    normalizeBuffer(keys);
  outputs[StemType::Keys] = keys;
  updateProgress(0.85f);

  // Residual other = input - (vocals + drums + bass) to avoid double energy
  juce::AudioBuffer<float> other(numChannels, numSamples);
  other.makeCopyOf(input);
  auto subtractStem = [&](const juce::AudioBuffer<float>& stem, float gain) {
    for (int ch = 0; ch < numChannels; ++ch)
      other.addFrom(ch, 0, stem, ch, 0, numSamples, -gain);
  };
  subtractStem(vocals, 0.6f);
  subtractStem(drums, 0.6f);
  subtractStem(bass, 0.6f);
  if (config_.normalizeOutput)
    normalizeBuffer(other);
  outputs[StemType::Other] = other;
  updateProgress(1.0f);

  return true;
}

void StemSeparator::processBandPass(const juce::AudioBuffer<float>& input,
                  juce::AudioBuffer<float>& output,
                  float centerHz,
                  float q) {
  output.makeCopyOf(input);
  juce::dsp::IIR::Filter<float> filter;
  filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(config_.sampleRate, centerHz, q);
  juce::dsp::ProcessSpec spec{ config_.sampleRate, static_cast<juce::uint32>(input.getNumSamples()), static_cast<juce::uint32>(input.getNumChannels()) };
  filter.prepare(spec);
  juce::dsp::AudioBlock<float> block(output);
  juce::dsp::ProcessContextReplacing<float> context(block);
  filter.process(context);
}

void StemSeparator::processLowPass(const juce::AudioBuffer<float>& input,
                   juce::AudioBuffer<float>& output,
                   float cutoffHz) {
  output.makeCopyOf(input);
  juce::dsp::IIR::Filter<float> filter;
  filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(config_.sampleRate, cutoffHz);
  juce::dsp::ProcessSpec spec{ config_.sampleRate, static_cast<juce::uint32>(input.getNumSamples()), static_cast<juce::uint32>(input.getNumChannels()) };
  filter.prepare(spec);
  juce::dsp::AudioBlock<float> block(output);
  juce::dsp::ProcessContextReplacing<float> context(block);
  filter.process(context);
}

void StemSeparator::processHighPass(const juce::AudioBuffer<float>& input,
                  juce::AudioBuffer<float>& output,
                  float cutoffHz) {
  output.makeCopyOf(input);
  juce::dsp::IIR::Filter<float> filter;
  filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(config_.sampleRate, cutoffHz);
  juce::dsp::ProcessSpec spec{ config_.sampleRate, static_cast<juce::uint32>(input.getNumSamples()), static_cast<juce::uint32>(input.getNumChannels()) };
  filter.prepare(spec);
  juce::dsp::AudioBlock<float> block(output);
  juce::dsp::ProcessContextReplacing<float> context(block);
  filter.process(context);
}

void StemSeparator::applyTransientEmphasis(juce::AudioBuffer<float>& buffer) {
  const int numSamples = buffer.getNumSamples();
  const int numChannels = buffer.getNumChannels();
  const float drive = (config_.quality == QualityMode::Best) ? 2.5f : 2.0f;
  for (int ch = 0; ch < numChannels; ++ch) {
    float prev = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
      const float x = buffer.getSample(ch, i);
      const float diff = x - prev;
      const float transient = std::tanh(diff * drive);
      buffer.setSample(ch, i, juce::jlimit(-1.0f, 1.0f, x + transient * 0.6f));
      prev = x;
    }
  }
}

void StemSeparator::normalizeBuffer(juce::AudioBuffer<float>& buffer) {
  const float mag = buffer.getMagnitude(0, buffer.getNumSamples());
  if (mag > 0.0001f)
    buffer.applyGain(1.0f / mag);
}

} // namespace AI
} // namespace omega
