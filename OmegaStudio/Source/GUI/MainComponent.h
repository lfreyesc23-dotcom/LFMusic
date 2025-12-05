//==============================================================================
// MainComponent.h
// Main UI component (the workspace)
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace Omega {
namespace Audio { class AudioEngine; }

namespace GUI {

//==============================================================================
// MainComponent - The main UI workspace
//==============================================================================
class MainComponent : public juce::Component,
                      private juce::Timer {
public:
    explicit MainComponent(Audio::AudioEngine* audioEngine);
    ~MainComponent() override;
    
    //==========================================================================
    // Component overrides
    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    //==========================================================================
    // Timer callback (for CPU meter updates, etc.)
    //==========================================================================
    void timerCallback() override;
    
    //==========================================================================
    // Data members
    //==========================================================================
    Audio::AudioEngine* audioEngine_;
    
    // UI State
    double cpuLoad_{0.0};
    juce::String deviceName_;
    double sampleRate_{0.0};
    int bufferSize_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace GUI
} // namespace Omega
