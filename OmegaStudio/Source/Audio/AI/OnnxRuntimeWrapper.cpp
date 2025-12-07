#include "OnnxRuntimeWrapper.h"

#ifdef ENABLE_ORT
 #include <onnxruntime_cxx_api.h>
#endif

namespace omega {
namespace AI {

OnnxStemSession::OnnxStemSession() = default;
OnnxStemSession::~OnnxStemSession() = default;

bool OnnxStemSession::loadModel(const juce::File& modelPath) {
#ifdef ENABLE_ORT
    if (!modelPath.existsAsFile())
        return false;

    try {
        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "OmegaStudio");
        options = std::make_unique<Ort::SessionOptions>();
        options->SetIntraOpNumThreads(4);
        options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        auto utf8 = modelPath.getFullPathName().toStdString();
        session = std::make_unique<Ort::Session>(*env, utf8.c_str(), *options);

        // Basic IO discovery (single input/output expected)
        Ort::AllocatorWithDefaultOptions allocator;
        size_t inputCount = session->GetInputCount();
        size_t outputCount = session->GetOutputCount();
        inputNames.clear();
        outputNames.clear();
        if (inputCount > 0) {
            inputNames.push_back(session->GetInputNameAllocated(0, allocator).release());
            auto typeInfo = session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo();
            inputShape = typeInfo.GetShape();
        }
        if (outputCount > 0) {
            outputNames.push_back(session->GetOutputNameAllocated(0, allocator).release());
            auto typeInfo = session->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo();
            outputShape = typeInfo.GetShape();
        }
        return true;
    } catch (const Ort::Exception&) {
        session.reset();
        return false;
    }
#else
    juce::ignoreUnused(modelPath);
    return false;
#endif
}

bool OnnxStemSession::isLoaded() const {
#ifdef ENABLE_ORT
    return session != nullptr;
#else
    return false;
#endif
}

OnnxInferenceResult OnnxStemSession::run(const juce::AudioBuffer<float>& input,
                                         std::function<void(float)> progress) {
    OnnxInferenceResult result;
#ifdef ENABLE_ORT
    if (!session) return result;

    const int numChannels = input.getNumChannels();
    const int numSamples = input.getNumSamples();
    if (numChannels <= 0 || numSamples <= 0) return result;

    // Flatten interleaved tensor [1, numSamples, numChannels] or [1, channels, samples]
    std::vector<float> interleaved;
    interleaved.reserve(static_cast<size_t>(numChannels * numSamples));
    for (int i = 0; i < numSamples; ++i)
        for (int ch = 0; ch < numChannels; ++ch)
            interleaved.push_back(input.getSample(ch, i));

    std::vector<int64_t> shape{1, numChannels, numSamples};
    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memInfo, interleaved.data(), interleaved.size(), shape.data(), shape.size());

    auto output = session->Run(Ort::RunOptions{nullptr}, inputNames.data(), &inputTensor, 1, outputNames.data(), outputNames.size());
    if (output.empty() || !output[0].IsTensor()) return result;

    auto& outTensor = output[0];
    auto info = outTensor.GetTensorTypeAndShapeInfo();
    auto outShape = info.GetShape(); // Expect [1, stems, channels, samples]
    if (outShape.size() < 4) return result;

    const int64_t stems = outShape[1];
    const int64_t channels = outShape[2];
    const int64_t samples = outShape[3];
    const float* data = outTensor.GetTensorData<float>();

    // Map stem index to name
    auto stemName = [&](int idx) {
        switch (idx) {
            case 0: return juce::String("vocals");
            case 1: return juce::String("drums");
            case 2: return juce::String("bass");
            case 3: return juce::String("other");
            default: return juce::String("stem") + juce::String(idx);
        }
    };

    size_t cursor = 0;
    for (int64_t s = 0; s < stems; ++s) {
        juce::AudioBuffer<float> buf((int) channels, (int) samples);
        for (int64_t c = 0; c < channels; ++c) {
            for (int64_t i = 0; i < samples; ++i) {
                buf.setSample((int) c, (int) i, data[cursor++]);
            }
        }
        result.stems[stemName((int)s)] = std::move(buf);
        if (progress) progress(static_cast<float>(s + 1) / (float) stems);
    }
    result.success = true;
#else
    juce::ignoreUnused(input, progress);
#endif
    return result;
}

} // namespace AI
} // namespace omega
