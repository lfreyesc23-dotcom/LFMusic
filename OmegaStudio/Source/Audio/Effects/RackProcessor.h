#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <atomic>

namespace omega {
namespace Audio {

enum class RackSlotType {
    Empty,
    InternalFX,
    VSTPlugin,
    AUPlugin,
    LV2Plugin
};

struct RackSlot {
    RackSlotType type = RackSlotType::Empty;
    juce::String pluginId;
    juce::String displayName;
    float dryWet = 1.0f;
    bool bypassed = false;
    bool solo = false;
    
    std::unique_ptr<juce::AudioProcessor> processor;
    juce::AudioBuffer<float> processBuffer;
};

class RackProcessor : public juce::AudioProcessor {
public:
    RackProcessor();
    ~RackProcessor() override;
    
    // Slot management
    int addSlot(RackSlotType type, const juce::String& pluginId);
    bool removeSlot(int index);
    void moveSlot(int fromIndex, int toIndex);
    void clearAllSlots();
    
    int getNumSlots() const { return static_cast<int>(slots_.size()); }
    RackSlot* getSlot(int index);
    const RackSlot* getSlot(int index) const;
    
    // Slot controls
    void setDryWet(int index, float amount);
    void setBypass(int index, bool bypass);
    void setSolo(int index, bool solo);
    
    // AudioProcessor implementation
    const juce::String getName() const override { return "RackProcessor"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    
private:
    std::vector<std::unique_ptr<RackSlot>> slots_;
    juce::CriticalSection slotLock_;
    double currentSampleRate_ = 44100.0;
    int currentBlockSize_ = 512;
    
    // Processing helpers
    void processSlot(RackSlot& slot, juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void applyDryWet(juce::AudioBuffer<float>& buffer, 
                    const juce::AudioBuffer<float>& dryBuffer, 
                    float wetAmount);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackProcessor)
};

} // namespace Audio
} // namespace omega
