/*
  ==============================================================================
    MixerEngine.cpp
    Implementation del mixer profesional
  ==============================================================================
*/

#include "MixerEngine.h"
#include <cmath>

namespace OmegaStudio {

//==============================================================================
// Helper functions
//==============================================================================

static float dbToGain(float db) {
    return std::pow(10.0f, db / 20.0f);
}

static float gainToDb(float gain) {
    return 20.0f * std::log10(std::max(gain, 0.00001f));
}

//==============================================================================
// BusSend Implementation
//==============================================================================

juce::var BusSend::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("busIndex", busIndex);
    obj->setProperty("level", level);
    obj->setProperty("preFader", preFader);
    obj->setProperty("muted", muted);
    return juce::var(obj.get());
}

BusSend BusSend::fromVar(const juce::var& v) {
    BusSend send;
    if (auto* obj = v.getDynamicObject()) {
        send.busIndex = obj->getProperty("busIndex");
        send.level = obj->getProperty("level");
        send.preFader = obj->getProperty("preFader");
        send.muted = obj->getProperty("muted");
    }
    return send;
}

//==============================================================================
// LevelMeter Implementation
//==============================================================================

LevelMeter::LevelMeter() {
    reset();
}

void LevelMeter::reset() {
    for (int i = 0; i < 2; ++i) {
        peakLevels[i] = 0.0f;
        rmsLevels[i] = 0.0f;
        peakHold[i] = 0.0f;
        peakHoldTime[i] = 0.0;
    }
}

void LevelMeter::process(const juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* channelData = buffer.getReadPointer(ch);
        
        // Peak
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax(peak, std::abs(channelData[i]));
        
        peakLevels[ch] = peak;
        
        // Peak hold
        if (peak > peakHold[ch]) {
            peakHold[ch] = peak;
            peakHoldTime[ch] = currentTime;
        }
        else if (currentTime - peakHoldTime[ch] > holdTime) {
            peakHold[ch] = peak;
        }
        
        // RMS
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sum += channelData[i] * channelData[i];
        
        float rms = std::sqrt(sum / numSamples);
        rmsLevels[ch] = rms;
    }
    
    currentTime += numSamples / 48000.0; // Approximate
}

float LevelMeter::getPeakLevel(int channel) const {
    return channel >= 0 && channel < 2 ? peakLevels[channel].load() : 0.0f;
}

float LevelMeter::getPeakLevelDb(int channel) const {
    float level = getPeakLevel(channel);
    return level > 0.00001f ? gainToDb(level) : MIN_DB;
}

float LevelMeter::getRMSLevel(int channel) const {
    return channel >= 0 && channel < 2 ? rmsLevels[channel].load() : 0.0f;
}

float LevelMeter::getRMSLevelDb(int channel) const {
    float level = getRMSLevel(channel);
    return level > 0.00001f ? gainToDb(level) : MIN_DB;
}

//==============================================================================
// ChannelStrip Implementation
//==============================================================================

ChannelStrip::ChannelStrip(const juce::String& name) : name(name) {}

ChannelStrip::~ChannelStrip() = default;

void ChannelStrip::setVolume(float newVolume) {
    volume = juce::jlimit(0.0f, 1.0f, newVolume);
}

float ChannelStrip::getVolumeDb() const {
    return gainToDb(volume);
}

void ChannelStrip::setPan(float newPan) {
    pan = juce::jlimit(-1.0f, 1.0f, newPan);
}

void ChannelStrip::setGain(float newGainDb) {
    gainDb = juce::jlimit(-60.0f, 24.0f, newGainDb);
}

void ChannelStrip::addSend(const BusSend& send) {
    sends.push_back(send);
}

void ChannelStrip::removeSend(int index) {
    if (index >= 0 && index < getNumSends())
        sends.erase(sends.begin() + index);
}

void ChannelStrip::setSendLevel(int sendIndex, float level) {
    if (sendIndex >= 0 && sendIndex < getNumSends())
        sends[sendIndex].level = juce::jlimit(0.0f, 1.0f, level);
}

