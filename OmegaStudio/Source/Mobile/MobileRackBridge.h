#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace Mobile {

struct MobileRackSlot {
    juce::String pluginId;
    float dryWet = 1.0f;
};

class MobileRackBridge {
public:
    void addSlot(const MobileRackSlot& slot) { slots.add(slot); }
    void clear() { slots.clear(); }
    int getNumSlots() const { return slots.size(); }

    // Stub: in real impl, bridge AUv3/IPC
    void processBlock(juce::AudioBuffer<float>& buffer) {
        juce::ignoreUnused(buffer);
    }

private:
    juce::Array<MobileRackSlot> slots;
};

} // namespace Mobile
} // namespace OmegaStudio
