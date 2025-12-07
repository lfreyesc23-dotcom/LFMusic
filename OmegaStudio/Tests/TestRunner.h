#pragma once

#include "AudioGoldenTests.h"
#include "MLTests.h"
#include "MIDITests.h"
#include "UITests.h"

namespace OmegaStudio {
namespace Tests {

/**
 * Master test runner for all OmegaStudio tests
 */
class TestRunner {
public:
    struct TestResults {
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        double elapsedTimeMs = 0.0;
        
        float getPassRate() const {
            if (totalTests == 0) return 0.0f;
            return (float)passedTests / (float)totalTests * 100.0f;
        }
    };
    
    static TestResults runAllTests() {
        TestResults results;
        auto startTime = juce::Time::getMillisecondCounterHiRes();
        
        DBG("========================================");
        DBG("   OmegaStudio Test Suite");
        DBG("========================================");
        
        // Audio Golden Tests
        DBG("\n[Audio Golden Tests]");
        bool audioGoldenPassed = AudioGoldenTests::runAllTests();
        results.totalTests += 6; // 6 audio tests
        if (audioGoldenPassed) results.passedTests += 6;
        else results.failedTests += 6;
        
        // ML/AI Tests
        DBG("\n[ML/AI Tests]");
        bool mlPassed = MLTests::runAllTests();
        results.totalTests += 4; // 4 ML tests
        if (mlPassed) results.passedTests += 4;
        else results.failedTests += 4;
        
        // MIDI Tests
        DBG("\n[MIDI Tests]");
        bool midiPassed = MIDITests::runAllTests();
        results.totalTests += 4; // 4 MIDI tests
        if (midiPassed) results.passedTests += 4;
        else results.failedTests += 4;
        
        // UI Tests
        DBG("\n[UI Tests]");
        bool uiPassed = UITests::runAllTests();
        results.totalTests += 4; // 4 UI tests
        if (uiPassed) results.passedTests += 4;
        else results.failedTests += 4;
        
        results.elapsedTimeMs = juce::Time::getMillisecondCounterHiRes() - startTime;
        
        // Print summary
        DBG("\n========================================");
        DBG("   Test Results");
        DBG("========================================");
        DBG("Total Tests:  " << results.totalTests);
        DBG("Passed:       " << results.passedTests);
        DBG("Failed:       " << results.failedTests);
        DBG("Pass Rate:    " << juce::String(results.getPassRate(), 1) << "%");
        DBG("Time Elapsed: " << juce::String(results.elapsedTimeMs, 2) << "ms");
        DBG("========================================\n");
        
        return results;
    }
    
    // Run specific test category
    static bool runCategory(const juce::String& category) {
        if (category == "audio") {
            return AudioGoldenTests::runAllTests();
        } else if (category == "ml" || category == "ai") {
            return MLTests::runAllTests();
        } else if (category == "midi") {
            return MIDITests::runAllTests();
        } else if (category == "ui") {
            return UITests::runAllTests();
        }
        
        DBG("Unknown test category: " << category);
        return false;
    }
};

} // namespace Tests
} // namespace OmegaStudio
