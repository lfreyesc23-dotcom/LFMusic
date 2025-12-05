/**
 * @file PitchCorrection.h
 * @brief Real-time pitch correction (Autotune) processor
 * 
 * Professional pitch correction with automatic detection and SIMD optimization.
 * Implements phase vocoder for pitch shifting without time stretching.
 */

#pragma once

#include <JuceHeader.h>
#include <array>
#include <complex>
#include <memory>
#include "../../Utils/Constants.h"

namespace omega {

/**
 * @class PitchDetector
 * @brief High-performance pitch detection using YIN algorithm
 */
class PitchDetector {
public:
    PitchDetector();
    ~PitchDetector() = default;

    /**
     * Initialize pitch detector with sample rate
     * @param sampleRate Audio sample rate
     * @param bufferSize Analysis buffer size (power of 2)
     */
    void initialize(double sampleRate, int bufferSize);

    /**
     * Detect pitch from audio buffer (RT-safe)
     * @param buffer Audio samples
     * @param numSamples Number of samples to analyze
     * @return Detected frequency in Hz (0.0 if no pitch detected)
     */
    float detectPitch(const float* buffer, int numSamples);

    /**
     * Get confidence level of last detection
     * @return Confidence (0.0 = no confidence, 1.0 = high confidence)
     */
    float getConfidence() const noexcept { return m_confidence; }

private:
    // YIN algorithm implementation
    void calculateDifference(const float* buffer, int numSamples);
    void cumulativeMeanNormalizedDifference();
    int absoluteThreshold(float threshold);
    float parabolicInterpolation(int tauEstimate);

    std::vector<float> m_yinBuffer;
    double m_sampleRate = 48000.0;
    int m_bufferSize = 2048;
    float m_confidence = 0.0f;
    
    static constexpr float kDefaultThreshold = 0.15f;
};

/**
 * @class PhaseVocoder
 * @brief Phase vocoder for pitch shifting without time stretching
 */
class PhaseVocoder {
public:
    PhaseVocoder();
    ~PhaseVocoder() = default;

    /**
     * Initialize phase vocoder
     * @param sampleRate Audio sample rate
     * @param fftSize FFT size (must be power of 2)
     * @param hopSize Hop size for overlap-add
     */
    void initialize(double sampleRate, int fftSize, int hopSize);

    /**
     * Process audio with pitch shifting (RT-safe)
     * @param inputBuffer Input audio
     * @param outputBuffer Output audio
     * @param numSamples Number of samples to process
     * @param pitchRatio Pitch shift ratio (2.0 = +1 octave)
     */
    void process(const float* inputBuffer, float* outputBuffer, 
                 int numSamples, float pitchRatio);

    /**
     * Reset internal state
     */
    void reset();

private:
    void processFrame();
    void applyWindow(float* buffer, int size);
    
    std::unique_ptr<juce::dsp::FFT> m_fft;
    std::vector<float> m_inputBuffer;
    std::vector<float> m_outputBuffer;
    std::vector<float> m_windowBuffer;
    std::vector<std::complex<float>> m_fftBuffer;
    std::vector<float> m_lastPhase;
    std::vector<float> m_sumPhase;
    
    double m_sampleRate = 48000.0;
    int m_fftSize = 2048;
    int m_hopSize = 512;
    int m_inputPos = 0;
    int m_outputPos = 0;
};

/**
 * @class PitchCorrection
 * @brief Complete auto-tune effect processor
 * 
 * Features:
 * - Real-time pitch detection (YIN algorithm)
 * - Smooth pitch correction with configurable strength
 * - Musical scale quantization (Chromatic, Major, Minor, etc.)
 * - Formant preservation
 * - SIMD-optimized processing
 */
class PitchCorrection {
public:
    /**
     * Musical scale types for pitch quantization
     */
    enum class Scale {
        Chromatic,      // All 12 semitones
        Major,          // Major scale
        Minor,          // Natural minor
        Pentatonic,     // Pentatonic scale
        Blues,          // Blues scale
        Custom          // User-defined scale
    };

    /**
     * Processing mode
     */
    enum class Mode {
        Automatic,      // Auto-detect and correct
        Manual,         // Fixed target pitch
        Off             // Bypass
    };

    PitchCorrection();
    ~PitchCorrection() = default;

    /**
     * Initialize pitch correction processor
     * @param sampleRate Audio sample rate
     * @param maxBlockSize Maximum audio block size
     */
    void initialize(double sampleRate, int maxBlockSize);

