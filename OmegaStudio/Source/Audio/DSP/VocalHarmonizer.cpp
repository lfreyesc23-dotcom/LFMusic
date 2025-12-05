/**
 * @file VocalHarmonizer.cpp
 * @brief Implementation of vocal harmonizer
 */

#include "VocalHarmonizer.h"
#include <cmath>

namespace omega {

//==============================================================================
// VocalDoubler Implementation
//==============================================================================

VocalDoubler::VocalDoubler() {
    m_random.setSeedRandomly();
}

void VocalDoubler::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    
    for (auto& voice : m_voices) {
        int bufferSize = static_cast<int>(sampleRate * 0.1); // 100ms max delay
        voice.delayBuffer.setSize(1, bufferSize);
        voice.delayBuffer.clear();
        voice.writePosition = 0;
        
        // Random offsets for natural doubling
        voice.timingOffset = m_random.nextFloat() * m_timingVariation;
        voice.pitchOffset = (m_random.nextFloat() - 0.5f) * m_pitchVariation;
        voice.panPosition = (m_random.nextFloat() - 0.5f) * 2.0f;
    }
}

void VocalDoubler::process(const float* inputBuffer, 
                          float* leftOutput, 
                          float* rightOutput, 
                          int numSamples) {
    // Clear outputs
    juce::FloatVectorOperations::clear(leftOutput, numSamples);
    juce::FloatVectorOperations::clear(rightOutput, numSamples);
    
    // Add dry signal
    float dryGain = 1.0f - m_mix;
    juce::FloatVectorOperations::addWithMultiply(leftOutput, inputBuffer, dryGain, numSamples);
    juce::FloatVectorOperations::addWithMultiply(rightOutput, inputBuffer, dryGain, numSamples);
    
    // Process each voice
    float wetGain = m_mix / m_numVoices;
    
    for (int v = 0; v < m_numVoices; ++v) {
        auto& voice = m_voices[v];
        int delaySamples = static_cast<int>(voice.timingOffset * m_sampleRate * 0.001f);
        
        for (int i = 0; i < numSamples; ++i) {
            // Write to delay buffer
            voice.delayBuffer.setSample(0, voice.writePosition, inputBuffer[i]);
            
            // Read delayed sample
            int readPos = (voice.writePosition - delaySamples + voice.delayBuffer.getNumSamples()) 
                         % voice.delayBuffer.getNumSamples();
            float sample = voice.delayBuffer.getSample(0, readPos);
            
            // Apply panning
            float pan = voice.panPosition * m_width;
            float leftGain = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            float rightGain = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            
            leftOutput[i] += sample * wetGain * leftGain;
            rightOutput[i] += sample * wetGain * rightGain;
            
            voice.writePosition = (voice.writePosition + 1) % voice.delayBuffer.getNumSamples();
        }
    }
}

void VocalDoubler::reset() {
    for (auto& voice : m_voices) {
        voice.delayBuffer.clear();
        voice.writePosition = 0;
    }
}

//==============================================================================
// HarmonyVoice Implementation
//==============================================================================

HarmonyVoice::HarmonyVoice() {
}

void HarmonyVoice::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_pitchShifter.initialize(sampleRate, 2048, 512);
    
    int delaySize = static_cast<int>(sampleRate * 0.1); // 100ms
    m_delayBuffer.setSize(1, delaySize);
    m_delayBuffer.clear();
}

void HarmonyVoice::process(const float* input, float* output, int numSamples) {
    if (m_interval == 0) {
        juce::FloatVectorOperations::copy(output, input, numSamples);
        juce::FloatVectorOperations::multiply(output, m_level, numSamples);
        return;
    }
    
    // Calculate pitch ratio
    float pitchRatio = std::pow(2.0f, m_interval / 12.0f);
    
    // Apply pitch shifting
    m_pitchShifter.process(input, output, numSamples, pitchRatio);
    
    // Apply level
    juce::FloatVectorOperations::multiply(output, m_level, numSamples);
}

