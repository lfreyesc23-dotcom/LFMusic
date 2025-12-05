//==============================================================================
// MainComponent.cpp
//==============================================================================

#include "MainComponent.h"
#include "../Audio/Engine/AudioEngine.h"
#include "../Utils/Constants.h"

namespace Omega::GUI {

//==============================================================================
MainComponent::MainComponent(Audio::AudioEngine* audioEngine)
    : audioEngine_(audioEngine)
{
    setSize(GUI::DEFAULT_WINDOW_WIDTH, GUI::DEFAULT_WINDOW_HEIGHT);
    
    // Start timer for UI updates (60 FPS)
    startTimerHz(GUI::TARGET_FPS);
    
    if (audioEngine_) {
        deviceName_ = audioEngine_->getCurrentDeviceName();
        sampleRate_ = audioEngine_->getSampleRate();
        bufferSize_ = audioEngine_->getBufferSize();
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
    }
    
    // Footer
    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colour(0xff666666));
    g.drawText("Built with JUCE & C++23 | Lock-Free Architecture | SIMD Optimized",
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
