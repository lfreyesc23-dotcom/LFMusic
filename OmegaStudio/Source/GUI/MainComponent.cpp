//==============================================================================
// MainComponent.cpp - Complete DAW integration
//==============================================================================

#include "MainComponent.h"
#include "../Audio/Engine/AudioEngine.h"
#include "../Utils/Constants.h"
#include "../Sequencer/ChannelRack.h"

namespace Omega::GUI {

//==============================================================================
MainComponent::MainComponent(Audio::AudioEngine* audioEngine)
    : audioEngine_(audioEngine),
      pluginManager(OmegaStudio::PluginManager::getInstance())
{
    // Apply FL Studio Look & Feel
    setLookAndFeel(&flLookAndFeel_);
    
    setSize(GUI::DEFAULT_WINDOW_WIDTH, GUI::DEFAULT_WINDOW_HEIGHT);
    
    // Initialize all DAW systems
    sampler = std::make_unique<OmegaStudio::ProSampler>();
    synth = std::make_unique<OmegaStudio::ProSynth>();
    drumMachine = std::make_unique<OmegaStudio::DrumMachine>();
    
    stemSeparator = std::make_unique<OmegaStudio::StemSeparator>();
    masteringAssistant = std::make_unique<OmegaStudio::MasteringAssistant>();
    smartEQ = std::make_unique<OmegaStudio::SmartEQ>();
    mixAnalyzer = std::make_unique<OmegaStudio::MixAnalyzer>();
    
    // Create Channel Rack Engine
    channelRackEngine_ = std::make_unique<OmegaStudio::Sequencer::ChannelRackEngine>();
    
    // Create FL Studio-style Channel Rack UI
    channelRackUI_ = std::make_unique<ChannelRackUI>(*channelRackEngine_);
    // NO añadirlo aún, lo hacemos al final después de los paneles
    
    // Create Record Toolbar (NEW - Panel superior de grabación)
    recordToolbar_ = std::make_unique<OmegaStudio::GUI::RecordToolbar>();
    addAndMakeVisible(recordToolbar_.get());
    DBG("✓ RecordToolbar created and added");
    
    recordToolbar_->onRecordClicked = [](bool recording) {
        DBG("Recording: " << recording);
    };
    
    recordToolbar_->onPlayClicked = [](bool playing) {
        DBG("Playing: " << playing);
    };
    
    recordToolbar_->onStopClicked = []() {
        DBG("Stopped");
    };
    
    recordToolbar_->onTempoChanged = [](double bpm) {
        DBG("Tempo changed to: " << bpm);
    };
    
    // Create Library Browser Panel (NEW - Panel lateral de biblioteca)
    libraryPanel_ = std::make_unique<OmegaStudio::GUI::LibraryBrowserPanel>();
    addAndMakeVisible(libraryPanel_.get());
    DBG("✓ LibraryBrowserPanel created and added");
    
    libraryPanel_->onFileDropped = [](const juce::File& file) {
        DBG("File dropped: " << file.getFullPathName());
        // TODO: Cargar sample al channel rack
    };
    
    libraryPanel_->onSampleSelected = [](const juce::String& path) {
        DBG("Sample selected: " << path);
    };
    
    // Create Mixer Channels Panel (NEW - Canales de mixer)
    mixerPanel_ = std::make_unique<OmegaStudio::GUI::MixerChannelsPanel>(8);
    addAndMakeVisible(mixerPanel_.get());
    DBG("✓ MixerChannelsPanel created and added with 8 channels");
    
    // Create TransportBar
    transportBar = std::make_unique<OmegaStudio::GUI::TransportBar>();
    addAndMakeVisible(transportBar.get());
    
    // IMPORTANTE: Añadir ChannelRack AL FINAL para que no tape los paneles
    addAndMakeVisible(*channelRackUI_);
    DBG("✓ ChannelRackUI added (after panels for correct z-order)");
    
    transportBar->onPlayStateChanged = [](bool /*playing*/) {
        // Start/stop playback
    };
    
    transportBar->onRecordStateChanged = [](bool /*recording*/) {
        // Toggle recording
    };
    
    setWantsKeyboardFocus(true);
    
    // Setup project change callback
    projectManager.onProjectChanged = [this]() {
        projectModified_ = true;
        numTracks_ = (int)projectManager.getProjectData().tracks.size();
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
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
    // FL Studio-style dark background
    g.fillAll(FLColors::DarkBg);
    
    // Top menu bar background
    auto menuBounds = getLocalBounds().removeFromTop(30);
    g.setColour(FLColors::PanelBg);
    g.fillRect(menuBounds);
    
    // Logo/Title
    g.setColour(FLColors::Orange);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.drawText("FRUTILLA STUDIO", menuBounds.reduced(10, 0),
               juce::Justification::centredLeft, false);
    
    // CPU Meter en la barra superior
    if (audioEngine_) {
        const auto cpuPercent = cpuLoad_ * 100.0;
        
        g.setColour(FLColors::TextSecondary);
        g.setFont(12.0f);
        auto cpuText = juce::String::formatted("CPU: %.1f%%", cpuPercent);
        g.drawText(cpuText, menuBounds.reduced(10, 0),
                  juce::Justification::centredRight, false);
    }
}

//==============================================================================
void MainComponent::resized() {
    auto bounds = getLocalBounds();
    DBG("=== MainComponent::resized() called ===");
    DBG("Total bounds: " << bounds.toString());
    
    // Menu bar en la parte superior (30px)
    bounds.removeFromTop(30);
    
    // Record Toolbar (panel superior de grabación) - 75px
    if (recordToolbar_) {
        auto toolbarBounds = bounds.removeFromTop(75);
        recordToolbar_->setBounds(toolbarBounds);
        DBG("RecordToolbar bounds: " << toolbarBounds.toString());
    }
    
    // TransportBar at bottom (60px)
    auto transportBounds = bounds.removeFromBottom(60);
    if (transportBar) {
        transportBar->setBounds(transportBounds);
        DBG("TransportBar bounds: " << transportBounds.toString());
    }
    
    // Library Browser Panel (panel lateral izquierdo) - 250px
    juce::Rectangle<int> libraryBounds;
    if (libraryPanel_ && showBrowserPanel_) {
        libraryBounds = bounds.removeFromLeft(250);
        libraryPanel_->setBounds(libraryBounds);
        DBG("LibraryPanel bounds: " << libraryBounds.toString());
    }
    
    // Mixer Channels Panel (panel lateral derecho) - 300px o más
    juce::Rectangle<int> mixerBounds;
    if (mixerPanel_ && showMixerPanel_) {
        mixerBounds = bounds.removeFromRight(juce::jmin(bounds.getWidth() / 2, 640));
        mixerPanel_->setBounds(mixerBounds);
        DBG("MixerPanel bounds: " << mixerBounds.toString());
    }
    
    // Channel Rack ocupa el resto del espacio central
    if (channelRackUI_) {
        channelRackUI_->setBounds(bounds);
        DBG("ChannelRack bounds: " << bounds.toString());
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