void HarmonyVoice::reset() {
    m_pitchShifter.reset();
    m_delayBuffer.clear();
    m_writePosition = 0;
}

void HarmonyVoice::setDelay(float ms) {
    m_delay = juce::jlimit(0.0f, 100.0f, ms);
}

//==============================================================================
// VocalHarmonizer Implementation
//==============================================================================

VocalHarmonizer::VocalHarmonizer() {
    for (int i = 0; i < kMaxVoices; ++i) {
        m_voices[i] = std::make_unique<HarmonyVoice>();
    }
}

void VocalHarmonizer::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    
    for (auto& voice : m_voices) {
        voice->initialize(sampleRate, maxBlockSize);
    }
    
    m_doubler.initialize(sampleRate, maxBlockSize);
    m_pitchDetector.initialize(sampleRate, 2048);
    
    m_tempBuffer.setSize(2, maxBlockSize);
    
    updateHarmonyVoices();
}

void VocalHarmonizer::process(const float* inputBuffer, 
                              juce::AudioBuffer<float>& outputBuffer, 
                              int numSamples) {
    outputBuffer.clear();
    
    // Add dry signal
    float dryGain = 1.0f - m_mix;
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
        juce::FloatVectorOperations::addWithMultiply(
            outputBuffer.getWritePointer(ch), 
            inputBuffer, 
            dryGain, 
            numSamples
        );
    }
    
    // Process doubler if enabled
    if (m_doublerEnabled) {
        m_doubler.process(inputBuffer, 
                         outputBuffer.getWritePointer(0),
                         outputBuffer.getWritePointer(1),
                         numSamples);
        return;
    }
    
    // Process harmony voices
    float wetGain = m_mix * m_harmonyLevel;
    
    for (int v = 0; v < kMaxVoices; ++v) {
        if (!m_voiceEnabled[v])
            continue;
        
        // Process voice to temp buffer
        float* tempVoice = m_tempBuffer.getWritePointer(0);
        m_voices[v]->process(inputBuffer, tempVoice, numSamples);
        
        // Apply pan and add to output
        float pan = m_voices[v]->getPan() * m_width;
        float leftGain = std::cos((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * wetGain;
        float rightGain = std::sin((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * wetGain;
        
        juce::FloatVectorOperations::addWithMultiply(
            outputBuffer.getWritePointer(0), tempVoice, leftGain, numSamples);
        juce::FloatVectorOperations::addWithMultiply(
            outputBuffer.getWritePointer(1), tempVoice, rightGain, numSamples);
    }
}

void VocalHarmonizer::reset() {
    for (auto& voice : m_voices) {
        voice->reset();
    }
    m_doubler.reset();
}

void VocalHarmonizer::updateHarmonyVoices() {
    switch (m_mode) {
        case Mode::Manual:
            // User defines intervals manually
            break;
            
        case Mode::ScaleBased:
            generateScaleHarmonies(m_key);
            break;
            
        case Mode::Chord:
            generateChordHarmonies();
            break;
            
        case Mode::Unison:
            for (int i = 0; i < kMaxVoices; ++i) {
                m_voices[i]->setInterval(0);
                m_voices[i]->setLevel(0.5f);
            }
            break;
            
        case Mode::Octaves:
            m_voices[0]->setInterval(-12);  // Octave down
            m_voices[1]->setInterval(12);   // Octave up
            m_voices[2]->setInterval(0);
            m_voices[3]->setInterval(0);
            break;
    }
    
    applyVoiceLimits();
}

void VocalHarmonizer::generateScaleHarmonies(int rootNote) {
    // Major scale intervals
    const int majorScaleIntervals[] = { 0, 2, 4, 5, 7, 9, 11, 12 };
    
    // Generate harmonies based on voicing
    switch (m_voicing) {
        case Voicing::Close:
            m_voices[0]->setInterval(3);   // Minor 3rd
            m_voices[1]->setInterval(5);   // Perfect 4th
            m_voices[2]->setInterval(7);   // Perfect 5th
            m_voices[3]->setInterval(12);  // Octave
            break;
            
        case Voicing::Open:
            m_voices[0]->setInterval(7);   // Perfect 5th
            m_voices[1]->setInterval(12);  // Octave
            m_voices[2]->setInterval(16);  // Octave + 4th
            m_voices[3]->setInterval(19);  // Octave + 5th
            break;
            
        case Voicing::Drop2:
            m_voices[0]->setInterval(-5);  // 4th down
            m_voices[1]->setInterval(3);   // 3rd up
            m_voices[2]->setInterval(7);   // 5th up
            m_voices[3]->setInterval(12);  // Octave up
            break;
            
        case Voicing::Stacked:
            m_voices[0]->setInterval(4);   // Major 3rd
            m_voices[1]->setInterval(7);   // Perfect 5th
            m_voices[2]->setInterval(12);  // Octave
            m_voices[3]->setInterval(16);  // Octave + 4th
            break;
    }
}

void VocalHarmonizer::generateChordHarmonies() {
    switch (m_chordQuality) {
        case ChordQuality::Major:
            m_voices[0]->setInterval(4);   // Major 3rd
            m_voices[1]->setInterval(7);   // Perfect 5th
            m_voices[2]->setInterval(12);  // Octave
            break;
            
        case ChordQuality::Minor:
            m_voices[0]->setInterval(3);   // Minor 3rd
            m_voices[1]->setInterval(7);   // Perfect 5th
            m_voices[2]->setInterval(12);  // Octave
            break;
            
        case ChordQuality::Diminished:
            m_voices[0]->setInterval(3);   // Minor 3rd
            m_voices[1]->setInterval(6);   // Tritone
            m_voices[2]->setInterval(9);   // Diminished 7th
            break;
            
        case ChordQuality::Augmented:
            m_voices[0]->setInterval(4);   // Major 3rd
            m_voices[1]->setInterval(8);   // Augmented 5th
            m_voices[2]->setInterval(12);  // Octave
            break;
            
        case ChordQuality::Sus2:
            m_voices[0]->setInterval(2);   // Major 2nd
            m_voices[1]->setInterval(7);   // Perfect 5th
            m_voices[2]->setInterval(12);  // Octave
            break;
            
        case ChordQuality::Sus4:
            m_voices[0]->setInterval(5);   // Perfect 4th
            m_voices[1]->setInterval(7);   // Perfect 5th
            m_voices[2]->setInterval(12);  // Octave
            break;
    }
}

void VocalHarmonizer::applyVoiceLimits() {
    // Apply panning spread
    const float panPositions[] = { -0.7f, -0.3f, 0.3f, 0.7f };
    for (int i = 0; i < kMaxVoices; ++i) {
        m_voices[i]->setPan(panPositions[i]);
    }
}

void VocalHarmonizer::setVoiceInterval(int voice, int semitones) {
    if (voice >= 0 && voice < kMaxVoices) {
        m_voices[voice]->setInterval(semitones);
    }
}

void VocalHarmonizer::setVoiceEnabled(int voice, bool enabled) {
    if (voice >= 0 && voice < kMaxVoices) {
        m_voiceEnabled[voice] = enabled;
    }
}

bool VocalHarmonizer::isVoiceEnabled(int voice) const {
    return voice >= 0 && voice < kMaxVoices && m_voiceEnabled[voice];
}

void VocalHarmonizer::setVoiceLevel(int voice, float level) {
    if (voice >= 0 && voice < kMaxVoices) {
        m_voices[voice]->setLevel(level);
    }
}

float VocalHarmonizer::getVoiceLevel(int voice) const {
    if (voice >= 0 && voice < kMaxVoices) {
        return m_voices[voice]->getLevel();
    }
    return 0.0f;
}

void VocalHarmonizer::setVoicePan(int voice, float pan) {
    if (voice >= 0 && voice < kMaxVoices) {
        m_voices[voice]->setPan(pan);
    }
}

float VocalHarmonizer::getVoicePan(int voice) const {
    if (voice >= 0 && voice < kMaxVoices) {
        return m_voices[voice]->getPan();
    }
    return 0.0f;
}

} // namespace omega
