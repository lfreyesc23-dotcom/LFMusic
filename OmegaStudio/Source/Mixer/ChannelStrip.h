#pragma once

#include <JuceHeader.h>
#include <memory>

namespace omega {

/**
 * @brief Professional channel strip with Gate, EQ, Compressor
 */
class ChannelStrip {
public:
    struct Settings {
        // Gate
        bool gateEnabled = false;
        float gateThreshold = -40.0f;    // dB
        float gateRatio = 10.0f;
        float gateAttack = 1.0f;         // ms
        float gateRelease = 50.0f;       // ms
        
        // EQ (4-band parametric)
        bool eqEnabled = true;
        float lowShelfFreq = 80.0f;
        float lowShelfGain = 0.0f;       // dB
        float lowMidFreq = 500.0f;
        float lowMidGain = 0.0f;
        float lowMidQ = 1.0f;
        float highMidFreq = 2000.0f;
        float highMidGain = 0.0f;
        float highMidQ = 1.0f;
        float highShelfFreq = 8000.0f;
        float highShelfGain = 0.0f;
        
        // Compressor
        bool compEnabled = true;
        float compThreshold = -20.0f;
        float compRatio = 4.0f;
        float compAttack = 5.0f;
        float compRelease = 100.0f;
        float compKnee = 3.0f;
        float compMakeup = 0.0f;
        
        // Output
        float outputGain = 0.0f;         // dB
        bool phase = false;              // Phase invert
    };
    
    ChannelStrip();
    ~ChannelStrip();
    
    void initialize(double sampleRate, int maxBlockSize);
    void setSettings(const Settings& settings);
    Settings& getSettings() { return settings_; }
    
    void process(juce::AudioBuffer<float>& buffer);
    void processStereo(float* left, float* right, int numSamples);
    void reset();
    
    // Metering
    float getGainReduction() const { return gainReduction_; }
    float getInputLevel() const { return inputLevel_; }
    float getOutputLevel() const { return outputLevel_; }
    
private:
    Settings settings_;
    double sampleRate_;
    
    // Gate
    float gateEnvelope_;
    
    // EQ filters
    juce::dsp::IIR::Filter<float> lowShelf_[2];
    juce::dsp::IIR::Filter<float> lowMid_[2];
    juce::dsp::IIR::Filter<float> highMid_[2];
    juce::dsp::IIR::Filter<float> highShelf_[2];
    
    // Compressor state
    float compEnvelope_;
    float gainReduction_;
    
    // Metering
    float inputLevel_;
    float outputLevel_;
    
    void updateFilters();
    float processGate(float input);
    float processCompressor(float input);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};

/**
 * @brief VCA (Voltage Controlled Amplifier) group control system
 */
class VCASystem {
public:
    struct VCAGroup {
        juce::String name;
        float gain;              // Linear gain
        bool mute;
        bool solo;
        std::vector<int> memberChannels;
        
        VCAGroup() : name("VCA 1"), gain(1.0f), mute(false), solo(false) {}
    };
    
    VCASystem();
    ~VCASystem();
    
    // VCA management
    int createVCA(const juce::String& name);
    void deleteVCA(int vcaId);
    VCAGroup* getVCA(int vcaId);
    
    // Channel assignment
    void assignChannelToVCA(int channelId, int vcaId);
    void removeChannelFromVCA(int channelId, int vcaId);
    bool isChannelInVCA(int channelId, int vcaId) const;
    std::vector<int> getVCAsForChannel(int channelId) const;
    
    // VCA control
    void setVCAGain(int vcaId, float gainDB);
    void setVCAMute(int vcaId, bool mute);
    void setVCASolo(int vcaId, bool solo);
    
    // Get final multiplier for a channel
    float getChannelMultiplier(int channelId) const;
    bool isChannelMuted(int channelId) const;
    
    // Spill mode (show/hide VCA members)
    void setSpillMode(int vcaId, bool enabled);
    bool isSpillActive(int vcaId) const;
    
    int getNumVCAs() const { return static_cast<int>(vcas_.size()); }
    
private:
    std::map<int, VCAGroup> vcas_;
    std::map<int, bool> spillStates_;
    int nextVcaId_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VCASystem)
};

/**
 * @brief Monitor section with dim, mono, A/B comparison
 */
class MonitorSection {
public:
    MonitorSection();
    ~MonitorSection();
    
    void initialize(double sampleRate);
    void process(juce::AudioBuffer<float>& buffer);
    
    // Controls
    void setDim(bool enabled);
    void setDimLevel(float dB);           // -20 to 0 dB
    void setMono(bool enabled);
    void setMute(bool enabled);
    
    // A/B comparison
    void setABMode(bool useB);            // true = B, false = A
    void captureA(const juce::AudioBuffer<float>& reference);
    void captureB(const juce::AudioBuffer<float>& reference);
    
    // Cue mix (for monitoring different sources)
    void setCueMix(int sourceId, float level);  // 0.0-1.0
    
    bool isDimEnabled() const { return dimEnabled_; }
    bool isMonoEnabled() const { return monoEnabled_; }
    bool isABMode() const { return useB_; }
    
private:
    double sampleRate_;
    
    bool dimEnabled_;
    float dimLevel_;
    bool monoEnabled_;
    bool muteEnabled_;
    
    bool useB_;
    juce::AudioBuffer<float> referenceA_;
    juce::AudioBuffer<float> referenceB_;
    
    std::map<int, float> cueLevels_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonitorSection)
};

} // namespace omega