void ChannelStrip::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;
    
    // Input metering
    inputMeter.process(buffer);
    
    // Mute
    if (muted) {
        buffer.clear();
        return;
    }
    
    // Trim gain
    if (gainDb != 0.0f) {
        float gain = dbToGain(gainDb);
        buffer.applyGain(gain);
    }
    
    // Phase inversion
    if (phaseInverted) {
        buffer.applyGain(-1.0f);
    }
    
    // Plugin chain
    pluginChain.process(buffer, midiMessages);
    
    // Apply gain and pan
    applyGainAndPan(buffer);
    
    // Routing
    applyRouting(buffer);
    
    // Output metering
    outputMeter.process(buffer);
}

void ChannelStrip::applyGainAndPan(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    if (numChannels == 1) {
        // Mono: just apply volume
        buffer.applyGain(0, 0, numSamples, volume);
    }
    else if (numChannels >= 2) {
        // Stereo: apply volume and pan
        float leftGain = volume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
        float rightGain = volume * (pan >= 0.0f ? 1.0f : 1.0f + pan);
        
        buffer.applyGain(0, 0, numSamples, leftGain);
        buffer.applyGain(1, 0, numSamples, rightGain);
    }
}

void ChannelStrip::applyRouting(juce::AudioBuffer<float>& buffer) {
    switch (routingMode) {
        case RoutingMode::Stereo:
            // Normal stereo - no change
            break;
            
        case RoutingMode::Mono:
            if (buffer.getNumChannels() >= 2) {
                // Sum L+R to mono
                buffer.addFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());
                buffer.applyGain(0, 0, buffer.getNumSamples(), 0.5f);
                buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
            }
            break;
            
        case RoutingMode::Left:
            if (buffer.getNumChannels() >= 2) {
                buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
            }
            break;
            
        case RoutingMode::Right:
            if (buffer.getNumChannels() >= 2) {
                buffer.copyFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());
            }
            break;
            
        case RoutingMode::MidSide:
            if (buffer.getNumChannels() >= 2) {
                // Convert L/R to M/S
                auto* left = buffer.getWritePointer(0);
                auto* right = buffer.getWritePointer(1);
                
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    float l = left[i];
                    float r = right[i];
                    left[i] = (l + r) * 0.5f;  // Mid
                    right[i] = (l - r) * 0.5f; // Side
                }
            }
            break;
    }
}

void ChannelStrip::prepareToPlay(double newSampleRate, int maximumExpectedSamplesPerBlock) {
    sampleRate = newSampleRate;
    blockSize = maximumExpectedSamplesPerBlock;
    
    pluginChain.prepareToPlay(sampleRate, blockSize);
    inputMeter.reset();
    outputMeter.reset();
}

void ChannelStrip::releaseResources() {
    pluginChain.releaseResources();
}

juce::var ChannelStrip::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("colour", colour.toString());
    obj->setProperty("volume", volume);
    obj->setProperty("pan", pan);
    obj->setProperty("gainDb", gainDb);
    obj->setProperty("muted", muted);
    obj->setProperty("soloed", soloed);
    obj->setProperty("armed", armed);
    obj->setProperty("phaseInverted", phaseInverted);
    obj->setProperty("outputBusIndex", outputBusIndex);
    
    juce::var sendsArray;
    for (const auto& send : sends)
        sendsArray.append(send.toVar());
    obj->setProperty("sends", sendsArray);
    
    obj->setProperty("pluginChain", pluginChain.getState());
    
    return juce::var(obj.get());
}

