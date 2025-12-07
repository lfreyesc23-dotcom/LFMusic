#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>

namespace OmegaStudio {

/**
 * @brief FL Studio 2025 Professional Mixer
 * 128 channels, advanced routing, grouping, sends/returns, sidechain
 */

class MixerChannel;
class EffectSlot;

/**
 * @brief Effect Slot in Mixer Channel
 */
class EffectSlot
{
public:
    EffectSlot(int slotIndex);
    ~EffectSlot();
    
    void loadEffect(const juce::String& effectName);
    void removeEffect();
    bool hasEffect() const { return effect != nullptr; }
    
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }
    
    void setDryWet(float mix) { dryWetMix = juce::jlimit(0.0f, 1.0f, mix); }
    float getDryWet() const { return dryWetMix; }
    
    void process(juce::AudioBuffer<float>& buffer);

private:
    int slotIndex;
    std::unique_ptr<juce::AudioProcessor> effect;
    bool isEnabled = true;
    float dryWetMix = 1.0f;
    juce::AudioBuffer<float> dryBuffer;
};

/**
 * @brief Mixer Send
 */
struct Send {
    int targetChannel = -1;
    float level = 0.0f;       // 0.0 to 1.0
    bool preFader = false;
    bool enabled = true;
};

/**
 * @brief Mixer Channel (128 available)
 */
class MixerChannel
{
public:
    MixerChannel(int channelNumber, const juce::String& name = "");
    ~MixerChannel();
    
    // Channel properties
    int getChannelNumber() const { return channelNumber; }
    void setName(const juce::String& name) { channelName = name; }
    juce::String getName() const { return channelName; }
    void setColour(juce::Colour colour) { channelColour = colour; }
    juce::Colour getColour() const { return channelColour; }
    
    // Level controls
    void setVolume(float volume) { this->volume = juce::jlimit(0.0f, 2.0f, volume); }
    float getVolume() const { return volume; }
    void setPan(float pan) { this->pan = juce::jlimit(-1.0f, 1.0f, pan); }
    float getPan() const { return pan; }
    
    // Channel state
    void setMuted(bool muted) { isMuted = muted; }
    bool getMuted() const { return isMuted; }
    void setSolo(bool solo) { isSolo = solo; }
    bool getSolo() const { return isSolo; }
    void setArmed(bool armed) { isArmed = armed; }
    bool getArmed() const { return isArmed; }
    
    // Routing
    void setRouting(int targetChannel) { routingTarget = targetChannel; }
    int getRouting() const { return routingTarget; }
    
    // Sends (12 sends per channel)
    void setSend(int sendIndex, const Send& send);
    Send getSend(int sendIndex) const;
    static constexpr int MaxSends = 12;
    
    // Sidechain
    void setSidechainSource(int sourceChannel) { sidechainSource = sourceChannel; }
    int getSidechainSource() const { return sidechainSource; }
    void setSidechainEnabled(bool enabled) { sidechainEnabled = enabled; }
    bool getSidechainEnabled() const { return sidechainEnabled; }
    
    // Effect slots (10 per channel)
    EffectSlot* getEffectSlot(int index);
    static constexpr int MaxEffectSlots = 10;
    
    // Metering
    float getPeakLevelLeft() const { return peakLevelL; }
    float getPeakLevelRight() const { return peakLevelR; }
    float getRMSLevelLeft() const { return rmsLevelL; }
    float getRMSLevelRight() const { return rmsLevelR; }
    void updateMeters(const juce::AudioBuffer<float>& buffer);
    void resetPeaks();
    
    // EQ (Built-in 7-band parametric)
    struct EQBand {
        bool enabled = false;
        float frequency = 1000.0f;
        float gain = 0.0f;
        float Q = 1.0f;
        enum class Type { LowShelf, Peak, HighShelf, LowPass, HighPass };
        Type type = Type::Peak;
    };
    void setEQBand(int band, const EQBand& eq);
    EQBand getEQBand(int band) const;
    void setEQEnabled(bool enabled) { eqEnabled = enabled; }
    bool getEQEnabled() const { return eqEnabled; }
    static constexpr int NumEQBands = 7;
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer, double sampleRate);
    void prepareToPlay(double sampleRate, int blockSize);
    
    // Icon/Image
    void setIcon(const juce::String& iconName) { this->iconName = iconName; }
    juce::String getIcon() const { return iconName; }

