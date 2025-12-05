//==============================================================================
// MainComponent.h
// Main UI component - Complete DAW workspace with all systems integrated
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "../Project/ProjectManager.h"
#include "../Audio/Plugins/PluginManager.h"
#include "../Sequencer/MIDI/MIDIEngine.h"
#include "../Mixer/MixerEngine.h"
#include "../Sequencer/Automation/AutomationSystem.h"
#include "../Audio/Instruments/Instruments.h"
#include "../Audio/AI/AdvancedAI.h"

namespace Omega {
namespace Audio { class AudioEngine; }

namespace GUI {

//==============================================================================
// MainComponent - Complete DAW workspace with all systems
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
    
    //==========================================================================
    // System access
    //==========================================================================
    OmegaStudio::ProjectManager& getProjectManager() { return projectManager; }
    OmegaStudio::PluginManager& getPluginManager() { return pluginManager; }
    OmegaStudio::MIDIEngine& getMIDIEngine() { return midiEngine; }
    OmegaStudio::MixerEngine& getMixerEngine() { return mixerEngine; }
    OmegaStudio::AutomationManager& getAutomationManager() { return automationManager; }
    
private:
    //==========================================================================
    // Timer callback (for CPU meter updates, etc.)
    //==========================================================================
    void timerCallback() override;
    
    //==========================================================================
    // Data members
    //==========================================================================
    Audio::AudioEngine* audioEngine_;
    
    // All DAW Systems (initialized and ready)
    OmegaStudio::ProjectManager projectManager;
    OmegaStudio::PluginManager& pluginManager;  // Reference to singleton
    OmegaStudio::MIDIEngine midiEngine;
    OmegaStudio::MixerEngine mixerEngine;
    OmegaStudio::AutomationManager automationManager;
    
    // Instruments
    std::unique_ptr<OmegaStudio::ProSampler> sampler;
    std::unique_ptr<OmegaStudio::ProSynth> synth;
    std::unique_ptr<OmegaStudio::DrumMachine> drumMachine;
    
    // AI Features
    std::unique_ptr<OmegaStudio::StemSeparator> stemSeparator;
    std::unique_ptr<OmegaStudio::MasteringAssistant> masteringAssistant;
    std::unique_ptr<OmegaStudio::SmartEQ> smartEQ;
    std::unique_ptr<OmegaStudio::MixAnalyzer> mixAnalyzer;
    
    // UI State
    double cpuLoad_{0.0};
    juce::String deviceName_;
    double sampleRate_{0.0};
    int bufferSize_{0};
    
    // Status display
    int numTracks_{0};
    bool projectModified_{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace GUI
} // namespace Omega