std::unique_ptr<ChannelStrip> ChannelStrip::fromVar(const juce::var& v) {
    auto channel = std::make_unique<ChannelStrip>();
    
    if (auto* obj = v.getDynamicObject()) {
        channel->name = obj->getProperty("name").toString();
        channel->colour = juce::Colour::fromString(obj->getProperty("colour").toString());
        channel->volume = obj->getProperty("volume");
        channel->pan = obj->getProperty("pan");
        channel->gainDb = obj->getProperty("gainDb");
        channel->muted = obj->getProperty("muted");
        channel->soloed = obj->getProperty("soloed");
        channel->armed = obj->getProperty("armed");
        channel->phaseInverted = obj->getProperty("phaseInverted");
        channel->outputBusIndex = obj->getProperty("outputBusIndex");
        
        if (auto* sendsArray = obj->getProperty("sends").getArray()) {
            for (const auto& sendVar : *sendsArray)
                channel->sends.push_back(BusSend::fromVar(sendVar));
        }
        
        channel->pluginChain.setState(obj->getProperty("pluginChain"));
    }
    
    return channel;
}

//==============================================================================
// MixerBus Implementation
//==============================================================================

MixerBus::MixerBus(const juce::String& name, Type type) : name(name), type(type) {}

MixerBus::~MixerBus() = default;

void MixerBus::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (muted) {
        buffer.clear();
        return;
    }
    
    // Plugin chain
    pluginChain.process(buffer, midiMessages);
    
    // Apply volume and pan
    if (buffer.getNumChannels() >= 2) {
        float leftGain = volume * (pan <= 0.0f ? 1.0f : 1.0f - pan);
        float rightGain = volume * (pan >= 0.0f ? 1.0f : 1.0f + pan);
        
        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }
    else {
        buffer.applyGain(volume);
    }
    
    // Metering
    meter.process(buffer);
}

void MixerBus::prepareToPlay(double newSampleRate, int maximumExpectedSamplesPerBlock) {
    pluginChain.prepareToPlay(newSampleRate, maximumExpectedSamplesPerBlock);
    meter.reset();
}

void MixerBus::releaseResources() {
    pluginChain.releaseResources();
}

juce::var MixerBus::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("type", static_cast<int>(type));
    obj->setProperty("volume", volume);
    obj->setProperty("pan", pan);
    obj->setProperty("muted", muted);
    obj->setProperty("pluginChain", pluginChain.getState());
    return juce::var(obj.get());
}

std::unique_ptr<MixerBus> MixerBus::fromVar(const juce::var& v) {
    auto bus = std::make_unique<MixerBus>("Bus", Type::Group);
    
    if (auto* obj = v.getDynamicObject()) {
        bus->name = obj->getProperty("name").toString();
        bus->type = static_cast<Type>(int(obj->getProperty("type")));
        bus->volume = obj->getProperty("volume");
        bus->pan = obj->getProperty("pan");
        bus->muted = obj->getProperty("muted");
        bus->pluginChain.setState(obj->getProperty("pluginChain"));
    }
    
    return bus;
}

//==============================================================================
// MixerEngine Implementation
//==============================================================================

MixerEngine::MixerEngine() {
    masterBus = std::make_unique<MixerBus>("Master", MixerBus::Type::Master);
}

MixerEngine::~MixerEngine() = default;

void MixerEngine::addChannel(std::unique_ptr<ChannelStrip> channel) {
    channels.push_back(std::move(channel));
}

void MixerEngine::removeChannel(int index) {
    if (index >= 0 && index < getNumChannels())
        channels.erase(channels.begin() + index);
}

void MixerEngine::clearChannels() {
    channels.clear();
}

ChannelStrip* MixerEngine::getChannel(int index) {
    return (index >= 0 && index < getNumChannels()) ? channels[index].get() : nullptr;
}

const ChannelStrip* MixerEngine::getChannel(int index) const {
    return (index >= 0 && index < getNumChannels()) ? channels[index].get() : nullptr;
}

void MixerEngine::addBus(std::unique_ptr<MixerBus> bus) {
    buses.push_back(std::move(bus));
}

void MixerEngine::removeBus(int index) {
    if (index >= 0 && index < getNumBuses())
        buses.erase(buses.begin() + index);
}

MixerBus* MixerEngine::getBus(int index) {
    return (index >= 0 && index < getNumBuses()) ? buses[index].get() : nullptr;
}

const MixerBus* MixerEngine::getBus(int index) const {
    return (index >= 0 && index < getNumBuses()) ? buses[index].get() : nullptr;
}

