//==============================================================================
// MultibandCompressor.h - Compresor Multibanda Profesional (4 bandas)
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>

namespace OmegaStudio {

//==============================================================================
/** Compresor Multibanda Profesional de 4 Bandas
 *  - 4 bandas independientes con crossovers Linkwitz-Riley
 *  - Controles completos por banda: threshold, ratio, attack, release, gain
 *  - Detección RMS o Peak
 *  - Knee ajustable
 *  - Sidechain por banda
 *  - Visualización de reducción de ganancia por banda
 */
class MultibandCompressor {
public:
    //==========================================================================
    enum class DetectionMode {
        Peak,       // Detección de picos
        RMS         // Detección RMS (más musical)
    };
    
    //==========================================================================
    struct BandSettings {
        // Frequency Range
        float crossoverLow = 120.0f;     // Hz (banda 1-2)
        float crossoverMid = 1000.0f;    // Hz (banda 2-3)
        float crossoverHigh = 8000.0f;   // Hz (banda 3-4)
        
        // Per-Band Compression
        struct Band {
            bool enabled = true;
            bool solo = false;
            bool mute = false;
            
            // Compressor params
            float threshold = -20.0f;    // dB
            float ratio = 4.0f;          // 1:1 - 20:1 (∞:1 para limiter)
            float attack = 10.0f;        // ms (0.1 - 100)
            float release = 100.0f;      // ms (10 - 1000)
            float knee = 6.0f;           // dB (0 = hard, 12 = soft)
            float makeupGain = 0.0f;     // dB (±24 dB)
            
            DetectionMode detectionMode = DetectionMode::RMS;
            
            // Auto makeup gain
            bool autoMakeup = false;
            
            // State
            float gainReduction = 0.0f;  // dB (para visualización)
        };
        
        std::array<Band, 4> bands;
    };
    
    //==========================================================================
    MultibandCompressor();
    ~MultibandCompressor() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Settings
    void setSettings(const BandSettings& settings);
    BandSettings& getSettings() { return settings_; }
    const BandSettings& getSettings() const { return settings_; }
    
    // Per-Band Control
    void setBandEnabled(int bandIndex, bool enabled);
    void setBandThreshold(int bandIndex, float thresholdDb);
    void setBandRatio(int bandIndex, float ratio);
    void setBandAttack(int bandIndex, float attackMs);
    void setBandRelease(int bandIndex, float releaseMs);
    void setBandKnee(int bandIndex, float kneeDb);
    void setBandMakeupGain(int bandIndex, float gainDb);
    void setBandAutoMakeup(int bandIndex, bool autoMakeup);
    void setBandDetectionMode(int bandIndex, DetectionMode mode);
    
    // Crossover Control
    void setCrossoverLow(float frequency);
    void setCrossoverMid(float frequency);
    void setCrossoverHigh(float frequency);
    
    // Solo/Mute
    void setBandSolo(int bandIndex, bool solo);
    void setBandMute(int bandIndex, bool mute);
    
    // Metering
    float getBandGainReduction(int bandIndex) const;
    float getBandInputLevel(int bandIndex) const;
    float getBandOutputLevel(int bandIndex) const;
    
    // Presets
    void loadPreset(const juce::String& presetName);
    juce::StringArray getPresetList() const;
    
    // Visualization
    struct BandSpectrum {
        std::array<float, 4> inputLevels{};
        std::array<float, 4> outputLevels{};
        std::array<float, 4> gainReductions{};
    };
    
    BandSpectrum getCurrentSpectrum() const;
    
private:
    //==========================================================================
    // Crossover Filter (Linkwitz-Riley 4th order)
    class LinkwitzRileyCrossover {
    public:
        void prepare(double sampleRate);
        void setCrossoverFrequency(float frequency);
        void process(const float* input, float* lowOut, float* highOut, int numSamples);
        void reset();
        
    private:
        double sampleRate_ = 48000.0;
        float frequency_ = 1000.0f;
        
        // Butterworth 2nd order (cascaded for LR4)
        std::array<float, 4> coeffsLow_{};
        std::array<float, 4> coeffsHigh_{};
        std::array<float, 4> stateLow_{};
        std::array<float, 4> stateHigh_{};
        
        void updateCoefficients();
    };
    
    //==========================================================================
    // Single Band Compressor
    class BandCompressor {
    public:
        void prepare(double sampleRate);
        void setParameters(const BandSettings::Band& params);
        void process(float* buffer, int numSamples);
        void reset();
        
