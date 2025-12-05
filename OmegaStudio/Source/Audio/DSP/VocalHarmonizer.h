/**
 * @file VocalHarmonizer.h
 * @brief Professional vocal harmonizer and doubler
 * 
 * Features:
 * - Automatic harmony generation (3rd, 5th, octave, custom intervals)
 * - Natural vocal doubling with micro-timing variations
 * - Intelligent voice leading
 * - Scale-aware harmonization
 * - Formant preservation
 * - Stereo widening
 */

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <memory>
#include "PitchCorrection.h"
#include "../../Utils/Constants.h"

namespace omega {

/**
 * @class VocalDoubler
 * @brief Creates natural-sounding vocal doubles
 */
class VocalDoubler {
public:
    VocalDoubler();
    ~VocalDoubler() = default;
    
    /**
     * Initialize doubler
     * @param sampleRate Sample rate
     * @param maxBlockSize Maximum block size
     */
    void initialize(double sampleRate, int maxBlockSize);
    
    /**
     * Process audio with doubling effect (RT-safe)
     * @param inputBuffer Input audio
     * @param leftOutput Left channel output
     * @param rightOutput Right channel output
     * @param numSamples Number of samples
     */
    void process(const float* inputBuffer, 
                float* leftOutput, 
                float* rightOutput, 
                int numSamples);
    
    /**
     * Reset processor state
     */
    void reset();
    
    // ============ Parameters ============
    
    /**
     * Set number of voices (1-4)
     */
    void setNumVoices(int voices) { m_numVoices = juce::jlimit(1, 4, voices); }
    int getNumVoices() const { return m_numVoices; }
    
    /**
     * Set timing variation in milliseconds (0-50ms)
     */
    void setTimingVariation(float ms) { m_timingVariation = juce::jlimit(0.0f, 50.0f, ms); }
    float getTimingVariation() const { return m_timingVariation; }
    
    /**
     * Set pitch variation in cents (0-30 cents)
     */
    void setPitchVariation(float cents) { m_pitchVariation = juce::jlimit(0.0f, 30.0f, cents); }
    float getPitchVariation() const { return m_pitchVariation; }
    
    /**
     * Set stereo width (0.0 = mono, 1.0 = wide)
     */
    void setWidth(float width) { m_width = juce::jlimit(0.0f, 1.0f, width); }
    float getWidth() const { return m_width; }
    
    /**
     * Set wet/dry mix (0.0 = dry, 1.0 = wet)
     */
    void setMix(float mix) { m_mix = juce::jlimit(0.0f, 1.0f, mix); }
    float getMix() const { return m_mix; }
    
private:
    struct VoiceState {
        juce::AudioBuffer<float> delayBuffer;
        int writePosition { 0 };
        float timingOffset { 0.0f };
        float pitchOffset { 0.0f };
        float panPosition { 0.0f };
    };
    
    std::array<VoiceState, 4> m_voices;
    int m_numVoices { 2 };
    float m_timingVariation { 15.0f };  // ms
    float m_pitchVariation { 10.0f };   // cents
    float m_width { 0.8f };
    float m_mix { 0.5f };
    double m_sampleRate { 48000.0 };
    
    juce::Random m_random;
};

/**
 * @class HarmonyVoice
 * @brief Single harmony voice with pitch shifting
 */
class HarmonyVoice {
public:
    HarmonyVoice();
    ~HarmonyVoice() = default;
    
    void initialize(double sampleRate, int maxBlockSize);
    void process(const float* input, float* output, int numSamples);
    void reset();
    
    // Parameters
    void setInterval(int semitones) { m_interval = semitones; }
    int getInterval() const { return m_interval; }
    
    void setPan(float pan) { m_pan = juce::jlimit(-1.0f, 1.0f, pan); }
    float getPan() const { return m_pan; }
    
    void setLevel(float level) { m_level = juce::jlimit(0.0f, 1.0f, level); }
    float getLevel() const { return m_level; }
    
    void setDelay(float ms);
    float getDelay() const { return m_delay; }
    
private:
    PhaseVocoder m_pitchShifter;
    juce::AudioBuffer<float> m_delayBuffer;
    int m_writePosition { 0 };
    
    int m_interval { 0 };      // Semitones
    float m_pan { 0.0f };
    float m_level { 0.7f };
    float m_delay { 0.0f };    // ms
    double m_sampleRate { 48000.0 };
};

/**
 * @class VocalHarmonizer
 * @brief Complete vocal harmonizer with intelligent harmony generation
 */
class VocalHarmonizer {
public:
    /**
     * Harmony mode
     */
    enum class Mode {
        Manual,         // User-defined intervals
        ScaleBased,     // Harmonize based on musical scale
        Chord,          // Generate chord (major/minor triads)
        Unison,         // Pitch-perfect unison (doubling)
        Octaves         // Octave doubling
    };
    
    /**
     * Harmony quality/voicing
     */
    enum class Voicing {
        Close,          // Close voicing (tight intervals)
        Open,           // Open voicing (wide intervals)
        Drop2,          // Drop-2 voicing
        Stacked         // Stacked (all voices up)
    };
    
