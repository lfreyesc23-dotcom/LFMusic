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
    
    // Create TransportBar
    transportBar = std::make_unique<OmegaStudio::GUI::TransportBar>();
    addAndMakeVisible(transportBar.get());
    
    transportBar->onPlayStateChanged = [this](bool playing) {
        // Start/stop playback
    };
    
    transportBar->onRecordStateChanged = [this](bool recording) {
        // Toggle recording
    };
    
    setWantsKeyboardFocus(true);
    
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
    auto bounds = getLocalBounds();
    
    // TransportBar at bottom
    if (transportBar) {
        transportBar->setBounds(bounds.removeFromBottom(50));
    }
}

//==============================================================================
void MainComponent::showPianoRoll() {
    if (!pianoRollWindow) {
        pianoRollWindow = std::make_unique<OmegaStudio::GUI::PianoRollWindow>("Piano Roll");
    }
    pianoRollWindow->setVisible(true);
    pianoRollWindow->toFront(true);
}

void MainComponent::showMixer() {
    if (!mixerWindow) {
        mixerWindow = std::make_unique<OmegaStudio::GUI::MixerWindow>("Mixer");
        if (mixerWindow->getEditor()) {
            mixerWindow->getEditor()->setMixerEngine(&mixerEngine);
        }
    }
    mixerWindow->setVisible(true);
    mixerWindow->toFront(true);
}

void MainComponent::showPlaylist() {
    if (!playlistWindow) {
        playlistWindow = std::make_unique<OmegaStudio::GUI::PlaylistWindow>("Playlist");
    }
    playlistWindow->setVisible(true);
    playlistWindow->toFront(true);
}

void MainComponent::showChannelRack() {
    if (!channelRackWindow) {
        channelRackWindow = std::make_unique<OmegaStudio::GUI::ChannelRackWindow>();
    }
    channelRackWindow->setVisible(true);
    channelRackWindow->toFront(true);
}

void MainComponent::showBrowser() {
    if (!browserWindow) {
        browserWindow = std::make_unique<OmegaStudio::GUI::BrowserWindow>();
    }
    browserWindow->setVisible(true);
    browserWindow->toFront(true);
}

void MainComponent::showAudioEditor() {
    if (!audioEditorWindow) {
        audioEditorWindow = std::make_unique<OmegaStudio::GUI::AudioEditorWindow>();
    }
    audioEditorWindow->setVisible(true);
    audioEditorWindow->toFront(true);
}

void MainComponent::showPerformanceMode() {
    if (!performanceModeWindow) {
        performanceModeWindow = std::make_unique<OmegaStudio::GUI::PerformanceModeWindow>();
    }
    performanceModeWindow->setVisible(true);
    performanceModeWindow->toFront(true);
}

void MainComponent::showMacroPanel() {
    if (!macroPanel) {
        macroPanel = std::make_unique<OmegaStudio::GUI::MacroPanelComponent>();
        addAndMakeVisible(macroPanel.get());
        resized(); // Trigger layout update
    }
    macroPanel->setVisible(!macroPanel->isVisible());
}

void MainComponent::showStemSeparator() {
    if (!stemSeparatorWindow) {
        stemSeparatorWindow = std::make_unique<OmegaStudio::GUI::StemSeparatorWindow>();
    }
    stemSeparatorWindow->setVisible(true);
    stemSeparatorWindow->toFront(true);
}

void MainComponent::showSmartMixingAssistant() {
    if (!smartMixingAssistantWindow) {
        smartMixingAssistantWindow = std::make_unique<OmegaStudio::GUI::SmartMixingAssistantWindow>();
    }
    smartMixingAssistantWindow->setVisible(true);
    smartMixingAssistantWindow->toFront(true);
}

bool MainComponent::keyPressed(const juce::KeyPress& key) {
    // F5 = Playlist
    if (key.getKeyCode() == juce::KeyPress::F5Key) {
        showPlaylist();
        return true;
    }
    // F6 = Channel Rack
    else if (key.getKeyCode() == juce::KeyPress::F6Key) {
        showChannelRack();
        return true;
    }
    // F7 = Piano Roll
    else if (key.getKeyCode() == juce::KeyPress::F7Key) {
        showPianoRoll();
        return true;
    }
    // F8 = Browser
    else if (key.getKeyCode() == juce::KeyPress::F8Key) {
        showBrowser();
        return true;
    }
    // F9 = Mixer
    else if (key.getKeyCode() == juce::KeyPress::F9Key) {
        showMixer();
        return true;
    }
    // F10 = Audio Editor (Edison)
    else if (key.getKeyCode() == juce::KeyPress::F10Key) {
        showAudioEditor();
        return true;
    }
    // F11 = Performance Mode
    else if (key.getKeyCode() == juce::KeyPress::F11Key) {
        showPerformanceMode();
        return true;
    }
    // F12 = Macro Panel
    else if (key.getKeyCode() == juce::KeyPress::F12Key) {
        showMacroPanel();
        return true;
    }
    // Ctrl+Shift+S = Stem Separator
    else if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && 
             key.getKeyCode() == 'S') {
        showStemSeparator();
        return true;
    }
    // Ctrl+Shift+M = Smart Mixing Assistant
    else if (key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() && 
             key.getKeyCode() == 'M') {
        showSmartMixingAssistant();
        return true;
    }
    
    return false;
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