        float getGainReduction() const { return gainReduction_; }
        float getInputLevel() const { return inputLevel_; }
        float getOutputLevel() const { return outputLevel_; }
        
    private:
        double sampleRate_ = 48000.0;
        BandSettings::Band params_;
        
        // Envelope followers
        float envelopeFollower_ = 0.0f;
        float gainReduction_ = 0.0f;
        float inputLevel_ = 0.0f;
        float outputLevel_ = 0.0f;
        
        // Coefficients
        float attackCoeff_ = 0.0f;
        float releaseCoeff_ = 0.0f;
        
        void updateCoefficients();
        float computeGain(float inputLevel);
    };
    
    //==========================================================================
    BandSettings settings_;
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    
    // Crossovers (3 crossovers para 4 bandas)
    std::array<LinkwitzRileyCrossover, 3> crossovers_;
    
    // Compressors (uno por banda)
    std::array<BandCompressor, 4> compressors_;
    
    // Buffers temporales para bandas
    juce::AudioBuffer<float> bandBuffers_[4];
    
    // Análisis
    std::array<float, 4> bandInputLevels_{};
    std::array<float, 4> bandOutputLevels_{};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultibandCompressor)
};

//==============================================================================
/** Presets Profesionales para Mastering */
class MultibandCompressorPresets {
public:
    static void applyPreset(MultibandCompressor& comp, const juce::String& presetName) {
        auto& settings = comp.getSettings();
        
        if (presetName == "Master Glue") {
            applyMasterGlue(settings);
        } else if (presetName == "Vocal Bus") {
            applyVocalBus(settings);
        } else if (presetName == "Drum Bus") {
            applyDrumBus(settings);
        } else if (presetName == "Mastering Gentle") {
            applyMasteringGentle(settings);
        } else if (presetName == "Mastering Aggressive") {
            applyMasteringAggressive(settings);
        } else if (presetName == "Bass Control") {
            applyBassControl(settings);
        } else if (presetName == "Broadcast") {
            applyBroadcast(settings);
        }
        
        comp.setSettings(settings);
    }
    
private:
    static void applyMasterGlue(MultibandCompressor::BandSettings& s) {
        // Crossovers
        s.crossoverLow = 120.0f;
        s.crossoverMid = 1000.0f;
        s.crossoverHigh = 8000.0f;
        
        // Band 0: Sub (20-120 Hz)
        s.bands[0].threshold = -18.0f;
        s.bands[0].ratio = 3.0f;
        s.bands[0].attack = 30.0f;
        s.bands[0].release = 200.0f;
        s.bands[0].knee = 6.0f;
        s.bands[0].autoMakeup = true;
        
        // Band 1: Low-Mid (120-1k Hz)
        s.bands[1].threshold = -15.0f;
        s.bands[1].ratio = 2.5f;
        s.bands[1].attack = 15.0f;
        s.bands[1].release = 150.0f;
        s.bands[1].knee = 6.0f;
        s.bands[1].autoMakeup = true;
        
        // Band 2: Mid-High (1k-8k Hz)
        s.bands[2].threshold = -12.0f;
        s.bands[2].ratio = 2.0f;
        s.bands[2].attack = 5.0f;
        s.bands[2].release = 100.0f;
        s.bands[2].knee = 6.0f;
        s.bands[2].autoMakeup = true;
        
        // Band 3: High (8k-20k Hz)
        s.bands[3].threshold = -10.0f;
        s.bands[3].ratio = 2.0f;
        s.bands[3].attack = 3.0f;
        s.bands[3].release = 80.0f;
        s.bands[3].knee = 4.0f;
        s.bands[3].autoMakeup = true;
    }
    
    static void applyVocalBus(MultibandCompressor::BandSettings& s) {
        s.crossoverLow = 200.0f;
        s.crossoverMid = 2000.0f;
        s.crossoverHigh = 8000.0f;
        
        s.bands[0].threshold = -20.0f;
        s.bands[0].ratio = 2.0f;
        s.bands[0].attack = 20.0f;
        s.bands[0].release = 150.0f;
        
        s.bands[1].threshold = -15.0f;
        s.bands[1].ratio = 3.0f;
        s.bands[1].attack = 8.0f;
        s.bands[1].release = 100.0f;
        
        s.bands[2].threshold = -12.0f;
        s.bands[2].ratio = 4.0f;
        s.bands[2].attack = 5.0f;
        s.bands[2].release = 80.0f;
        
        s.bands[3].threshold = -10.0f;
        s.bands[3].ratio = 2.5f;
        s.bands[3].attack = 3.0f;
        s.bands[3].release = 60.0f;
    }
    