    VocalHarmonizer();
    ~VocalHarmonizer() = default;
    
    /**
     * Initialize harmonizer
     * @param sampleRate Sample rate
     * @param maxBlockSize Maximum block size
     */
    void initialize(double sampleRate, int maxBlockSize);
    
    /**
     * Process audio with harmonization (RT-safe)
     * @param inputBuffer Mono input
     * @param outputBuffer Stereo output
     * @param numSamples Number of samples
     */
    void process(const float* inputBuffer, 
                juce::AudioBuffer<float>& outputBuffer, 
                int numSamples);
    
    /**
     * Reset processor state
     */
    void reset();
    
    // ============ Mode & Configuration ============
    
    void setMode(Mode mode) { m_mode = mode; updateHarmonyVoices(); }
    Mode getMode() const { return m_mode; }
    
    void setVoicing(Voicing voicing) { m_voicing = voicing; updateHarmonyVoices(); }
    Voicing getVoicing() const { return m_voicing; }
    
    /**
     * Set musical key (0 = C, 1 = C#, ..., 11 = B)
     */
    void setKey(int key) { m_key = key % 12; updateHarmonyVoices(); }
    int getKey() const { return m_key; }
    
    /**
     * Set scale type
     */
    void setScale(PitchCorrection::Scale scale) { m_scale = scale; updateHarmonyVoices(); }
    PitchCorrection::Scale getScale() const { return m_scale; }
    
    /**
     * Set chord quality for Chord mode
     */
    enum class ChordQuality {
        Major,
        Minor,
        Diminished,
        Augmented,
        Sus2,
        Sus4
    };
    
    void setChordQuality(ChordQuality quality) { m_chordQuality = quality; updateHarmonyVoices(); }
    ChordQuality getChordQuality() const { return m_chordQuality; }
    
    // ============ Manual Harmony Control ============
    
    /**
     * Set manual harmony intervals (semitones) for Manual mode
     * @param voice Voice index (0-3)
     * @param semitones Interval in semitones
     */
    void setVoiceInterval(int voice, int semitones);
    
    /**
     * Enable/disable individual voice
     */
    void setVoiceEnabled(int voice, bool enabled);
    bool isVoiceEnabled(int voice) const;
    
    /**
     * Set voice level (0.0-1.0)
     */
    void setVoiceLevel(int voice, float level);
    float getVoiceLevel(int voice) const;
    
    /**
     * Set voice pan (-1.0 = left, 1.0 = right)
     */
    void setVoicePan(int voice, float pan);
    float getVoicePan(int voice) const;
    
    // ============ Global Parameters ============
    
    /**
     * Set dry/wet mix (0.0 = dry, 1.0 = wet)
     */
    void setMix(float mix) { m_mix = juce::jlimit(0.0f, 1.0f, mix); }
    float getMix() const { return m_mix; }
    
    /**
     * Set overall harmony level
     */
    void setHarmonyLevel(float level) { m_harmonyLevel = juce::jlimit(0.0f, 1.0f, level); }
    float getHarmonyLevel() const { return m_harmonyLevel; }
    
    /**
     * Set formant preservation (0.0 = none, 1.0 = full)
     */
    void setFormantPreservation(float amount) { m_formantPreservation = amount; }
    float getFormantPreservation() const { return m_formantPreservation; }
    
    /**
     * Set stereo width (0.0 = mono, 1.0 = wide)
     */
    void setWidth(float width) { m_width = juce::jlimit(0.0f, 1.0f, width); }
    float getWidth() const { return m_width; }
    
    /**
     * Enable natural doubling mode
     */
    void setDoublerEnabled(bool enabled) { m_doublerEnabled = enabled; }
    bool isDoublerEnabled() const { return m_doublerEnabled; }
    
private:
    void updateHarmonyVoices();
    void generateScaleHarmonies(int rootNote);
    void generateChordHarmonies();
    void applyVoiceLimits();
    
    static constexpr int kMaxVoices = 4;
    std::array<std::unique_ptr<HarmonyVoice>, kMaxVoices> m_voices;
    std::array<bool, kMaxVoices> m_voiceEnabled { true, true, false, false };
    
    VocalDoubler m_doubler;
    PitchDetector m_pitchDetector;
    
    Mode m_mode { Mode::ScaleBased };
    Voicing m_voicing { Voicing::Close };
    ChordQuality m_chordQuality { ChordQuality::Major };
    
    int m_key { 0 };  // C major by default
    PitchCorrection::Scale m_scale { PitchCorrection::Scale::Major };
    
    float m_mix { 0.5f };
    float m_harmonyLevel { 0.7f };
    float m_formantPreservation { 0.5f };
    float m_width { 0.8f };
    bool m_doublerEnabled { false };
    
    double m_sampleRate { 48000.0 };
    
    juce::AudioBuffer<float> m_tempBuffer;
};

} // namespace omega
