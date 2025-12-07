/*
  ==============================================================================

    StemSeparation.h
    AI-powered stem separation (vocals/drums/bass/other)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <map>

namespace omega {
namespace AI {

enum class StemType {
  Vocals,
  Drums,
  Bass,
  Guitar,
  Keys,
  Other
};

enum class QualityMode {
  Fast,
  Balanced,
  Best
};

struct SeparationConfig {
  double sampleRate = 44100.0;
  bool normalizeOutput = true;
  QualityMode quality = QualityMode::Balanced;
  bool preferOnnx = false;
  juce::File modelPath;
};

class StemSeparator {
public:
  StemSeparator();

  void setConfig(const SeparationConfig& config) { config_ = config; }
  void setModelPath(const juce::File& path) { config_.modelPath = path; }
  void preferOnnxRuntime(bool enabled) { config_.preferOnnx = enabled; }

  bool separateStems(const juce::AudioBuffer<float>& input,
             std::map<StemType, juce::AudioBuffer<float>>& outputs,
             std::function<void(float)> progressCallback = {});

  void setQualityMode(QualityMode mode) { config_.quality = mode; }
  float getProgress() const { return progress_.load(); }

private:
  void processBandPass(const juce::AudioBuffer<float>& input,
             juce::AudioBuffer<float>& output,
             float centerHz,
             float q);

  void processLowPass(const juce::AudioBuffer<float>& input,
            juce::AudioBuffer<float>& output,
            float cutoffHz);

  void processHighPass(const juce::AudioBuffer<float>& input,
             juce::AudioBuffer<float>& output,
             float cutoffHz);

  void applyTransientEmphasis(juce::AudioBuffer<float>& buffer);
  void normalizeBuffer(juce::AudioBuffer<float>& buffer);

  SeparationConfig config_{};
  std::atomic<float> progress_{0.0f};

#ifdef ENABLE_ORT
  class OnnxStemSession;
  std::unique_ptr<OnnxStemSession> onnxSession_;
#endif
};

} // namespace AI
} // namespace omega
