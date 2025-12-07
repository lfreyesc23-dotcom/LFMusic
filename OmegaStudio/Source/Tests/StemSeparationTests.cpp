#include <JuceHeader.h>
#include "../Audio/AI/StemSeparation.h"

using namespace omega::AI;

class StemSeparationTest : public juce::UnitTest {
public:
    StemSeparationTest() : juce::UnitTest("StemSeparation", "AI") {}

    void runTest() override {
        beginTest("Separates buffers and returns stems");
        StemSeparator sep;
        SeparationConfig cfg;
        cfg.sampleRate = 48000.0;
        cfg.normalizeOutput = true;
        sep.setConfig(cfg);

        juce::AudioBuffer<float> input(2, 1024);
        input.clear();
        input.setSample(0, 10, 1.0f);
        input.setSample(1, 20, 0.5f);

        std::map<StemType, juce::AudioBuffer<float>> outputs;
        float lastProgress = 0.0f;
        bool ok = sep.separateStems(input, outputs, [&](float p) { lastProgress = p; });

        expect(ok, "separateStems should succeed");
        expectGreaterOrEqual((int)outputs.size(), 4, "Should produce at least four stems");
        expectWithinAbsoluteError(lastProgress, 1.0f, 0.001f, "Progress should reach 1.0");

        auto checkStem = [&](StemType t) {
            auto it = outputs.find(t);
            expect(it != outputs.end(), "Stem missing");
            if (it != outputs.end()) {
                auto mag = it->second.getMagnitude(0, it->second.getNumSamples());
                expect(mag > 0.0f, "Stem magnitude should be non-zero");
            }
        };
        checkStem(StemType::Vocals);
        checkStem(StemType::Drums);
        checkStem(StemType::Bass);
        checkStem(StemType::Other);
    }
};

static StemSeparationTest stemSeparationTest;