private:
    int channelNumber;
    juce::String channelName;
    juce::Colour channelColour;
    juce::String iconName;
    
    float volume = 0.8f;
    float pan = 0.0f;
    bool isMuted = false;
    bool isSolo = false;
    bool isArmed = false;
    
    int routingTarget = 0;  // 0 = Master
    std::array<Send, MaxSends> sends;
    
    int sidechainSource = -1;
    bool sidechainEnabled = false;
    
    std::array<std::unique_ptr<EffectSlot>, MaxEffectSlots> effectSlots;
    
    // Metering
    float peakLevelL = 0.0f;
    float peakLevelR = 0.0f;
    float rmsLevelL = 0.0f;
    float rmsLevelR = 0.0f;
    
    // Built-in EQ
    bool eqEnabled = false;
    std::array<EQBand, NumEQBands> eqBands;
    std::array<juce::dsp::IIR::Filter<float>, NumEQBands> eqFilters;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};

/**
 * @brief Mixer Group (for grouping channels)
 */
class MixerGroup
{
public:
    MixerGroup(const juce::String& name);
    ~MixerGroup();
    
    void setName(const juce::String& name) { groupName = name; }
    juce::String getName() const { return groupName; }
    
    void addChannel(int channelNumber);
    void removeChannel(int channelNumber);
    void clearChannels();
    const std::vector<int>& getChannels() const { return channels; }
    
    void setColour(juce::Colour colour) { groupColour = colour; }
    juce::Colour getColour() const { return groupColour; }

private:
    juce::String groupName;
    juce::Colour groupColour;
    std::vector<int> channels;
};

/**
 * @brief Main Mixer
 */
class Mixer
{
public:
    Mixer();
    ~Mixer();
    
    // Channel management
    MixerChannel* getChannel(int channelNumber);
    MixerChannel* getMasterChannel() { return getChannel(0); }
    static constexpr int NumChannels = 128;
    
    // Groups
    MixerGroup* createGroup(const juce::String& name);
    void deleteGroup(MixerGroup* group);
    std::vector<MixerGroup*> getAllGroups() { return groups; }
    
    // Processing
    void prepareToPlay(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Solo management (exclusive solo mode)
    bool hasSoloedChannels() const;
    std::vector<int> getSoloedChannels() const;
    
    // Routing validation
    bool isRoutingValid(int fromChannel, int toChannel) const;
    void fixRoutingIssues();
    
    // Snapshots (save/recall all mixer states)
    struct Snapshot {
        juce::String name;
        struct ChannelState {
            float volume;
            float pan;
            bool muted;
            bool solo;
            std::array<Send, MixerChannel::MaxSends> sends;
        };
        std::map<int, ChannelState> channelStates;
    };
    void saveSnapshot(const juce::String& name);
    void recallSnapshot(const juce::String& name);
    void morphToSnapshot(const juce::String& name, float morphAmount);
    std::vector<juce::String> getSnapshotNames() const;
    
    // Presets
    void saveChannelPreset(int channelNumber, const juce::String& name);
    void loadChannelPreset(int channelNumber, const juce::String& name);
    
    // Batch operations
    void resetAllChannels();
    void muteAllChannels();
    void unmuteAllChannels();
    void clearAllEffects();
    
    // Multitrack recording
    struct RecordingSettings {
        std::vector<int> channelsToRecord;
        juce::File outputFolder;
        bool recordIndividualFiles = true;
        bool recordMasterMix = true;
    };
    void startMultitrackRecording(const RecordingSettings& settings);
    void stopMultitrackRecording();
    bool isRecording() const { return recording; }
    
    // Analysis
    float getTotalCPULoad() const;
    std::vector<int> getChannelsOverThreshold(float threshold) const;
    
    // Serialization
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);

private:
    std::array<std::unique_ptr<MixerChannel>, NumChannels> channels;
    std::vector<MixerGroup*> groups;
    std::map<juce::String, Snapshot> snapshots;
    
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool recording = false;
    RecordingSettings recordingSettings;
    
    // Processing helpers
    void processSends(int channelNumber, juce::AudioBuffer<float>& buffer);
    void processSidechain(int channelNumber, const juce::AudioBuffer<float>& sidechainInput);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mixer)
};

} // namespace OmegaStudio
