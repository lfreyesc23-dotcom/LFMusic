#include "ChannelStrip.h"
#include <cmath>
#include <algorithm>

namespace omega {

//==============================================================================
// ChannelStrip Implementation
//==============================================================================

ChannelStrip::ChannelStrip()
    : sampleRate_(44100.0)
    , gateEnvelope_(0.0f)
    , compEnvelope_(0.0f)
    , gainReduction_(0.0f)
    , inputLevel_(0.0f)
    , outputLevel_(0.0f)
{
}

ChannelStrip::~ChannelStrip()
{
}

void ChannelStrip::initialize(double sampleRate, int /* maxBlockSize */)
{
    sampleRate_ = sampleRate;
    
    // Initialize filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf_[ch].prepare(spec);
        lowMid_[ch].prepare(spec);
        highMid_[ch].prepare(spec);
        highShelf_[ch].prepare(spec);
    }
    
    updateFilters();
}

void ChannelStrip::setSettings(const Settings& settings)
{
    settings_ = settings;
    updateFilters();
}

void ChannelStrip::updateFilters()
{
    // Low shelf
    auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate_, settings_.lowShelfFreq, 0.707f, 
        juce::Decibels::decibelsToGain(settings_.lowShelfGain)
    );
    
    // Low mid bell
    auto lowMidCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate_, settings_.lowMidFreq, settings_.lowMidQ,
        juce::Decibels::decibelsToGain(settings_.lowMidGain)
    );
    
    // High mid bell
    auto highMidCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate_, settings_.highMidFreq, settings_.highMidQ,
        juce::Decibels::decibelsToGain(settings_.highMidGain)
    );
    
    // High shelf
    auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate_, settings_.highShelfFreq, 0.707f,
        juce::Decibels::decibelsToGain(settings_.highShelfGain)
    );
    
    for (int ch = 0; ch < 2; ++ch)
    {
        *lowShelf_[ch].coefficients = *lowShelfCoeffs;
        *lowMid_[ch].coefficients = *lowMidCoeffs;
        *highMid_[ch].coefficients = *highMidCoeffs;
        *highShelf_[ch].coefficients = *highShelfCoeffs;
    }
}

void ChannelStrip::process(juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    // Measure input
    inputLevel_ = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        inputLevel_ += buffer.getRMSLevel(ch, 0, numSamples);
    }
    inputLevel_ /= numChannels;
    
    for (int ch = 0; ch < numChannels && ch < 2; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = data[i];
            
            // Gate
            if (settings_.gateEnabled)
            {
                sample = processGate(sample);
            }
            
            // EQ
            if (settings_.eqEnabled)
            {
                sample = lowShelf_[ch].processSample(sample);
                sample = lowMid_[ch].processSample(sample);
                sample = highMid_[ch].processSample(sample);
                sample = highShelf_[ch].processSample(sample);
            }
            
            // Compressor
            if (settings_.compEnabled)
            {
                sample = processCompressor(sample);
            }
            
            // Output gain
            sample *= juce::Decibels::decibelsToGain(settings_.outputGain);
            
            // Phase invert
            if (settings_.phase)
                sample = -sample;
            
            data[i] = sample;
        }
    }
    
    // Measure output
    outputLevel_ = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        outputLevel_ += buffer.getRMSLevel(ch, 0, numSamples);
    }
    outputLevel_ /= numChannels;
}

void ChannelStrip::processStereo(float* left, float* right, int numSamples)
{
    juce::AudioBuffer<float> buffer(2, numSamples);
    buffer.copyFrom(0, 0, left, numSamples);
    buffer.copyFrom(1, 0, right, numSamples);
    
    process(buffer);
    
    // Copy back
    std::memcpy(left, buffer.getReadPointer(0), numSamples * sizeof(float));
    std::memcpy(right, buffer.getReadPointer(1), numSamples * sizeof(float));
}

