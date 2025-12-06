//==============================================================================
// LimiterMaximizer.h - Professional Brickwall Limiter con Look-Ahead
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <algorithm>

namespace OmegaStudio {

//==============================================================================
/** Limitador/Maximizer Profesional con Look-Ahead
 *  - True Peak limiting (brickwall)
 *  - Look-ahead buffer para prevención de distorsión
 *  - ISP (Inter-Sample Peak) detection
 *  - Oversampling 2x/4x/8x
 *  - Dithering para reducir cuantización
 *  - Ceiling ajustable (-20 dB a 0 dB)
 *  - Auto-gain para maximizar loudness
 *  - Metering: LUFS, True Peak, RMS
 */
class LimiterMaximizer {
public:
    //==========================================================================
    enum class OversamplingFactor {
        None = 1,
        x2 = 2,
        x4 = 4,
        x8 = 8
    };
    
    enum class DitheringType {
        None,
        Rectangular,
        Triangular,
        Shaped         // Noise-shaped dithering
    };
    
    //==========================================================================
    struct Settings {
        // Limiter params
        float ceiling = -0.3f;          // dB (máximo nivel de salida)
        float threshold = -6.0f;        // dB (donde empieza el limiting)
        float release = 100.0f;         // ms (10 - 1000)
        float lookAhead = 5.0f;         // ms (0 - 20)
        
        // ISP Detection
        bool ispDetection = true;       // Inter-Sample Peak detection
        OversamplingFactor oversampling = OversamplingFactor::x4;
        
        // Auto Gain
        bool autoGain = false;          // Maximizar al ceiling automáticamente
        float inputGain = 0.0f;         // dB (±24 dB)
        
        // Dithering
        DitheringType dithering = DitheringType::Shaped;
        int bitDepth = 24;              // 16, 20, 24 bits
        
        // Metering
        bool meteringEnabled = true;
        
        // Soft Clip
        bool softClip = false;          // Soft clipping antes del ceiling
        float softClipAmount = 0.5f;    // 0.0 - 1.0
    };
    
    //==========================================================================
    LimiterMaximizer();
    ~LimiterMaximizer() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Settings
    void setSettings(const Settings& settings);
    Settings& getSettings() { return settings_; }
    const Settings& getSettings() const { return settings_; }
    
    // Parameter Control
    void setCeiling(float ceilingDb);
    void setThreshold(float thresholdDb);
    void setRelease(float releaseMs);
    void setLookAhead(float lookAheadMs);
    void setAutoGain(bool enabled);
    void setInputGain(float gainDb);
    void setOversampling(OversamplingFactor factor);
    void setDithering(DitheringType type, int bitDepth);
    void setSoftClip(bool enabled, float amount);
    
    // Metering
    struct MeteringData {
        float inputPeak = 0.0f;         // dB
        float outputPeak = 0.0f;        // dB
        float truePeak = 0.0f;          // dBTP (True Peak)
        float gainReduction = 0.0f;     // dB
        float lufsShortTerm = 0.0f;     // LUFS (3 seconds)
        float lufsMomentary = 0.0f;     // LUFS (400 ms)
        float lufsIntegrated = 0.0f;    // LUFS (gated, entire track)
        float rms = 0.0f;               // dB
        float crestFactor = 0.0f;       // dB (Peak - RMS)
        int clippedSamples = 0;         // Número de samples limitados
    };
    
    MeteringData getCurrentMetering() const;
    void resetMetering();
    
    // Presets
    void loadPreset(const juce::String& presetName);
    juce::StringArray getPresetList() const;
    
private:
    //==========================================================================
    // Look-ahead delay line
    class LookAheadBuffer {
    public:
        void prepare(int maxSamples);
        void write(float sample);
        float read() const;
        void advance();
        void reset();
        
    private:
        std::vector<float> buffer_;
        int writePos_ = 0;
        int readPos_ = 0;
        int delay_ = 0;
    };
    
    //==========================================================================
    // True Peak detector (con oversampling)
    class TruePeakDetector {
    public:
        void prepare(double sampleRate, OversamplingFactor factor);
        float detectTruePeak(const float* buffer, int numSamples);
        void reset();
        
    private:
        juce::dsp::Oversampling<float> oversampler_{1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};
        double sampleRate_ = 48000.0;
        OversamplingFactor factor_ = OversamplingFactor::x4;
    };
    
    //==========================================================================
    // LUFS Meter (EBU R128)
    class LUFSMeter {
    public:
        void prepare(double sampleRate);
        void process(const juce::AudioBuffer<float>& buffer);
        float getMomentary() const { return momentaryLUFS_; }
        float getShortTerm() const { return shortTermLUFS_; }
        float getIntegrated() const { return integratedLUFS_; }
        void reset();
        
