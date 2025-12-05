/**
 * @file VocalEnhancer.h
 * @brief AI-powered vocal enhancement processor
 * 
 * Features:
 * - Formant detection and preservation
 * - Automatic EQ for vocal clarity
 * - Intelligent de-essing
 * - Breath removal
 * - Proximity effect compensation
 * - Dynamic range optimization
 * - Spectral shaping for broadcast quality
 */

#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include <vector>
#include "../../Utils/Constants.h"

namespace omega {

/**
 * @class FormantDetector
 * @brief Detects and tracks vocal formants (F1, F2, F3)
 */
class FormantDetector {
public:
    FormantDetector();
    ~FormantDetector() = default;

    /**
     * Initialize formant detector
     * @param sampleRate Audio sample rate
     */
    void initialize(double sampleRate);

    /**
     * Analyze audio buffer and detect formants
     * @param buffer Audio samples
     * @param numSamples Number of samples
     */
    void analyze(const float* buffer, int numSamples);

    /**
     * Get detected formant frequencies
     * @return Array of formant frequencies [F1, F2, F3] in Hz
     */
    std::array<float, 3> getFormants() const { return m_formants; }

    /**
     * Get formant detection confidence
     * @return Confidence (0.0 to 1.0)
     */
    float getConfidence() const { return m_confidence; }

    /**
     * Check if voice is detected
     */
    bool isVoiceDetected() const { return m_voiceDetected; }

private:
    void performLPC(); // Linear Predictive Coding
    void extractFormants();
    
    std::unique_ptr<juce::dsp::FFT> m_fft;
    std::vector<float> m_analysisBuffer;
    std::vector<float> m_spectrum;
    std::array<float, 3> m_formants = {800.0f, 1200.0f, 2500.0f}; // Default formants
    
    double m_sampleRate = 48000.0;
    float m_confidence = 0.0f;
    bool m_voiceDetected = false;
    
    static constexpr int kFFTOrder = 12; // 4096 samples
    static constexpr int kFFTSize = 1 << kFFTOrder;
};

/**
 * @class DeEsser
 * @brief Intelligent de-esser for sibilance reduction
 */
class DeEsser {
public:
    DeEsser();
    ~DeEsser() = default;

    /**
     * Initialize de-esser
     * @param sampleRate Audio sample rate
     */
    void initialize(double sampleRate);

    /**
     * Process audio (RT-safe)
     * @param buffer Audio buffer to process in-place
     * @param numSamples Number of samples
     */
    void process(float* buffer, int numSamples);

    /**
     * Reset processor state
     */
    void reset();

    /**
     * Set de-essing threshold
     * @param threshold Threshold in dB (-60 to 0)
     */
    void setThreshold(float threshold) { m_threshold = threshold; }

    /**
     * Set de-essing amount
     * @param amount Amount (0.0 to 1.0)
     */
    void setAmount(float amount) { m_amount = juce::jlimit(0.0f, 1.0f, amount); }

    /**
     * Set frequency range for de-essing
     * @param lowFreq Low frequency in Hz (4000-8000)
     * @param highFreq High frequency in Hz (8000-12000)
     */
    void setFrequencyRange(float lowFreq, float highFreq);

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> m_bandpassFilter;
    juce::dsp::Compressor<float> m_compressor;
    
    double m_sampleRate = 48000.0;
    float m_threshold = -20.0f;
    float m_amount = 0.5f;
    float m_lowFreq = 5000.0f;
    float m_highFreq = 10000.0f;
};

/**
 * @class BreathRemover
 * @brief Automatic breath sound detection and removal
 */
class BreathRemover {
public:
    BreathRemover();
    ~BreathRemover() = default;

    /**
     * Initialize breath remover
     * @param sampleRate Audio sample rate
     */
    void initialize(double sampleRate);

    /**
     * Process audio (RT-safe)
     * @param buffer Audio buffer to process in-place
     * @param numSamples Number of samples
     */
    void process(float* buffer, int numSamples);

    /**
     * Reset processor state
     */
    void reset();

