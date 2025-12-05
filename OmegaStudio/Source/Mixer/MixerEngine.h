/*
  ==============================================================================
    MixerEngine.h
    
    Sistema profesional de mixer:
    - Channel strips con faders
    - Send/Return buses (reverb, delay sends)
    - Routing matrix flexible
    - Group tracks/buses
    - Pre/Post fader sends
    - Metering (Peak, RMS, LUFS)
    - Stereo panning (balance/stereo width)
    - Phase inversion
    
    Professional mixing console
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Audio/Plugins/PluginManager.h"
#include <memory>
#include <vector>
#include <map>

namespace OmegaStudio {

//==============================================================================
/** Tipos de ruteo */
enum class RoutingMode {
    Stereo,         // L+R estéreo normal
    Mono,           // Mono (suma L+R)
    Left,           // Solo canal izquierdo
    Right,          // Solo canal derecho
    MidSide         // Mid/Side encoding
};

//==============================================================================
/** Send to bus */
struct BusSend {
    int busIndex { -1 };
    float level { 0.0f };        // 0.0 - 1.0
    bool preFader { false };     // true = pre-fader, false = post-fader
    bool muted { false };
    
    juce::var toVar() const;
    static BusSend fromVar(const juce::var& v);
};

//==============================================================================
/** Peak/RMS Meter */
class LevelMeter {
public:
    LevelMeter();
    
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);
    
    // Peak levels (-inf to 0 dB)
    float getPeakLevel(int channel) const;
    float getPeakLevelDb(int channel) const;
    
    // RMS levels
    float getRMSLevel(int channel) const;
    float getRMSLevelDb(int channel) const;
    
    // Hold time for peak indicators
    void setHoldTime(double seconds) { holdTime = seconds; }
    
private:
    std::array<std::atomic<float>, 2> peakLevels { 0.0f, 0.0f };
    std::array<std::atomic<float>, 2> rmsLevels { 0.0f, 0.0f };
    
    std::array<float, 2> peakHold { 0.0f, 0.0f };
    std::array<double, 2> peakHoldTime { 0.0, 0.0 };
    
    double holdTime { 2.0 };
    double currentTime { 0.0 };
    
    static constexpr float MIN_DB = -60.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

//==============================================================================
/** Channel Strip - Un canal del mixer */
class ChannelStrip {
public:
    ChannelStrip(const juce::String& name = "Channel");
    ~ChannelStrip();
    
    // Properties
    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    
    juce::Colour getColour() const { return colour; }
    void setColour(juce::Colour newColour) { colour = newColour; }
    
    // Gain & Pan
    void setVolume(float volume);          // 0.0 - 1.0 (linear)
    float getVolume() const { return volume; }
    float getVolumeDb() const;
    
    void setPan(float pan);                // -1.0 (L) to +1.0 (R)
    float getPan() const { return pan; }
    
    void setGain(float gainDb);            // Trim gain in dB
    float getGain() const { return gainDb; }
    
    // Mute/Solo/Arm
    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    bool isMuted() const { return muted; }
    
    void setSoloed(bool shouldBeSoloed) { soloed = shouldBeSoloed; }
    bool isSoloed() const { return soloed; }
    
    void setArmed(bool shouldBeArmed) { armed = shouldBeArmed; }
    bool isArmed() const { return armed; }
    
    // Phase
    void setPhaseInverted(bool inverted) { phaseInverted = inverted; }
    bool isPhaseInverted() const { return phaseInverted; }
    
    // Routing
    void setRoutingMode(RoutingMode mode) { routingMode = mode; }
    RoutingMode getRoutingMode() const { return routingMode; }
    
    void setOutputBus(int busIndex) { outputBusIndex = busIndex; }
    int getOutputBus() const { return outputBusIndex; }
    
    // Sends
    void addSend(const BusSend& send);
    void removeSend(int index);
    void setSendLevel(int sendIndex, float level);
    int getNumSends() const { return static_cast<int>(sends.size()); }
    const BusSend& getSend(int index) const { return sends[index]; }
    BusSend& getSend(int index) { return sends[index]; }
    
    // Plugin Chain
    PluginChain& getPluginChain() { return pluginChain; }
    const PluginChain& getPluginChain() const { return pluginChain; }
    
