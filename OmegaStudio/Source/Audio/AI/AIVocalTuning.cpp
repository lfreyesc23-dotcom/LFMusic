/*
  ==============================================================================

    AIVocalTuning.cpp

  ==============================================================================
*/

#include "AIVocalTuning.h"
#include <cmath>

namespace omega {
namespace AI {

AIVocalTuner::AIVocalTuner() {
    initialize(48000.0);
}

void AIVocalTuner::initialize(double sampleRate) {
    sampleRate_ = sampleRate;
}

void AIVocalTuner::process(juce::AudioBuffer<float>& buffer) {
    detectedPitch_ = detectPitch(buffer);
    float targetPitch = quantizeToScale(detectedPitch_);
    
    // Placeholder pitch shift
    if (std::abs(targetPitch - detectedPitch_) > 1.0f) {
        float shift = (targetPitch - detectedPitch_) * correction_;
        (void)shift; // Apply pitch shift here
    }
}

float AIVocalTuner::detectPitch(const juce::AudioBuffer<float>& buffer) {
    // Simplified autocorrelation
    const float* data = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();
    
    float maxCorr = 0.0f;
    int bestLag = 100;
    
    for (int lag = 50; lag < 500; ++lag) {
        float corr = 0.0f;
        for (int i = 0; i < numSamples - lag; ++i) {
            corr += data[i] * data[i + lag];
        }
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    float freq = static_cast<float>(sampleRate_) / bestLag;
    return freq;
}

float AIVocalTuner::quantizeToScale(float pitch) {
    int midiNote = static_cast<int>(12.0f * std::log2(pitch / 440.0f) + 69.0f);
    int octave = midiNote / 12;
    int noteInOctave = midiNote % 12;
    
    // Find closest scale note
    int closestNote = scale_[0];
    int minDist = 12;
    
    for (int scaleNote : scale_) {
        int dist = std::abs(noteInOctave - scaleNote);
        if (dist < minDist) {
            minDist = dist;
            closestNote = scaleNote;
        }
    }
    
    int targetMidi = octave * 12 + closestNote;
    float targetFreq = 440.0f * std::pow(2.0f, (targetMidi - 69) / 12.0f);
    
    return targetFreq;
}

} // namespace AI
} // namespace omega