    private:
        double sampleRate_ = 48000.0;
        
        // K-weighting filters
        juce::dsp::IIR::Filter<float> preFilter_;
        juce::dsp::IIR::Filter<float> highShelfFilter_;
        
        // Buffers para windows
        std::vector<float> momentaryWindow_;  // 400 ms
        std::vector<float> shortTermWindow_;  // 3 s
        std::vector<float> integratedBuffer_; // Todo el track
        
        float momentaryLUFS_ = -70.0f;
        float shortTermLUFS_ = -70.0f;
        float integratedLUFS_ = -70.0f;
        
        int windowPos_ = 0;
        
        void updateLUFS();
        float calculateLoudness(const std::vector<float>& samples);
    };
    
    //==========================================================================
    // Dithering
    class Ditherer {
    public:
        void prepare(DitheringType type, int bitDepth);
        float apply(float sample);
        void reset();
        
    private:
        DitheringType type_ = DitheringType::None;
        int bitDepth_ = 24;
        float ditherAmount_ = 0.0f;
        
        // Para TPDF (Triangular PDF)
        float lastRandom_ = 0.0f;
        
        // Para shaped dithering
        std::array<float, 8> shapingFilter_{};
        
        juce::Random random_;
    };
    
    //==========================================================================
    Settings settings_;
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    
    // Look-ahead buffers (uno por canal)
    std::vector<LookAheadBuffer> lookAheadBuffers_;
    
    // Envelope follower para gain reduction
    float envelopeFollower_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    // True peak detection
    TruePeakDetector truePeakDetector_;
    
    // LUFS metering
    LUFSMeter lufsMeter_;
    
    // Dithering
    Ditherer ditherer_;
    
    // Metering
    MeteringData metering_;
    int totalSamplesProcessed_ = 0;
    
    // Soft clipping
    float softClip(float sample, float amount);
    
    // Gain calculation
    float calculateGainReduction(float peakLevel);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterMaximizer)
};

//==============================================================================
/** Presets Profesionales para Mastering */
class LimiterMaximizerPresets {
public:
    static void applyPreset(LimiterMaximizer& limiter, const juce::String& presetName) {
        auto& settings = limiter.getSettings();
        
        if (presetName == "Transparent") {
            settings.ceiling = -0.3f;
            settings.threshold = -6.0f;
            settings.release = 250.0f;
            settings.lookAhead = 10.0f;
            settings.autoGain = false;
            settings.softClip = false;
            settings.oversampling = LimiterMaximizer::OversamplingFactor::x4;
        } 
        else if (presetName == "Loud Master") {
            settings.ceiling = -0.1f;
            settings.threshold = -3.0f;
            settings.release = 100.0f;
            settings.lookAhead = 5.0f;
            settings.autoGain = true;
            settings.softClip = true;
            settings.softClipAmount = 0.3f;
            settings.oversampling = LimiterMaximizer::OversamplingFactor::x8;
        } 
        else if (presetName == "Streaming (-14 LUFS)") {
            settings.ceiling = -1.0f;
            settings.threshold = -14.0f;
            settings.release = 300.0f;
            settings.lookAhead = 8.0f;
            settings.autoGain = false;
            settings.softClip = false;
            settings.oversampling = LimiterMaximizer::OversamplingFactor::x4;
        } 
        else if (presetName == "Broadcast") {
            settings.ceiling = -0.5f;
            settings.threshold = -8.0f;
            settings.release = 150.0f;
            settings.lookAhead = 12.0f;
            settings.autoGain = true;
            settings.softClip = true;
            settings.softClipAmount = 0.5f;
            settings.oversampling = LimiterMaximizer::OversamplingFactor::x4;
        } 
        else if (presetName == "Mastering Gentle") {
            settings.ceiling = -0.3f;
            settings.threshold = -9.0f;
            settings.release = 400.0f;
            settings.lookAhead = 15.0f;
            settings.autoGain = false;
            settings.softClip = false;
            settings.oversampling = LimiterMaximizer::OversamplingFactor::x4;
        } 
        else if (presetName == "Club Banger") {
            settings.ceiling = -0.1f;
            settings.threshold = -2.0f;
            settings.release = 50.0f;
            settings.lookAhead = 3.0f;
            settings.autoGain = true;
            settings.softClip = true;
            settings.softClipAmount = 0.7f;
            settings.oversampling = LimiterMaximizer::OversamplingFactor::x8;
        }
        
        settings.ispDetection = true;
        settings.dithering = LimiterMaximizer::DitheringType::Shaped;
        settings.bitDepth = 24;
        settings.meteringEnabled = true;
        
        limiter.setSettings(settings);
    }
};

} // namespace OmegaStudio