    /**
     * Process audio buffer (RT-safe)
     * @param buffer Audio buffer to process in-place
     * @param numSamples Number of samples
     */
    void process(float* buffer, int numSamples);

    /**
     * Process stereo audio buffer (RT-safe)
     * @param leftBuffer Left channel
     * @param rightBuffer Right channel
     * @param numSamples Number of samples
     */
    void processStereo(float* leftBuffer, float* rightBuffer, int numSamples);

    /**
     * Reset processor state
     */
    void reset();

    // ============ Parameter Setters (RT-safe) ============

    /**
     * Set correction strength
     * @param strength 0.0 (no correction) to 1.0 (hard quantization)
     */
    void setStrength(float strength) noexcept {
        m_strength = juce::jlimit(0.0f, 1.0f, strength);
    }

    /**
     * Set correction speed (attack time)
     * @param speed 0.0 (slow) to 1.0 (instant)
     */
    void setSpeed(float speed) noexcept {
        m_speed = juce::jlimit(0.0f, 1.0f, speed);
    }

    /**
     * Set musical scale for quantization
     * @param scale Musical scale type
     */
    void setScale(Scale scale) noexcept {
        m_scale = scale;
        updateScaleNotes();
    }

    /**
     * Set root note for scale (0 = C, 1 = C#, etc.)
     * @param rootNote MIDI note number (0-11)
     */
    void setRootNote(int rootNote) noexcept {
        m_rootNote = rootNote % 12;
        updateScaleNotes();
    }

    /**
     * Set processing mode
     * @param mode Processing mode
     */
    void setMode(Mode mode) noexcept {
        m_mode = mode;
    }

    /**
     * Set manual target pitch (for Manual mode)
     * @param frequency Target frequency in Hz
     */
    void setTargetPitch(float frequency) noexcept {
        m_targetPitch = frequency;
    }

    /**
     * Enable/disable formant preservation
     * @param preserve True to preserve formants
     */
    void setFormantPreservation(bool preserve) noexcept {
        m_preserveFormants = preserve;
    }

    // ============ Parameter Getters ============

    float getStrength() const noexcept { return m_strength; }
    float getSpeed() const noexcept { return m_speed; }
    Scale getScale() const noexcept { return m_scale; }
    int getRootNote() const noexcept { return m_rootNote; }
    Mode getMode() const noexcept { return m_mode; }
    bool isFormantPreservationEnabled() const noexcept { return m_preserveFormants; }

    /**
     * Get current detected pitch
     * @return Frequency in Hz (0.0 if no pitch detected)
     */
    float getDetectedPitch() const noexcept { return m_detectedPitch; }

    /**
     * Get current corrected pitch
     * @return Frequency in Hz
     */
    float getCorrectedPitch() const noexcept { return m_correctedPitch; }

    /**
     * Get detection confidence
     * @return Confidence level (0.0 to 1.0)
     */
    float getConfidence() const noexcept { return m_pitchDetector.getConfidence(); }

private:
    // Helper functions
    void updateScaleNotes();
    float quantizePitch(float detectedFreq);
    float frequencyToMidi(float freq) const noexcept;
    float midiToFrequency(float midi) const noexcept;
    int findClosestScaleNote(int midiNote) const;
    float calculatePitchRatio(float currentFreq, float targetFreq) const noexcept;

    // Processing components
    PitchDetector m_pitchDetector;
    PhaseVocoder m_phaseVocoder;

    // Parameters
    float m_strength = 0.5f;        // Correction strength
    float m_speed = 0.5f;           // Correction speed
    Scale m_scale = Scale::Chromatic;
    int m_rootNote = 0;             // C
    Mode m_mode = Mode::Automatic;
    float m_targetPitch = 440.0f;   // A4
    bool m_preserveFormants = true;

    // State
    double m_sampleRate = 48000.0;
    float m_detectedPitch = 0.0f;
    float m_correctedPitch = 0.0f;
    float m_smoothedPitch = 0.0f;
    
    // Scale notes (enabled MIDI notes)
    std::array<bool, 12> m_scaleNotes;

    // Smoothing
    float m_smoothingCoeff = 0.95f;

    // Constants
    static constexpr float kA4Frequency = 440.0f;
    static constexpr float kMinFrequency = 60.0f;   // ~B1
    static constexpr float kMaxFrequency = 1500.0f; // ~G6
};

} // namespace omega
