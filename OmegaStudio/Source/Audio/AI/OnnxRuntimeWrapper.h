#pragma once

#include <JuceHeader.h>
#include <functional>
#include <map>
#include <vector>

namespace omega {
namespace AI {

struct OnnxInferenceResult {
    std::map<juce::String, juce::AudioBuffer<float>> stems;
    bool success = false;
};

class OnnxStemSession {
public:
    OnnxStemSession();
    ~OnnxStemSession();

    bool loadModel(const juce::File& modelPath);
    bool isLoaded() const;

    OnnxInferenceResult run(const juce::AudioBuffer<float>& input,
                            std::function<void(float)> progress = {});

private:
#ifdef ENABLE_ORT
    bool prepareIo(const juce::AudioBuffer<float>& input);

    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::SessionOptions> options;
    std::unique_ptr<Ort::Session> session;
    std::vector<const char*> inputNames;
    std::vector<const char*> outputNames;
    std::vector<int64_t> inputShape;
    std::vector<int64_t> outputShape;
#endif
};

} // namespace AI
} // namespace omega
