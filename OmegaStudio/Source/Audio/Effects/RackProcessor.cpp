#include "RackProcessor.h"

namespace omega {
namespace Audio {

RackProcessor::RackProcessor() {
}

RackProcessor::~RackProcessor() {
    clearAllSlots();
}

void RackProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate_ = sampleRate;
    currentBlockSize_ = samplesPerBlock;
    
    juce::ScopedLock lock(slotLock_);
    
    for (auto& slot : slots_) {
        if (slot && slot->processor) {
            slot->processor->setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
            slot->processor->prepareToPlay(sampleRate, samplesPerBlock);
        }
        
        // Allocate process buffer for dry/wet mixing
        slot->processBuffer.setSize(2, samplesPerBlock);
    }
}

void RackProcessor::releaseResources() {
    juce::ScopedLock lock(slotLock_);
    
    for (auto& slot : slots_) {
        if (slot && slot->processor) {
            slot->processor->releaseResources();
        }
    }
}

void RackProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedLock lock(slotLock_);
    
    if (slots_.empty()) {
        return;
    }
    
    // Check if any slot is soloed
    bool hasSolo = false;
    for (const auto& slot : slots_) {
        if (slot && slot->solo && !slot->bypassed) {
            hasSolo = true;
            break;
        }
    }
    
    // Process each slot in series
    for (auto& slot : slots_) {
        if (!slot || !slot->processor) continue;
        
        // Skip if bypassed or muted by solo
        if (slot->bypassed || (hasSolo && !slot->solo)) {
            continue;
        }
        
        processSlot(*slot, buffer, midiMessages);
    }
}

void RackProcessor::processSlot(RackSlot& slot, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    if (!slot.processor) return;
    
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    // Store dry signal for dry/wet mixing
    juce::AudioBuffer<float> dryBuffer;
    if (slot.dryWet < 1.0f) {
        dryBuffer.setSize(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch) {
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        }
    }
    
    // Process through plugin
    slot.processor->processBlock(buffer, midi);
    
    // Apply dry/wet mix
    if (slot.dryWet < 1.0f) {
        applyDryWet(buffer, dryBuffer, slot.dryWet);
    }
}

void RackProcessor::applyDryWet(juce::AudioBuffer<float>& buffer,
                               const juce::AudioBuffer<float>& dryBuffer,
                               float wetAmount) {
    const float dryAmount = 1.0f - wetAmount;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* wet = buffer.getWritePointer(ch);
        const float* dry = dryBuffer.getReadPointer(ch);
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            wet[i] = dry[i] * dryAmount + wet[i] * wetAmount;
        }
    }
}

int RackProcessor::addSlot(RackSlotType type, const juce::String& pluginId) {
    juce::ScopedLock lock(slotLock_);
    
    auto slot = std::make_unique<RackSlot>();
    slot->type = type;
    slot->pluginId = pluginId;
    slot->displayName = pluginId;
    slot->dryWet = 1.0f;
    slot->bypassed = false;
    slot->solo = false;
    
    // TODO: Load actual plugin based on type and pluginId
    // For now, create a null processor
    slot->processor = nullptr;
    
    int index = static_cast<int>(slots_.size());
    slots_.push_back(std::move(slot));
    
    return index;
}

bool RackProcessor::removeSlot(int index) {
    juce::ScopedLock lock(slotLock_);
    
    if (index < 0 || index >= static_cast<int>(slots_.size()))
        return false;
    
    slots_.erase(slots_.begin() + index);
    return true;
}

void RackProcessor::moveSlot(int fromIndex, int toIndex) {
    juce::ScopedLock lock(slotLock_);
    
    if (fromIndex < 0 || fromIndex >= static_cast<int>(slots_.size()))
        return;
    if (toIndex < 0 || toIndex >= static_cast<int>(slots_.size()))
        return;
    if (fromIndex == toIndex)
        return;
    
    auto slot = std::move(slots_[fromIndex]);
    slots_.erase(slots_.begin() + fromIndex);
    slots_.insert(slots_.begin() + toIndex, std::move(slot));
}

void RackProcessor::clearAllSlots() {
    juce::ScopedLock lock(slotLock_);
    
    for (auto& slot : slots_) {
        if (slot && slot->processor) {
            slot->processor->releaseResources();
        }
    }
    
    slots_.clear();
}

RackSlot* RackProcessor::getSlot(int index) {
    juce::ScopedLock lock(slotLock_);
    
    if (index < 0 || index >= static_cast<int>(slots_.size()))
        return nullptr;
    
    return slots_[index].get();
}

const RackSlot* RackProcessor::getSlot(int index) const {
    juce::ScopedLock lock(slotLock_);
    
    if (index < 0 || index >= static_cast<int>(slots_.size()))
        return nullptr;
    
    return slots_[index].get();
}

void RackProcessor::setDryWet(int index, float amount) {
    juce::ScopedLock lock(slotLock_);
    
    if (auto* slot = getSlot(index)) {
        slot->dryWet = juce::jlimit(0.0f, 1.0f, amount);
    }
}

void RackProcessor::setBypass(int index, bool bypass) {
    juce::ScopedLock lock(slotLock_);
    
    if (auto* slot = getSlot(index)) {
        slot->bypassed = bypass;
    }
}

void RackProcessor::setSolo(int index, bool solo) {
    juce::ScopedLock lock(slotLock_);
    
    if (auto* slot = getSlot(index)) {
        slot->solo = solo;
    }
}

} // namespace Audio
} // namespace omega
