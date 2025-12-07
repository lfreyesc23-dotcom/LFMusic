#pragma once

#include <JuceHeader.h>
#include "../Source/Audio/AI/StemSeparation.h"
#include "../Source/Audio/AI/DenoiseService.h"
#include "../Source/Audio/AI/AIServiceStubs.h"

namespace OmegaStudio {
namespace Tests {

/**
 * ML/AI Tests - Verify machine learning services
 */
class MLTests {
public:
    static bool runAllTests() {
        bool allPassed = true;
        
        allPassed &= testStemSeparation();
        allPassed &= testDenoiseService();
        allPassed &= testLoopStarter();
        allPassed &= testMOSSimulation();
        
        return allPassed;
    }
    
private:
    // Test stem separation SNR/SDR
    static bool testStemSeparation() {
        omega::AI::StemSeparator separator;
        omega::AI::SeparationConfig config;
        config.sampleRate = 44100.0;
        config.quality = omega::AI::QualityMode::Fast;
        
        separator.setConfig(config);
        
        // Create test audio (mixed sine waves)
        juce::AudioBuffer<float> input(2, 44100);
        for (int i = 0; i < 44100; ++i) {
            // Simulate vocals (440 Hz) + drums (noise)
            float vocals = std::sin(juce::MathConstants<float>::twoPi * 440.0f * i / 44100.0f);
            float drums = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.3f;
            input.setSample(0, i, vocals + drums);
            input.setSample(1, i, vocals + drums);
        }
        
        // Process separation
        auto result = separator.separate(input, {omega::AI::StemType::Vocals});
        
        bool passed = !result.empty();
        if (passed && result.count(omega::AI::StemType::Vocals)) {
            float snr = calculateSNR(input, result[omega::AI::StemType::Vocals]);
            passed = snr > 5.0f; // At least 5dB improvement
            DBG("Stem Separation SNR: " << snr << " dB");
        }
        
        DBG("Stem Separation Test: " << (passed ? "PASSED" : "FAILED"));
        return passed;
    }
    
    // Test denoise service
    static bool testDenoiseService() {
        omega::AI::DenoiseService denoiser;
        omega::AI::DenoiseConfig config;
        config.sampleRate = 44100.0;
        config.reductionAmount = 0.8f;
        
        denoiser.setConfig(config);
        
        // Create noisy signal
        juce::AudioBuffer<float> input(2, 8192);
        for (int i = 0; i < 8192; ++i) {
            float signal = std::sin(juce::MathConstants<float>::twoPi * 440.0f * i / 44100.0f);
            float noise = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.5f;
            input.setSample(0, i, signal + noise);
            input.setSample(1, i, signal + noise);
        }
        
        auto result = denoiser.processAudio(input);
        
        bool passed = result.success && result.noiseReductionDb < 0.0f; // Negative = noise reduced
        
        DBG("Denoise Test: " << result.noiseReductionDb << " dB reduction - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test loop starter generation
    static bool testLoopStarter() {
        omega::AI::LoopStarterService loopStarter;
        omega::AI::LoopRequest request;
        request.genre = "Electronic";
        request.bpm = 128.0f;
        request.bars = 8;
        request.key = "Am";
        
        std::vector<omega::AI::LoopClip> clips;
        bool success = loopStarter.generateArrangement(request, clips);
        
        bool passed = success && clips.size() >= 4; // Should generate multiple clips
        
        DBG("Loop Starter Test: Generated " << clips.size() << " clips - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Simulate MOS (Mean Opinion Score) for denoise quality
    static bool testMOSSimulation() {
        // MOS typically ranges 1-5 (5 = excellent)
        omega::AI::DenoiseService denoiser;
        
        juce::AudioBuffer<float> cleanSignal(2, 8192);
        juce::AudioBuffer<float> noisySignal(2, 8192);
        
        // Generate clean signal
        for (int i = 0; i < 8192; ++i) {
            float signal = std::sin(juce::MathConstants<float>::twoPi * 440.0f * i / 44100.0f);
            cleanSignal.setSample(0, i, signal);
            cleanSignal.setSample(1, i, signal);
            
            float noise = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 0.3f;
            noisySignal.setSample(0, i, signal + noise);
            noisySignal.setSample(1, i, signal + noise);
        }
        
        auto result = denoiser.processAudio(noisySignal);
        
        // Calculate correlation with clean signal (simulated MOS)
        float correlation = calculateCorrelation(cleanSignal, result.denoisedAudio);
        float simulatedMOS = 1.0f + (correlation * 4.0f); // Map to 1-5 scale
        
        bool passed = simulatedMOS > 3.0f; // Should be "Good" or better
        
        DBG("Simulated MOS Test: " << simulatedMOS << " / 5.0 - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Helper: Calculate Signal-to-Noise Ratio
    static float calculateSNR(const juce::AudioBuffer<float>& noisy, 
                             const juce::AudioBuffer<float>& denoised) {
        float noisyPower = 0.0f;
        float denoisedPower = 0.0f;
        
        for (int ch = 0; ch < noisy.getNumChannels(); ++ch) {
            noisyPower += noisy.getRMSLevel(ch, 0, noisy.getNumSamples());
            denoisedPower += denoised.getRMSLevel(ch, 0, denoised.getNumSamples());
        }
        
        noisyPower /= noisy.getNumChannels();
        denoisedPower /= denoised.getNumChannels();
        
        if (noisyPower < 0.0001f) return 0.0f;
        
        return 20.0f * std::log10(denoisedPower / noisyPower);
    }
    
    // Helper: Calculate correlation between two buffers
    static float calculateCorrelation(const juce::AudioBuffer<float>& a,
                                     const juce::AudioBuffer<float>& b) {
        if (a.getNumSamples() != b.getNumSamples()) return 0.0f;
        
        float sum = 0.0f;
        float sumA = 0.0f;
        float sumB = 0.0f;
        
        for (int ch = 0; ch < juce::jmin(a.getNumChannels(), b.getNumChannels()); ++ch) {
            const float* dataA = a.getReadPointer(ch);
            const float* dataB = b.getReadPointer(ch);
            
            for (int i = 0; i < a.getNumSamples(); ++i) {
                sum += dataA[i] * dataB[i];
                sumA += dataA[i] * dataA[i];
                sumB += dataB[i] * dataB[i];
            }
        }
        
        float denominator = std::sqrt(sumA * sumB);
        if (denominator < 0.0001f) return 0.0f;
        
        return sum / denominator;
    }
};

} // namespace Tests
} // namespace OmegaStudio
