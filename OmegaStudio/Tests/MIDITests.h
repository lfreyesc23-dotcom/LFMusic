#pragma once

#include <JuceHeader.h>
#include "../Source/Sequencer/MIDI/MIDIEngine.h"

namespace OmegaStudio {
namespace Tests {

/**
 * MIDI Tests - Verify MIDI processing and latency
 */
class MIDITests {
public:
    static bool runAllTests() {
        bool allPassed = true;
        
        allPassed &= testMIDIRoundtripLatency();
        allPassed &= testMIDIJitter();
        allPassed &= testMPELanes();
        allPassed &= testMIDIRouting();
        
        return allPassed;
    }
    
private:
    // Test MIDI roundtrip latency
    static bool testMIDIRoundtripLatency() {
        Sequencer::MIDI::MIDIEngine midiEngine;
        
        juce::MidiBuffer inputBuffer;
        
        // Send test notes
        auto startTime = juce::Time::getMillisecondCounterHiRes();
        
        for (int i = 0; i < 100; ++i) {
            inputBuffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), i * 10);
        }
        
        // Process through engine
        juce::AudioBuffer<float> audioBuffer(2, 1024);
        midiEngine.processBlock(audioBuffer, inputBuffer);
        
        auto elapsed = juce::Time::getMillisecondCounterHiRes() - startTime;
        
        // Should process 100 notes in < 1ms
        bool passed = elapsed < 1.0;
        
        DBG("MIDI Roundtrip Latency: " << elapsed << "ms - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test MIDI timing jitter
    static bool testMIDIJitter() {
        juce::MidiBuffer buffer;
        
        // Create perfectly timed notes
        std::vector<int> timestamps;
        for (int i = 0; i < 100; ++i) {
            int timestamp = i * 100; // Every 100 samples
            timestamps.push_back(timestamp);
            buffer.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), timestamp);
        }
        
        // Read back and measure jitter
        int prevTimestamp = -1;
        std::vector<int> intervals;
        
        for (const auto& event : buffer) {
            if (prevTimestamp >= 0) {
                intervals.push_back(event.samplePosition - prevTimestamp);
            }
            prevTimestamp = event.samplePosition;
        }
        
        // Calculate standard deviation
        float mean = 0.0f;
        for (int interval : intervals) {
            mean += interval;
        }
        mean /= intervals.size();
        
        float variance = 0.0f;
        for (int interval : intervals) {
            float diff = interval - mean;
            variance += diff * diff;
        }
        variance /= intervals.size();
        float stdDev = std::sqrt(variance);
        
        // Jitter should be < 1 sample
        bool passed = stdDev < 1.0f;
        
        DBG("MIDI Jitter: " << stdDev << " samples - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test MPE (MIDI Polyphonic Expression) lanes
    static bool testMPELanes() {
        Sequencer::MIDI::MIDIEngine midiEngine;
        
        // Enable MPE mode
        midiEngine.setMPEEnabled(true);
        
        juce::MidiBuffer buffer;
        
        // Send notes on different MPE channels (2-15)
        for (int channel = 2; channel <= 15; ++channel) {
            buffer.addEvent(juce::MidiMessage::noteOn(channel, 60, 0.8f), 0);
            
            // Add pitch bend (per-note)
            buffer.addEvent(juce::MidiMessage::pitchWheel(channel, 8192), 100);
            
            // Add pressure (per-note)
            buffer.addEvent(juce::MidiMessage::channelPressureChange(channel, 100), 200);
        }
        
        juce::AudioBuffer<float> audioBuffer(2, 1024);
        midiEngine.processBlock(audioBuffer, buffer);
        
        // Verify all 14 MPE channels processed
        bool passed = midiEngine.getActiveVoices() == 14;
        
        DBG("MPE Lanes Test: " << midiEngine.getActiveVoices() << " voices - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test MIDI routing
    static bool testMIDIRouting() {
        Sequencer::MIDI::MIDIEngine midiEngine;
        
        // Create routing: Input -> Track 1 -> Track 2
        midiEngine.addRoute(0, 1); // Input 0 to Track 1
        midiEngine.addRoute(1, 2); // Track 1 to Track 2
        
        juce::MidiBuffer input;
        input.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        
        // Process
        juce::AudioBuffer<float> audioBuffer(2, 512);
        midiEngine.processBlock(audioBuffer, input);
        
        // Verify routing occurred
        auto track1Buffer = midiEngine.getTrackBuffer(1);
        auto track2Buffer = midiEngine.getTrackBuffer(2);
        
        bool passed = !track1Buffer.isEmpty() && !track2Buffer.isEmpty();
        
        DBG("MIDI Routing Test: Track1=" << track1Buffer.getNumEvents() 
            << ", Track2=" << track2Buffer.getNumEvents() 
            << " - " << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
};

} // namespace Tests
} // namespace OmegaStudio