    static void applyDrumBus(MultibandCompressor::BandSettings& s) {
        s.crossoverLow = 80.0f;
        s.crossoverMid = 500.0f;
        s.crossoverHigh = 5000.0f;
        
        s.bands[0].threshold = -12.0f;
        s.bands[0].ratio = 4.0f;
        s.bands[0].attack = 30.0f;
        s.bands[0].release = 250.0f;
        
        s.bands[1].threshold = -10.0f;
        s.bands[1].ratio = 3.0f;
        s.bands[1].attack = 5.0f;
        s.bands[1].release = 100.0f;
        
        s.bands[2].threshold = -8.0f;
        s.bands[2].ratio = 2.5f;
        s.bands[2].attack = 2.0f;
        s.bands[2].release = 80.0f;
        
        s.bands[3].threshold = -6.0f;
        s.bands[3].ratio = 2.0f;
        s.bands[3].attack = 1.0f;
        s.bands[3].release = 50.0f;
    }
    
    static void applyMasteringGentle(MultibandCompressor::BandSettings& s) {
        s.crossoverLow = 100.0f;
        s.crossoverMid = 1000.0f;
        s.crossoverHigh = 10000.0f;
        
        for (int i = 0; i < 4; ++i) {
            s.bands[i].threshold = -24.0f + i * 3.0f;
            s.bands[i].ratio = 1.5f + i * 0.2f;
            s.bands[i].attack = 25.0f - i * 5.0f;
            s.bands[i].release = 200.0f - i * 30.0f;
            s.bands[i].knee = 8.0f;
            s.bands[i].autoMakeup = true;
            s.bands[i].detectionMode = MultibandCompressor::DetectionMode::RMS;
        }
    }
    
    static void applyMasteringAggressive(MultibandCompressor::BandSettings& s) {
        s.crossoverLow = 120.0f;
        s.crossoverMid = 1200.0f;
        s.crossoverHigh = 8000.0f;
        
        for (int i = 0; i < 4; ++i) {
            s.bands[i].threshold = -18.0f + i * 2.0f;
            s.bands[i].ratio = 4.0f + i * 0.5f;
            s.bands[i].attack = 10.0f - i * 2.0f;
            s.bands[i].release = 120.0f - i * 20.0f;
            s.bands[i].knee = 4.0f;
            s.bands[i].autoMakeup = true;
        }
    }
    
    static void applyBassControl(MultibandCompressor::BandSettings& s) {
        s.crossoverLow = 150.0f;
        s.crossoverMid = 500.0f;
        s.crossoverHigh = 5000.0f;
        
        // Heavy en graves
        s.bands[0].threshold = -15.0f;
        s.bands[0].ratio = 6.0f;
        s.bands[0].attack = 40.0f;
        s.bands[0].release = 300.0f;
        
        s.bands[1].threshold = -12.0f;
        s.bands[1].ratio = 3.0f;
        s.bands[1].attack = 20.0f;
        s.bands[1].release = 150.0f;
        
        // Suave en agudos
        s.bands[2].threshold = -20.0f;
        s.bands[2].ratio = 2.0f;
        s.bands[2].attack = 10.0f;
        s.bands[2].release = 100.0f;
        
        s.bands[3].threshold = -24.0f;
        s.bands[3].ratio = 1.5f;
        s.bands[3].attack = 5.0f;
        s.bands[3].release = 80.0f;
    }
    
    static void applyBroadcast(MultibandCompressor::BandSettings& s) {
        s.crossoverLow = 250.0f;
        s.crossoverMid = 2000.0f;
        s.crossoverHigh = 8000.0f;
        
        for (int i = 0; i < 4; ++i) {
            s.bands[i].threshold = -10.0f;
            s.bands[i].ratio = 10.0f;  // Heavy
            s.bands[i].attack = 1.0f;  // Fast
            s.bands[i].release = 50.0f;
            s.bands[i].knee = 2.0f;
            s.bands[i].autoMakeup = true;
            s.bands[i].detectionMode = MultibandCompressor::DetectionMode::RMS;
        }
    }
};

} // namespace OmegaStudio
