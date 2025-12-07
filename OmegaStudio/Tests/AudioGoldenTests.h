#pragma once

#include <JuceHeader.h>
#include "../Source/Audio/DSP/SIMDProcessor.h"
#include "../Source/Audio/DSP/PitchCorrection.h"
#include "../Source/Audio/Effects/PremiumFX.h"

namespace OmegaStudio {
namespace Tests {

/**
 * Audio Golden Tests - Verify DSP algorithms produce expected output
 */
class AudioGoldenTests {
public:
    static bool runAllTests() {
        bool allPassed = true;
        
        allPassed &= testImpulseResponse();
        allPassed &= testStepResponse();
        allPassed &= testSIMDProcessor();
        allPassed &= testPitchCorrection();
        allPassed &= testFXProcessing();
        allPassed &= testCPUBudget();
        
        return allPassed;
    }
    
private:
    // Test impulse response (delta function)
    static bool testImpulseResponse() {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        
        // Create impulse at sample 0
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        // Process through FX
        Audio::Effects::Emphasis emphasis;
        emphasis.prepare(44100.0, 512);
        
        juce::MidiBuffer midi;
        emphasis.processBlock(buffer, midi);
        
        // Verify output is non-zero
        float output = buffer.getRMSLevel(0, 0, 512);
        bool passed = output > 0.0f;
        
        DBG("Impulse Response Test: " << (passed ? "PASSED" : "FAILED"));
        return passed;
    }
    
    // Test step response
    static bool testStepResponse() {
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        
        // Create step function
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(0, i, 1.0f);
            buffer.setSample(1, i, 1.0f);
        }
        
        Audio::Effects::LuxeVerb reverb;
        reverb.prepare(44100.0, 512);
        
        juce::MidiBuffer midi;
        reverb.processBlock(buffer, midi);
        
        // Verify reverb tail exists
        float rms = buffer.getRMSLevel(0, 256, 256);
        bool passed = rms > 0.1f;
        
        DBG("Step Response Test: " << (passed ? "PASSED" : "FAILED"));
        return passed;
    }
    
    // Test SIMD processor
    static bool testSIMDProcessor() {
        Audio::DSP::SIMDProcessor simd;
        
        juce::AudioBuffer<float> buffer(2, 512);
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(0, i, std::sin(juce::MathConstants<float>::twoPi * i / 512.0f));
            buffer.setSample(1, i, std::sin(juce::MathConstants<float>::twoPi * i / 512.0f));
        }
        
        simd.prepare(44100.0, 512);
        
        juce::MidiBuffer midi;
        simd.processBlock(buffer, midi);
        
        // Verify processing occurred
        float rms = buffer.getRMSLevel(0, 0, 512);
        bool passed = rms > 0.0f && rms < 1.0f;
        
        DBG("SIMD Processor Test: " << (passed ? "PASSED" : "FAILED"));
        return passed;
    }
    
    // Test pitch correction
    static bool testPitchCorrection() {
        Audio::DSP::PitchCorrection pitch;
        pitch.setTarget(440.0); // A4
        
        juce::AudioBuffer<float> buffer(2, 2048);
        
        // Generate 440Hz sine wave
        for (int i = 0; i < 2048; ++i) {
            float sample = std::sin(juce::MathConstants<float>::twoPi * 440.0f * i / 44100.0f);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        
        pitch.prepare(44100.0, 2048);
        
        juce::MidiBuffer midi;
        pitch.processBlock(buffer, midi);
        
        // Verify output is reasonable
        float rms = buffer.getRMSLevel(0, 0, 2048);
        bool passed = rms > 0.3f && rms < 0.9f;
        
        DBG("Pitch Correction Test: " << (passed ? "PASSED" : "FAILED"));
        return passed;
    }
    
    // Test FX processing
    static bool testFXProcessing() {
        Audio::Effects::TransientProcessor transient;
        transient.prepare(44100.0, 512);
        
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Generate drum hit (attack + decay)
        for (int i = 0; i < 512; ++i) {
            float env = std::exp(-i / 100.0f);
            float noise = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * 2.0f;
            buffer.setSample(0, i, noise * env);
            buffer.setSample(1, i, noise * env);
        }
        
        juce::MidiBuffer midi;
        transient.processBlock(buffer, midi);
        
        // Verify transient processing occurred
        float peakStart = buffer.getMagnitude(0, 0, 50);
        float peakEnd = buffer.getMagnitude(0, 400, 112);
        bool passed = peakStart > peakEnd * 2.0f; // Attack should be emphasized
        
        DBG("FX Processing Test: " << (passed ? "PASSED" : "FAILED"));
        return passed;
    }
    
    // Test CPU budget per block
    static bool testCPUBudget() {
        const double sampleRate = 44100.0;
        const int blockSize = 512;
        const double maxTimeMs = (blockSize / sampleRate) * 1000.0; // Time available per block
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        
        for (int i = 0; i < blockSize; ++i) {
            buffer.setSample(0, i, std::sin(juce::MathConstants<float>::twoPi * i / blockSize));
            buffer.setSample(1, i, std::sin(juce::MathConstants<float>::twoPi * i / blockSize));
        }
        
        // Measure processing time
        auto start = juce::Time::getMillisecondCounterHiRes();
        
        Audio::Effects::Emphasizer emphasizer;
        emphasizer.prepare(sampleRate, blockSize);
        
        juce::MidiBuffer midi;
        emphasizer.processBlock(buffer, midi);
        
        auto elapsed = juce::Time::getMillisecondCounterHiRes() - start;
        
        // Should use less than 50% of available time
        bool passed = elapsed < (maxTimeMs * 0.5);
        
        DBG("CPU Budget Test: " << elapsed << "ms / " << maxTimeMs << "ms - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
};

} // namespace Tests
} // namespace OmegaStudio