    /**
     * Set sensitivity
     * @param sensitivity Sensitivity (0.0 = off, 1.0 = maximum)
     */
    void setSensitivity(float sensitivity) {
        m_sensitivity = juce::jlimit(0.0f, 1.0f, sensitivity);
    }

    /**
     * Set reduction amount
     * @param reduction Reduction in dB (0 to -60)
     */
    void setReduction(float reduction) {
        m_reduction = juce::jlimit(-60.0f, 0.0f, reduction);
    }

private:
    bool detectBreath(float energy, float spectralCentroid);
    float calculateSpectralCentroid(const float* buffer, int numSamples);
    
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> m_highpassFilter;
    
    double m_sampleRate = 48000.0;
    float m_sensitivity = 0.5f;
    float m_reduction = -12.0f;
    float m_gateState = 1.0f;
    
    static constexpr float kBreathFreqLow = 200.0f;
    static constexpr float kBreathFreqHigh = 3000.0f;
};

/**
 * @class VocalEQ
 * @brief Intelligent parametric EQ optimized for vocals
 */
class VocalEQ {
public:
    VocalEQ();
    ~VocalEQ() = default;

    /**
     * Initialize EQ
     * @param sampleRate Audio sample rate
     */
    void initialize(double sampleRate);

    /**
     * Process audio (RT-safe)
     * @param buffer Audio buffer to process in-place
     * @param numSamples Number of samples
     */
    void process(float* buffer, int numSamples);

    /**
     * Reset EQ state
     */
    void reset();

    /**
     * Auto-adjust EQ based on vocal characteristics
     * @param formants Detected formant frequencies
     */
    void autoAdjust(const std::array<float, 3>& formants);

    /**
     * Set manual EQ parameters
     */
    void setLowCut(float frequency);
    void setLowShelf(float frequency, float gain);
    void setMidPeak(float frequency, float gain, float q);
    void setPresence(float gain);
    void setBrightness(float gain);

private:
    using FilterType = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                       juce::dsp::IIR::Coefficients<float>>;
    
    FilterType m_lowCutFilter;
    FilterType m_lowShelfFilter;
    FilterType m_midPeakFilter;
    FilterType m_presenceFilter;
    FilterType m_brightnessFilter;
    
    double m_sampleRate = 48000.0;
    
    // Default frequencies
    static constexpr float kLowCutFreq = 80.0f;
    static constexpr float kLowShelfFreq = 200.0f;
    static constexpr float kMidPeakFreq = 2500.0f;
    static constexpr float kPresenceFreq = 5000.0f;
    static constexpr float kBrightnessFreq = 10000.0f;
};

/**
 * @class ProximityCompensator
 * @brief Compensates for proximity effect in close-mic recordings
 */
class ProximityCompensator {
public:
    ProximityCompensator();
    ~ProximityCompensator() = default;

    /**
     * Initialize compensator
     * @param sampleRate Audio sample rate
     */
    void initialize(double sampleRate);

    /**
     * Process audio (RT-safe)
     * @param buffer Audio buffer to process in-place
     * @param numSamples Number of samples
     */
    void process(float* buffer, int numSamples);

    /**
     * Reset state
     */
    void reset();

    /**
     * Set compensation amount
     * @param amount Amount (0.0 = no compensation, 1.0 = full)
     */
    void setAmount(float amount) {
        m_amount = juce::jlimit(0.0f, 1.0f, amount);
    }

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> m_highpassFilter;
    
    double m_sampleRate = 48000.0;
    float m_amount = 0.5f;
};

/**
 * @class VocalEnhancer
 * @brief Complete AI-powered vocal enhancement suite
 * 
 * Combines multiple processors for broadcast-quality vocal processing:
 * - Formant-preserving compression
 * - Intelligent EQ with auto-adjustment
 * - De-essing
 * - Breath removal
 * - Proximity effect compensation
 * - Spectral shaping
 * - Harmonic enhancement
 */
class VocalEnhancer {
public:
    /**
     * Processing mode
     */
    enum class Mode {
        Natural,        // Subtle enhancement
        Podcast,        // Clarity for spoken word
        Radio,          // Broadcast quality
        Studio,         // Professional vocal production
        Custom          // User-defined parameters
    };