bool MixerEngine::isAnySolo() const {
    for (const auto& channel : channels) {
        if (channel->isSoloed())
            return true;
    }
    return false;
}

void MixerEngine::clearAllSolos() {
    for (auto& channel : channels)
        channel->setSoloed(false);
}

void MixerEngine::process(std::vector<juce::AudioBuffer<float>*>& channelBuffers,
                          std::vector<juce::MidiBuffer*>& midiBuffers,
                          juce::AudioBuffer<float>& masterOutput) {
    masterOutput.clear();
    
    bool anySolo = isAnySolo();
    
    // Process each channel
    for (size_t i = 0; i < channels.size() && i < channelBuffers.size(); ++i) {
        auto& channel = channels[i];
        auto* buffer = channelBuffers[i];
        auto* midiBuffer = (i < midiBuffers.size()) ? midiBuffers[i] : nullptr;
        
        if (!buffer) continue;
        
        // Skip if soloing and this isn't soloed
        if (anySolo && !channel->isSoloed())
            continue;
        
        // Process channel
        juce::MidiBuffer emptyMidi;
        channel->process(*buffer, midiBuffer ? *midiBuffer : emptyMidi);
        
        // Add to master
        for (int ch = 0; ch < juce::jmin(buffer->getNumChannels(), masterOutput.getNumChannels()); ++ch) {
            masterOutput.addFrom(ch, 0, *buffer, ch, 0, buffer->getNumSamples());
        }
    }
    
    // Process master bus
    juce::MidiBuffer emptyMidi;
    masterBus->process(masterOutput, emptyMidi);
}

void MixerEngine::prepareToPlay(double newSampleRate, int maximumExpectedSamplesPerBlock) {
    sampleRate = newSampleRate;
    blockSize = maximumExpectedSamplesPerBlock;
    
    for (auto& channel : channels)
        channel->prepareToPlay(sampleRate, blockSize);
    
    for (auto& bus : buses)
        bus->prepareToPlay(sampleRate, blockSize);
    
    masterBus->prepareToPlay(sampleRate, blockSize);
    
    // Prepare bus buffers
    busBuffers.resize(buses.size() + 1); // +1 for master
    for (auto& buffer : busBuffers)
        buffer.setSize(2, blockSize);
}

void MixerEngine::releaseResources() {
    for (auto& channel : channels)
        channel->releaseResources();
    
    for (auto& bus : buses)
        bus->releaseResources();
    
    masterBus->releaseResources();
}

juce::var MixerEngine::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    juce::var channelsArray;
    for (const auto& channel : channels)
        channelsArray.append(channel->toVar());
    obj->setProperty("channels", channelsArray);
    
    juce::var busesArray;
    for (const auto& bus : buses)
        busesArray.append(bus->toVar());
    obj->setProperty("buses", busesArray);
    
    obj->setProperty("masterBus", masterBus->toVar());
    
    return juce::var(obj.get());
}

void MixerEngine::loadFromVar(const juce::var& v) {
    channels.clear();
    buses.clear();
    
    if (auto* obj = v.getDynamicObject()) {
        if (auto* channelsArray = obj->getProperty("channels").getArray()) {
            for (const auto& channelVar : *channelsArray) {
                auto channel = ChannelStrip::fromVar(channelVar);
                if (channel)
                    channels.push_back(std::move(channel));
            }
        }
        
        if (auto* busesArray = obj->getProperty("buses").getArray()) {
            for (const auto& busVar : *busesArray) {
                auto bus = MixerBus::fromVar(busVar);
                if (bus)
                    buses.push_back(std::move(bus));
            }
        }
        
        masterBus = MixerBus::fromVar(obj->getProperty("masterBus"));
    }
}

void MixerEngine::processSends(ChannelStrip& channel, const juce::AudioBuffer<float>& buffer) {
    // TODO: Implement send processing to buses
}

void MixerEngine::routeToBuses() {
    // TODO: Implement bus routing
}

} // namespace OmegaStudio