float ChannelStrip::processGate(float input)
{
    float absInput = std::abs(input);
    float thresholdLin = juce::Decibels::decibelsToGain(settings_.gateThreshold);
    
    // Calculate target gain
    float targetGain = (absInput > thresholdLin) ? 1.0f : (1.0f / settings_.gateRatio);
    
    // Envelope follower
    float attackCoeff = std::exp(-1.0f / (settings_.gateAttack * 0.001f * sampleRate_));
    float releaseCoeff = std::exp(-1.0f / (settings_.gateRelease * 0.001f * sampleRate_));
    
    if (targetGain > gateEnvelope_)
        gateEnvelope_ = attackCoeff * gateEnvelope_ + (1.0f - attackCoeff) * targetGain;
    else
        gateEnvelope_ = releaseCoeff * gateEnvelope_ + (1.0f - releaseCoeff) * targetGain;
    
    return input * gateEnvelope_;
}

float ChannelStrip::processCompressor(float input)
{
    float absInput = std::abs(input);
    float thresholdLin = juce::Decibels::decibelsToGain(settings_.compThreshold);
    
    float gain = 1.0f;
    
    if (absInput > thresholdLin)
    {
        // Calculate gain reduction
        float inputDB = juce::Decibels::gainToDecibels(absInput);
        float thresholdDB = settings_.compThreshold;
        float over = inputDB - thresholdDB;
        
        // Soft knee
        if (over < settings_.compKnee)
        {
            over = over * over / (2.0f * settings_.compKnee);
        }
        else
        {
            over = over - settings_.compKnee / 2.0f;
        }
        
        float gainReductionDB = -over * (1.0f - 1.0f / settings_.compRatio);
        gain = juce::Decibels::decibelsToGain(gainReductionDB);
    }
    
    // Envelope follower
    float attackCoeff = std::exp(-1.0f / (settings_.compAttack * 0.001f * sampleRate_));
    float releaseCoeff = std::exp(-1.0f / (settings_.compRelease * 0.001f * sampleRate_));
    
    if (gain < compEnvelope_)
        compEnvelope_ = attackCoeff * compEnvelope_ + (1.0f - attackCoeff) * gain;
    else
        compEnvelope_ = releaseCoeff * compEnvelope_ + (1.0f - releaseCoeff) * gain;
    
    gainReduction_ = juce::Decibels::gainToDecibels(compEnvelope_);
    
    // Apply makeup gain
    float makeup = juce::Decibels::decibelsToGain(settings_.compMakeup);
    
    return input * compEnvelope_ * makeup;
}

void ChannelStrip::reset()
{
    gateEnvelope_ = 0.0f;
    compEnvelope_ = 1.0f;
    gainReduction_ = 0.0f;
    
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf_[ch].reset();
        lowMid_[ch].reset();
        highMid_[ch].reset();
        highShelf_[ch].reset();
    }
}

//==============================================================================
// VCASystem Implementation
//==============================================================================

VCASystem::VCASystem()
    : nextVcaId_(1)
{
}

VCASystem::~VCASystem()
{
}

int VCASystem::createVCA(const juce::String& name)
{
    int id = nextVcaId_++;
    
    VCAGroup vca;
    vca.name = name;
    vcas_[id] = vca;
    
    return id;
}

void VCASystem::deleteVCA(int vcaId)
{
    vcas_.erase(vcaId);
    spillStates_.erase(vcaId);
}

VCASystem::VCAGroup* VCASystem::getVCA(int vcaId)
{
    auto it = vcas_.find(vcaId);
    if (it != vcas_.end())
        return &it->second;
    return nullptr;
}

void VCASystem::assignChannelToVCA(int channelId, int vcaId)
{
    auto* vca = getVCA(vcaId);
    if (vca)
    {
        if (std::find(vca->memberChannels.begin(), vca->memberChannels.end(), channelId) == vca->memberChannels.end())
        {
            vca->memberChannels.push_back(channelId);
        }
    }
}

void VCASystem::removeChannelFromVCA(int channelId, int vcaId)
{
    auto* vca = getVCA(vcaId);
    if (vca)
    {
        vca->memberChannels.erase(
            std::remove(vca->memberChannels.begin(), vca->memberChannels.end(), channelId),
            vca->memberChannels.end()
        );
    }
}

bool VCASystem::isChannelInVCA(int channelId, int vcaId) const
{
    auto it = vcas_.find(vcaId);
    if (it != vcas_.end())
    {
        const auto& members = it->second.memberChannels;
        return std::find(members.begin(), members.end(), channelId) != members.end();
    }
    return false;
}

