/**
 * @file MasteringBundle.h
 * @brief Professional mixing and mastering processors bundle
 * 
 * Includes: MultibandCompressor, TransientDesigner, StereoImager, MidSideProcessor
 */

#pragma once

#include <JuceHeader.h>
#include <array>
#include "../../Utils/Constants.h"

namespace omega {

//==============================================================================
/** MultibandCompressor - 4-band compressor */
class MultibandCompressor {
public:
    static constexpr int kNumBands = 4;
    
    struct BandSettings {
        bool enabled { true };
        float threshold { -20.0f };
        float ratio { 4.0f };
        float attack { 10.0f };
        float release { 100.0f };
        float makeupGain { 0.0f };
        float solo { false };
        float mute { false };
    };
    
    MultibandCompressor();
    void initialize(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Crossover frequencies (3 crossovers for 4 bands)
    void setCrossover(int index, float frequency);
    float getCrossover(int index) const;
    
    // Band control
    void setBandSettings(int band, const BandSettings& settings);
    const BandSettings& getBandSettings(int band) const;
    
    float getBandGainReduction(int band) const { return m_bandGainReduction[band]; }
    
private:
    void updateFilters();
    
    std::array<BandSettings, kNumBands> m_bands;
    std::array<float, 3> m_crossovers { 120.0f, 1000.0f, 8000.0f };
    std::array<float, kNumBands> m_bandGainReduction { 0.0f, 0.0f, 0.0f, 0.0f };
    
    std::array<juce::dsp::IIR::Filter<float>, kNumBands * 2> m_filters;  // Stereo
    juce::AudioBuffer<float> m_bandBuffers[kNumBands];
    
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** TransientDesigner - Attack/sustain shaping */
class TransientDesigner {
public:
    TransientDesigner();
    void initialize(double sampleRate, int maxBlockSize);
    void process(float* buffer, int numSamples);
    void reset();
    
    // Main parameters
    void setAttack(float amount) { m_attack = juce::jlimit(-12.0f, 12.0f, amount); }
    float getAttack() const { return m_attack; }
    
    void setSustain(float amount) { m_sustain = juce::jlimit(-12.0f, 12.0f, amount); }
    float getSustain() const { return m_sustain; }
    
    // Advanced
    void setSpeed(float speed) { m_speed = juce::jlimit(0.0f, 1.0f, speed); }
    float getSpeed() const { return m_speed; }
    
    void setClip(float clip) { m_clip = juce::jlimit(0.0f, 1.0f, clip); }
    float getClip() const { return m_clip; }
    
private:
    float detectTransient(const float* buffer, int start, int length);
    
    float m_attack { 0.0f };
    float m_sustain { 0.0f };
    float m_speed { 0.5f };
    float m_clip { 0.0f };
    
    juce::AudioBuffer<float> m_envelopeBuffer;
    float m_lastEnvelope { 0.0f };
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** StereoImager - Stereo width control */
class StereoImager {
public:
    StereoImager();
    void initialize(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Main parameters
    void setWidth(float width) { m_width = juce::jlimit(0.0f, 2.0f, width); }
    float getWidth() const { return m_width; }
    
    // Multiband width
    void setLowWidth(float width) { m_lowWidth = juce::jlimit(0.0f, 1.0f, width); }
    float getLowWidth() const { return m_lowWidth; }
    
    void setLowCrossover(float hz) { m_lowCrossover = hz; updateFilters(); }
    float getLowCrossover() const { return m_lowCrossover; }
    
    // Correlation meter
    float getCorrelation() const { return m_correlation; }
    
    // Safe bass (mono low end)
    void setSafeBass(bool enabled) { m_safeBass = enabled; }
    bool isSafeBassEnabled() const { return m_safeBass; }
    
private:
    void updateFilters();
    float calculateCorrelation(const float* left, const float* right, int numSamples);
    
    float m_width { 1.0f };
    float m_lowWidth { 0.0f };  // Mono bass
    float m_lowCrossover { 120.0f };
    bool m_safeBass { true };
    float m_correlation { 0.0f };
    
    juce::dsp::IIR::Filter<float> m_lowPassLeft, m_lowPassRight;
    juce::dsp::IIR::Filter<float> m_highPassLeft, m_highPassRight;
    
    juce::AudioBuffer<float> m_tempBuffer;
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** MidSideProcessor - Mid/Side processing */
class MidSideProcessor {
public:
    MidSideProcessor();
    void initialize(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Mid/Side balance
    void setMidGain(float dB) { m_midGain = juce::Decibels::decibelsToGain(dB); }
    float getMidGainDB() const { return juce::Decibels::gainToDecibels(m_midGain); }
    
    void setSideGain(float dB) { m_sideGain = juce::Decibels::decibelsToGain(dB); }
    float getSideGainDB() const { return juce::Decibels::gainToDecibels(m_sideGain); }
    
    // EQ (simplified 3-band)
    struct EQBand {
        bool enabled { false };
        float frequency { 1000.0f };
        float gain { 0.0f };
        float q { 1.0f };
    };
    
    void setMidEQ(int band, const EQBand& settings);
    void setSideEQ(int band, const EQBand& settings);
    
    // Compression
    void setMidCompression(bool enabled, float threshold, float ratio);
    void setSideCompression(bool enabled, float threshold, float ratio);
    
    // Monitoring
    enum class MonitorMode {
        Stereo,
        MidOnly,
        SideOnly
    };
    
    void setMonitorMode(MonitorMode mode) { m_monitorMode = mode; }
    MonitorMode getMonitorMode() const { return m_monitorMode; }
    
private:
    void encodeToMidSide(float* left, float* right, int numSamples);
    void decodeToLeftRight(float* mid, float* side, int numSamples);
    
    float m_midGain { 1.0f };
    float m_sideGain { 1.0f };
    
    std::array<EQBand, 3> m_midEQ;
    std::array<EQBand, 3> m_sideEQ;
    
    bool m_midCompEnabled { false };
    float m_midCompThreshold { -20.0f };
    float m_midCompRatio { 4.0f };
    
    bool m_sideCompEnabled { false };
    float m_sideCompThreshold { -20.0f };
    float m_sideCompRatio { 4.0f };
    
    MonitorMode m_monitorMode { MonitorMode::Stereo };
    
    juce::AudioBuffer<float> m_midBuffer, m_sideBuffer;
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** AdvancedMetering - Complete metering solution */
class AdvancedMetering {
public:
    struct MeteringData {
        float peakLeft { 0.0f };
        float peakRight { 0.0f };
        float rmsLeft { 0.0f };
        float rmsRight { 0.0f };
        float truePeakLeft { 0.0f };
        float truePeakRight { 0.0f };
        float lufsIntegrated { 0.0f };
        float lufsShortTerm { 0.0f };
        float lufsMomentary { 0.0f };
        float crestFactor { 0.0f };
        float dynamicRange { 0.0f };
        bool clipping { false };
    };
    
    AdvancedMetering();
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    const MeteringData& getMeteringData() const { return m_data; }
    
    // History
    std::vector<float> getPeakHistory(int channel) const;
    std::vector<float> getRMSHistory(int channel) const;
    
private:
    void calculateRMS(const float* buffer, int numSamples, float& rms);
    void calculateTruePeak(const float* buffer, int numSamples, float& truePeak);
    void calculateLUFS(const juce::AudioBuffer<float>& buffer);
    void calculateDynamicRange();
    
    MeteringData m_data;
    
    std::vector<float> m_peakHistoryL, m_peakHistoryR;
    std::vector<float> m_rmsHistoryL, m_rmsHistoryR;
    
    static constexpr int kHistorySize = 100;
    double m_sampleRate { 48000.0 };
};

} // namespace omega