    // Metering
    const LevelMeter& getInputMeter() const { return inputMeter; }
    const LevelMeter& getOutputMeter() const { return outputMeter; }
    
    // Processing (RT-safe)
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
    // Prepare
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();
    
    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<ChannelStrip> fromVar(const juce::var& v);
    
private:
    juce::String name;
    juce::Colour colour { juce::Colours::blue };
    
    float volume { 0.8f };
    float pan { 0.0f };
    float gainDb { 0.0f };
    
    bool muted { false };
    bool soloed { false };
    bool armed { false };
    bool phaseInverted { false };
    
    RoutingMode routingMode { RoutingMode::Stereo };
    int outputBusIndex { 0 }; // Master bus by default
    
    std::vector<BusSend> sends;
    PluginChain pluginChain;
    
    LevelMeter inputMeter;
    LevelMeter outputMeter;
    
    double sampleRate { 48000.0 };
    int blockSize { 512 };
    
    void applyGainAndPan(juce::AudioBuffer<float>& buffer);
    void applyRouting(juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};

//==============================================================================
/** Bus (Master, Group, o Send/Return) */
class MixerBus {
public:
    enum class Type {
        Master,         // Master output
        Group,          // Group bus (submix)
        SendReturn      // Send/Return bus (FX)
    };
    
    MixerBus(const juce::String& name, Type type);
    ~MixerBus();
    
    // Properties
    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    
    Type getType() const { return type; }
    
    // Gain
    void setVolume(float volume) { this->volume = volume; }
    float getVolume() const { return volume; }
    
    void setPan(float pan) { this->pan = pan; }
    float getPan() const { return pan; }
    
    // Mute
    void setMuted(bool shouldBeMuted) { muted = shouldBeMuted; }
    bool isMuted() const { return muted; }
    
    // Plugin Chain
    PluginChain& getPluginChain() { return pluginChain; }
    const PluginChain& getPluginChain() const { return pluginChain; }
    
    // Metering
    const LevelMeter& getMeter() const { return meter; }
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
    // Prepare
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();
    
    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<MixerBus> fromVar(const juce::var& v);
    
private:
    juce::String name;
    Type type;
    
    float volume { 0.8f };
    float pan { 0.0f };
    bool muted { false };
    
    PluginChain pluginChain;
    LevelMeter meter;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerBus)
};

//==============================================================================
/** Mixer Engine Principal */
class MixerEngine {
public:
    MixerEngine();
    ~MixerEngine();
    
    // Channels
    void addChannel(std::unique_ptr<ChannelStrip> channel);
    void removeChannel(int index);
    void clearChannels();
    
    int getNumChannels() const { return static_cast<int>(channels.size()); }
    ChannelStrip* getChannel(int index);
    const ChannelStrip* getChannel(int index) const;
    
    // Buses
    void addBus(std::unique_ptr<MixerBus> bus);
    void removeBus(int index);
    
    int getNumBuses() const { return static_cast<int>(buses.size()); }
    MixerBus* getBus(int index);
    const MixerBus* getBus(int index) const;
    
    MixerBus* getMasterBus() { return masterBus.get(); }
    const MixerBus* getMasterBus() const { return masterBus.get(); }
    
    // Solo management
    bool isAnySolo() const;
    void clearAllSolos();
    
    // Stats
    int getChannelCount() const { return static_cast<int>(channels.size()); }
    int getBusCount() const { return static_cast<int>(buses.size()); }
    
    // Processing (RT-safe)
    void process(std::vector<juce::AudioBuffer<float>*>& channelBuffers,
                 std::vector<juce::MidiBuffer*>& midiBuffers,
                 juce::AudioBuffer<float>& masterOutput);
    
    // Prepare
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();
    
    // Serialization
    juce::var toVar() const;
    void loadFromVar(const juce::var& v);
    
private:
    std::vector<std::unique_ptr<ChannelStrip>> channels;
    std::vector<std::unique_ptr<MixerBus>> buses;
    std::unique_ptr<MixerBus> masterBus;
    
    double sampleRate { 48000.0 };
    int blockSize { 512 };
    
    // Buffers for routing
    std::vector<juce::AudioBuffer<float>> busBuffers;
    
    void processSends(ChannelStrip& channel, const juce::AudioBuffer<float>& buffer);
    void routeToBuses();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerEngine)
};

} // namespace OmegaStudio
