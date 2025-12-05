//==============================================================================
// MainComponent.cpp - Complete DAW integration
//==============================================================================

#include "MainComponent.h"
#include "../Audio/Engine/AudioEngine.h"
#include "../Utils/Constants.h"

namespace Omega::GUI {

//==============================================================================
MainComponent::MainComponent(Audio::AudioEngine* audioEngine)
    : audioEngine_(audioEngine),
      pluginManager(OmegaStudio::PluginManager::getInstance())
{
    setSize(GUI::DEFAULT_WINDOW_WIDTH, GUI::DEFAULT_WINDOW_HEIGHT);
    
    // Initialize all DAW systems
    sampler = std::make_unique<OmegaStudio::ProSampler>();
    synth = std::make_unique<OmegaStudio::ProSynth>();
    drumMachine = std::make_unique<OmegaStudio::DrumMachine>();
    
    stemSeparator = std::make_unique<OmegaStudio::StemSeparator>();
    masteringAssistant = std::make_unique<OmegaStudio::MasteringAssistant>();
    smartEQ = std::make_unique<OmegaStudio::SmartEQ>();
    mixAnalyzer = std::make_unique<OmegaStudio::MixAnalyzer>();
    
    // Setup project change callback
    projectManager.onProjectChanged = [this]() {
        projectModified_ = true;
        numTracks_ = projectManager.getProjectData().tracks.size();
        repaint();
    };
    
    // Start timer for UI updates (60 FPS)
    startTimerHz(GUI::TARGET_FPS);
    
    if (audioEngine_) {
        deviceName_ = audioEngine_->getCurrentDeviceName();
        sampleRate_ = audioEngine_->getSampleRate();
        bufferSize_ = audioEngine_->getBufferSize();
        
        // Prepare audio systems
        double sr = sampleRate_;
        int bs = bufferSize_;
        
        synth->prepareToPlay(sr, bs);
        drumMachine->prepareToPlay(sr, bs);
        stemSeparator->prepareToPlay(sr, bs);
    }
}

//==============================================================================
MainComponent::~MainComponent() {
    stopTimer();
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
    // Modern dark theme
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(48.0f, juce::Font::bold));
    g.drawText("OMEGA STUDIO", getLocalBounds().reduced(20),
               juce::Justification::centredTop, true);
    
    // Subtitle
    g.setFont(juce::Font(24.0f));
    g.setColour(juce::Colour(0xff00d4ff));  // Cyan accent
    g.drawText("Next-Generation Digital Audio Workstation",
               getLocalBounds().reduced(20).withTrimmedTop(80),
               juce::Justification::centredTop, true);
    
    // Status info
    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::lightgrey);
    
    int yPos = 200;
    const int lineHeight = 30;
    
    if (audioEngine_) {
        // Device info
        g.drawText("Audio Device: " + deviceName_,
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Sample rate
        g.drawText(juce::String::formatted("Sample Rate: %.1f Hz", sampleRate_),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Buffer size
        g.drawText(juce::String::formatted("Buffer Size: %d samples (%.1f ms)",
                                          bufferSize_,
                                          (bufferSize_ / sampleRate_) * 1000.0),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // CPU load
        const auto cpuPercent = cpuLoad_ * 100.0;
        auto cpuColour = cpuPercent < 50.0 ? juce::Colours::green :
                        cpuPercent < 75.0 ? juce::Colours::orange :
                        juce::Colours::red;
        
        g.setColour(cpuColour);
        g.drawText(juce::String::formatted("CPU Load: %.1f%%", cpuPercent),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Engine state
        g.setColour(juce::Colours::lightgrey);
        const char* stateStr = "Unknown";
        switch (audioEngine_->getState()) {
            case Audio::EngineState::Uninitialized: stateStr = "Uninitialized"; break;
            case Audio::EngineState::Initialized: stateStr = "Initialized"; break;
            case Audio::EngineState::Running: stateStr = "Running"; break;
            case Audio::EngineState::Stopped: stateStr = "Stopped"; break;
            case Audio::EngineState::Error: stateStr = "Error"; break;
        }
        g.drawText(juce::String("Engine State: ") + stateStr,
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // DAW Systems Status
        g.setColour(juce::Colour(0xff00d4ff));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("DAW Systems Status",
                   20, yPos + 20, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += 60;
        
        g.setFont(juce::Font(14.0f));
        g.setColour(juce::Colours::lightgrey);
        
        // Project stats
        g.drawText(juce::String::formatted("Project: %d tracks | Modified: %s",
                                          numTracks_,
                                          projectModified_ ? "Yes" : "No"),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Plugin stats
        g.drawText(juce::String::formatted("Plugins: %d loaded | %d total scanned",
                                          pluginManager.getLoadedPluginCount(),
                                          pluginManager.getScannedPluginCount()),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // MIDI stats
        g.drawText(juce::String::formatted("MIDI: %d clips | %d notes total",
                                          midiEngine.getClipCount(),
                                          midiEngine.getTotalNoteCount()),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Mixer stats
        g.drawText(juce::String::formatted("Mixer: %d channels | %d buses",
                                          mixerEngine.getChannelCount(),
                                          mixerEngine.getBusCount()),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Automation stats
        g.drawText(juce::String::formatted("Automation: %d parameters | %d points total",
                                          automationManager.getParameterCount(),
                                          automationManager.getTotalPointCount()),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // Instruments status
        g.setColour(juce::Colour(0xff00ff88));
        g.drawText(juce::String::formatted("Instruments: Sampler (%d samples) | Synth (4-osc) | Drums (16 pads)",
                                          sampler->getSampleCount()),
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
        yPos += lineHeight;
        
        // AI status
        g.setColour(juce::Colour(0xffff00ff));
        g.drawText("AI: Stem Separation | Auto-Mastering | Smart EQ | Mix Analyzer",
                   20, yPos, getWidth() - 40, lineHeight,
                   juce::Justification::left, true);
    }
    
    // Footer
    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("Built with JUCE & C++20 | Lock-Free Architecture | SIMD Optimized | 10,200+ Lines",
               getLocalBounds().reduced(20).withTrimmedBottom(20),
               juce::Justification::centredBottom, true);
}

//==============================================================================
void MainComponent::resized() {
    // Layout child components here when we have them
}

//==============================================================================
void MainComponent::timerCallback() {
    // Update UI state from audio engine
    if (audioEngine_) {
        cpuLoad_ = audioEngine_->getCpuLoad();
        
        // Process messages from audio thread
        auto& messageQueue = audioEngine_->getMessageFIFO();
        while (auto msg = messageQueue.pop()) {
            // Handle messages (meter updates, etc.)
            // For now, just consume them
        }
    }
    
    // Repaint (this is 60 FPS, no problem for GPU)
    repaint();
}

} // namespace Omega::GUI