std::vector<int> VCASystem::getVCAsForChannel(int channelId) const
{
    std::vector<int> result;
    for (const auto& [vcaId, vca] : vcas_)
    {
        if (std::find(vca.memberChannels.begin(), vca.memberChannels.end(), channelId) != vca.memberChannels.end())
        {
            result.push_back(vcaId);
        }
    }
    return result;
}

void VCASystem::setVCAGain(int vcaId, float gainDB)
{
    auto* vca = getVCA(vcaId);
    if (vca)
    {
        vca->gain = juce::Decibels::decibelsToGain(gainDB);
    }
}

void VCASystem::setVCAMute(int vcaId, bool mute)
{
    auto* vca = getVCA(vcaId);
    if (vca)
    {
        vca->mute = mute;
    }
}

void VCASystem::setVCASolo(int vcaId, bool solo)
{
    auto* vca = getVCA(vcaId);
    if (vca)
    {
        vca->solo = solo;
    }
}

float VCASystem::getChannelMultiplier(int channelId) const
{
    float multiplier = 1.0f;
    
    // Multiply all VCA gains that this channel belongs to
    for (const auto& [vcaId, vca] : vcas_)
    {
        if (std::find(vca.memberChannels.begin(), vca.memberChannels.end(), channelId) != vca.memberChannels.end())
        {
            multiplier *= vca.gain;
        }
    }
    
    return multiplier;
}

bool VCASystem::isChannelMuted(int channelId) const
{
    // Check if any VCA this channel belongs to is muted
    for (const auto& [vcaId, vca] : vcas_)
    {
        if (std::find(vca.memberChannels.begin(), vca.memberChannels.end(), channelId) != vca.memberChannels.end())
        {
            if (vca.mute)
                return true;
        }
    }
    
    // Check solo logic
    bool anySolo = false;
    bool channelIsSoloed = false;
    
    for (const auto& [vcaId, vca] : vcas_)
    {
        if (vca.solo)
        {
            anySolo = true;
            if (std::find(vca.memberChannels.begin(), vca.memberChannels.end(), channelId) != vca.memberChannels.end())
            {
                channelIsSoloed = true;
            }
        }
    }
    
    if (anySolo && !channelIsSoloed)
        return true;
    
    return false;
}

void VCASystem::setSpillMode(int vcaId, bool enabled)
{
    spillStates_[vcaId] = enabled;
}

bool VCASystem::isSpillActive(int vcaId) const
{
    auto it = spillStates_.find(vcaId);
    return it != spillStates_.end() && it->second;
}

//==============================================================================
// MonitorSection Implementation
//==============================================================================

MonitorSection::MonitorSection()
    : sampleRate_(44100.0)
    , dimEnabled_(false)
    , dimLevel_(0.5f)
    , monoEnabled_(false)
    , muteEnabled_(false)
    , useB_(false)
{
}

MonitorSection::~MonitorSection()
{
}

void MonitorSection::initialize(double sampleRate)
{
    sampleRate_ = sampleRate;
}

void MonitorSection::process(juce::AudioBuffer<float>& buffer)
{
    if (muteEnabled_)
    {
        buffer.clear();
        return;
    }
    
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    // Mono sum
    if (monoEnabled_ && numChannels >= 2)
    {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float mono = (left[i] + right[i]) * 0.5f;
            left[i] = mono;
            right[i] = mono;
        }
    }
    
    // Dim
    if (dimEnabled_)
    {
        buffer.applyGain(dimLevel_);
    }
}

void MonitorSection::setDim(bool enabled)
{
    dimEnabled_ = enabled;
}

void MonitorSection::setDimLevel(float dB)
{
    dimLevel_ = juce::Decibels::decibelsToGain(juce::jlimit(-20.0f, 0.0f, dB));
}

void MonitorSection::setMono(bool enabled)
{
    monoEnabled_ = enabled;
}

void MonitorSection::setMute(bool enabled)
{
    muteEnabled_ = enabled;
}

void MonitorSection::setABMode(bool useB)
{
    useB_ = useB;
}

void MonitorSection::captureA(const juce::AudioBuffer<float>& reference)
{
    referenceA_.makeCopyOf(reference);
}

void MonitorSection::captureB(const juce::AudioBuffer<float>& reference)
{
    referenceB_.makeCopyOf(reference);
}

void MonitorSection::setCueMix(int sourceId, float level)
{
    cueLevels_[sourceId] = juce::jlimit(0.0f, 1.0f, level);
}

} // namespace omega