    /**
     * Voice type (for optimization)
     */
    enum class VoiceType {
        Male,
        Female,
        Child,
        Auto            // Automatic detection
    };

    VocalEnhancer();
    ~VocalEnhancer() = default;

    /**
     * Initialize vocal enhancer
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
     * Reset all processors
     */
    void reset();

    /**
     * Auto-analyze and optimize for input signal
     * @param buffer Analysis buffer
     * @param numSamples Number of samples
     */
    void autoOptimize(const float* buffer, int numSamples);

    // ============ Parameter Setters ============

    /**
     * Set processing mode
     */
    void setMode(Mode mode);

    /**
     * Set voice type
     */
    void setVoiceType(VoiceType type) { m_voiceType = type; }

    /**
     * Set overall enhancement amount
     * @param amount Amount (0.0 = bypass, 1.0 = full)
     */
    void setAmount(float amount) {
        m_amount = juce::jlimit(0.0f, 1.0f, amount);
    }

    /**
     * Enable/disable individual processors
     */
    void setDeEsserEnabled(bool enabled) { m_deEsserEnabled = enabled; }
    void setBreathRemovalEnabled(bool enabled) { m_breathRemovalEnabled = enabled; }
    void setProximityCompensationEnabled(bool enabled) { m_proximityCompEnabled = enabled; }
    void setAutoEQEnabled(bool enabled) { m_autoEQEnabled = enabled; }

    /**
     * Set de-esser parameters
     */
    void setDeEsserAmount(float amount);
    void setDeEsserThreshold(float threshold);

    /**
     * Set breath removal parameters
     */
    void setBreathRemovalSensitivity(float sensitivity);
    void setBreathRemovalReduction(float reduction);

    /**
     * Set proximity compensation
     */
    void setProximityCompensation(float amount);

    /**
     * Set EQ parameters
     */
    void setLowCut(float frequency);
    void setPresence(float gain);
    void setBrightness(float gain);

    // ============ Parameter Getters ============

    Mode getMode() const { return m_mode; }
    VoiceType getVoiceType() const { return m_voiceType; }
    float getAmount() const { return m_amount; }

    /**
     * Get detected formants
     */
    std::array<float, 3> getDetectedFormants() const {
        return m_formantDetector.getFormants();
    }

    /**
     * Check if voice is currently detected
     */
    bool isVoiceDetected() const {
        return m_formantDetector.isVoiceDetected();
    }

    /**
     * Get input/output levels for metering
     */
    float getInputLevel() const { return m_inputLevel; }
    float getOutputLevel() const { return m_outputLevel; }

private:
    // Apply mode-specific settings
    void applyModeSettings();
    void detectVoiceType(const float* buffer, int numSamples);
    
    // Processing components
    FormantDetector m_formantDetector;
    DeEsser m_deEsser;
    BreathRemover m_breathRemover;
    VocalEQ m_vocalEQ;
    ProximityCompensator m_proximityComp;
    juce::dsp::Compressor<float> m_compressor;
    juce::dsp::Limiter<float> m_limiter;

    // Parameters
    Mode m_mode = Mode::Natural;
    VoiceType m_voiceType = VoiceType::Auto;
    float m_amount = 0.7f;

    // Enable/disable flags
    bool m_deEsserEnabled = true;
    bool m_breathRemovalEnabled = true;
    bool m_proximityCompEnabled = true;
    bool m_autoEQEnabled = true;

    // State
    double m_sampleRate = 48000.0;
    float m_inputLevel = 0.0f;
    float m_outputLevel = 0.0f;

    // Processing buffers
    std::vector<float> m_dryBuffer;
    std::vector<float> m_wetBuffer;

    // Analysis
    int m_analysisFrameCounter = 0;
    static constexpr int kAnalysisFrames = 4800; // Analyze every 100ms at 48kHz
};

} // namespace omega
