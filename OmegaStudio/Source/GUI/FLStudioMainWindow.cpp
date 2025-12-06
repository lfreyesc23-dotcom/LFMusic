//==============================================================================
// FLStudioMainWindow.cpp
// Implementación completa de la interfaz FL Studio
//==============================================================================

#include "FLStudioMainWindow.h"

namespace Omega {
namespace GUI {

//==============================================================================
// FLTopToolbar Implementation
//==============================================================================
FLTopToolbar::FLTopToolbar() {
    // New button
    newButton_ = std::make_unique<juce::TextButton>("New");
    newButton_->onClick = [this]() { if (onNewProject) onNewProject(); };
    addAndMakeVisible(*newButton_);
    
    // Open button
    openButton_ = std::make_unique<juce::TextButton>("Open");
    openButton_->onClick = [this]() { if (onOpenProject) onOpenProject(); };
    addAndMakeVisible(*openButton_);
    
    // Save button
    saveButton_ = std::make_unique<juce::TextButton>("Save");
    saveButton_->onClick = [this]() { if (onSaveProject) onSaveProject(); };
    addAndMakeVisible(*saveButton_);
    
    // Export button
    exportButton_ = std::make_unique<juce::TextButton>("Export");
    exportButton_->onClick = [this]() { if (onExportAudio) onExportAudio(); };
    addAndMakeVisible(*exportButton_);
    
    // CPU label
    cpuLabel_ = std::make_unique<juce::Label>();
    cpuLabel_->setText("CPU: 0%", juce::dontSendNotification);
    addAndMakeVisible(*cpuLabel_);
}

void FLTopToolbar::paint(juce::Graphics& g) {
    g.fillAll(FLColors::PanelBg);
    
    // Logo
    g.setColour(FLColors::Orange);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("🍓 FRUTILLA STUDIO", 10, 0, 200, getHeight(),
              juce::Justification::centredLeft, false);
}

void FLTopToolbar::resized() {
    auto bounds = getLocalBounds().reduced(5);
    bounds.removeFromLeft(200); // Logo space
    
    newButton_->setBounds(bounds.removeFromLeft(60).reduced(2));
    bounds.removeFromLeft(5);
    openButton_->setBounds(bounds.removeFromLeft(60).reduced(2));
    bounds.removeFromLeft(5);
    saveButton_->setBounds(bounds.removeFromLeft(60).reduced(2));
    bounds.removeFromLeft(5);
    exportButton_->setBounds(bounds.removeFromLeft(70).reduced(2));
    
    // CPU label on the right
    cpuLabel_->setBounds(bounds.removeFromRight(100).reduced(2));
}

//==============================================================================
// FLSidePanel Implementation
//==============================================================================
FLSidePanel::FLSidePanel() {
    const struct { const char* name; const char* icon; View view; } views[] = {
        {"🎹 Channel Rack", "🎹", View::ChannelRack},
        {"📊 Playlist", "📊", View::Playlist},
        {"🎵 Piano Roll", "🎵", View::PianoRoll},
        {"🎚️ Mixer", "🎚️", View::Mixer},
        {"📁 Browser", "📁", View::Browser},
        {"🎙️ Recording", "🎙️", View::Recording}
    };
    
    for (const auto& v : views) {
        auto* button = new ViewButton(v.name, v.view);
        button->onClick = [this, view = v.view, button]() {
            currentView_ = view;
            if (onViewSelected) onViewSelected(view);
            
            // Update button states
            for (auto* btn : viewButtons_)
                btn->setToggleState(false, juce::dontSendNotification);
            button->setToggleState(true, juce::dontSendNotification);
        };
        
        viewButtons_.add(button);
        addAndMakeVisible(button);
    }
    
    // Select first view by default
    if (!viewButtons_.isEmpty())
        viewButtons_[0]->setToggleState(true, juce::dontSendNotification);
}

void FLSidePanel::paint(juce::Graphics& g) {
    g.fillAll(FLColors::DarkBg);
    
    // Border
    g.setColour(FLColors::Border);
    g.drawLine(getWidth() - 1, 0, getWidth() - 1, getHeight(), 1.0f);
}

void FLSidePanel::resized() {
    auto bounds = getLocalBounds().reduced(5);
    const int buttonHeight = 45;
    const int spacing = 5;
    
    for (auto* button : viewButtons_) {
        button->setBounds(bounds.removeFromTop(buttonHeight));
        bounds.removeFromTop(spacing);
    }
}

//==============================================================================
// FLTransportBar Implementation
//==============================================================================
FLTransportBar::FLTransportBar() {
    // Play button
    playButton_ = std::make_unique<juce::TextButton>("▶");
    playButton_->setTooltip("Play/Pause (Space)");
    playButton_->onClick = [this]() {
        isPlaying_ = !isPlaying_;
        if (onPlayPause) onPlayPause(isPlaying_);
        playButton_->setButtonText(isPlaying_ ? "⏸" : "▶");
    };
    addAndMakeVisible(*playButton_);
    
    // Stop button
    stopButton_ = std::make_unique<juce::TextButton>("⏹");
    stopButton_->setTooltip("Stop");
    stopButton_->onClick = [this]() {
        isPlaying_ = false;
        playButton_->setButtonText("▶");
        position_ = 0.0;
        if (onStop) onStop();
    };
    addAndMakeVisible(*stopButton_);
    
    // Record button
    recordButton_ = std::make_unique<juce::TextButton>("⏺");
    recordButton_->setTooltip("Record");
    recordButton_->onClick = [this]() {
        isRecording_ = !isRecording_;
        if (onRecord) onRecord(isRecording_);
    };
    addAndMakeVisible(*recordButton_);
    
    // Loop button
    loopButton_ = std::make_unique<juce::TextButton>("🔁");
    loopButton_->setTooltip("Loop");
    loopButton_->setToggleState(true, juce::dontSendNotification);
    loopButton_->onClick = [this]() {
        isLooping_ = !isLooping_;
        loopButton_->setToggleState(isLooping_, juce::dontSendNotification);
        if (onLoop) onLoop(isLooping_);
    };
    addAndMakeVisible(*loopButton_);
    
    // Tempo slider
    tempoSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                    juce::Slider::TextBoxRight);
    tempoSlider_->setRange(60.0, 200.0, 0.1);
    tempoSlider_->setValue(140.0);
    tempoSlider_->setTooltip("Tempo (BPM)");
    tempoSlider_->onValueChange = [this]() {
        tempo_ = tempoSlider_->getValue();
        if (onTempoChanged) onTempoChanged(tempo_);
    };
    addAndMakeVisible(*tempoSlider_);
    
    // Tempo label
    tempoLabel_ = std::make_unique<juce::Label>();
    tempoLabel_->setText("BPM:", juce::dontSendNotification);
    addAndMakeVisible(*tempoLabel_);
    
    // Position label
    positionLabel_ = std::make_unique<juce::Label>();
    positionLabel_->setText("0:0:0", juce::dontSendNotification);
    addAndMakeVisible(*positionLabel_);
    
    startTimerHz(30);
}

void FLTransportBar::paint(juce::Graphics& g) {
    g.fillAll(FLColors::MediumBg);
    
    // Top border
    g.setColour(FLColors::Border);
    g.drawLine(0, 0, getWidth(), 0, 1.0f);
}

void FLTransportBar::resized() {
    auto bounds = getLocalBounds().reduced(5);
    
    playButton_->setBounds(bounds.removeFromLeft(50).reduced(2));
    bounds.removeFromLeft(5);
    stopButton_->setBounds(bounds.removeFromLeft(50).reduced(2));
    bounds.removeFromLeft(5);
    recordButton_->setBounds(bounds.removeFromLeft(50).reduced(2));
    bounds.removeFromLeft(5);
    loopButton_->setBounds(bounds.removeFromLeft(50).reduced(2));
    bounds.removeFromLeft(20);
    
    tempoLabel_->setBounds(bounds.removeFromLeft(50).reduced(2));
    tempoSlider_->setBounds(bounds.removeFromLeft(150).reduced(2));
    bounds.removeFromLeft(20);
    
    positionLabel_->setBounds(bounds.removeFromLeft(100).reduced(2));
}

void FLTransportBar::timerCallback() {
    if (isPlaying_) {
        position_ += 0.1; // Dummy increment
        int bars = (int)(position_ / 4.0);
        int beats = (int)position_ % 4;
        int ticks = (int)((position_ - (int)position_) * 100.0);
        positionLabel_->setText(juce::String::formatted("%d:%d:%02d", bars, beats, ticks),
                               juce::dontSendNotification);
    }
}

void FLTransportBar::setPlaying(bool playing) { isPlaying_ = playing; }
void FLTransportBar::setRecording(bool recording) { isRecording_ = recording; }
void FLTransportBar::setLooping(bool looping) { isLooping_ = looping; }
void FLTransportBar::setTempo(double bpm) { tempo_ = bpm; tempoSlider_->setValue(bpm); }
void FLTransportBar::setPosition(double beats) { position_ = beats; }

//==============================================================================
// FLRecordingPanel Implementation
//==============================================================================
FLRecordingPanel::FLRecordingPanel() {
    // Record button
    recordButton_ = std::make_unique<juce::TextButton>("● Record");
    recordButton_->onClick = [this]() {
        isRecording_ = !isRecording_;
        if (isRecording_) {
            recordButton_->setButtonText("⏸ Pause");
            if (onStartRecording) onStartRecording();
        } else {
            recordButton_->setButtonText("● Record");
            if (onStopRecording) onStopRecording();
        }
    };
    addAndMakeVisible(*recordButton_);
    
    // Play button
    playButton_ = std::make_unique<juce::TextButton>("▶ Play");
    playButton_->onClick = [this]() {
        if (onPlayRecording) onPlayRecording();
    };
    addAndMakeVisible(*playButton_);
    
    // Stop button
    stopButton_ = std::make_unique<juce::TextButton>("■ Stop");
    stopButton_->onClick = [this]() {
        isRecording_ = false;
        recordButton_->setButtonText("● Record");
        if (onStopRecording) onStopRecording();
    };
    addAndMakeVisible(*stopButton_);
    
    // Input selector
    inputSelector_ = std::make_unique<juce::ComboBox>();
    inputSelector_->addItem("Default Input", 1);
    inputSelector_->setSelectedId(1);
    addAndMakeVisible(*inputSelector_);
    
    // Level meter
    levelMeter_ = std::make_unique<juce::Label>();
    addAndMakeVisible(*levelMeter_);
    
    // Time label
    timeLabel_ = std::make_unique<juce::Label>();
    timeLabel_->setText("0:00", juce::dontSendNotification);
    addAndMakeVisible(*timeLabel_);
    
    startTimerHz(30);
}

void FLRecordingPanel::paint(juce::Graphics& g) {
    g.fillAll(FLColors::DarkBg);
    
    // Title
    g.setColour(FLColors::Orange);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("🎙️ Recording Studio", 20, 20, getWidth() - 40, 40,
              juce::Justification::centredLeft, false);
    
    // Waveform area
    auto waveformBounds = getLocalBounds().reduced(20).withTrimmedTop(200).withTrimmedBottom(150);
    g.setColour(FLColors::MediumBg);
    g.fillRoundedRectangle(waveformBounds.toFloat(), 8.0f);
    
    g.setColour(FLColors::Border);
    g.drawRoundedRectangle(waveformBounds.toFloat(), 8.0f, 1.0f);
    
    // Level meter visualization
    if (isRecording_) {
        g.setColour(FLColors::Danger);
        auto meterBounds = waveformBounds.reduced(10);
        float level = inputLevel_ * meterBounds.getHeight();
        g.fillRect(meterBounds.withHeight((int)level).withBottomY(meterBounds.getBottom()));
    }
}

void FLRecordingPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(80); // Title space
    
    // Controls
    auto controlBounds = bounds.removeFromTop(100);
    recordButton_->setBounds(controlBounds.removeFromLeft(120).reduced(5));
    playButton_->setBounds(controlBounds.removeFromLeft(100).reduced(5));
    stopButton_->setBounds(controlBounds.removeFromLeft(100).reduced(5));
    
    bounds.removeFromTop(20);
    
    // Input selector
    auto inputBounds = bounds.removeFromTop(60);
    auto label = inputBounds.removeFromLeft(120);
    inputSelector_->setBounds(inputBounds.removeFromLeft(200).reduced(5));
    
    // Time display at bottom
    auto bottomBounds = getLocalBounds().reduced(20).removeFromBottom(80);
    timeLabel_->setBounds(bottomBounds);
    timeLabel_->setJustificationType(juce::Justification::centred);
}

void FLRecordingPanel::timerCallback() {
    if (isRecording_) {
        recordTime_ += 1.0/30.0;
        int minutes = (int)(recordTime_ / 60.0);
        int seconds = (int)recordTime_ % 60;
        timeLabel_->setText(juce::String::formatted("%d:%02d", minutes, seconds),
                           juce::dontSendNotification);
        
        // Simulate input level
        inputLevel_ = juce::Random::getSystemRandom().nextFloat() * 0.8f;
        repaint();
    }
}

//==============================================================================
// FLStudioMainWindow Implementation
//==============================================================================
FLStudioMainWindow::FLStudioMainWindow() {
    setLookAndFeel(&flLookAndFeel_);
    
    // Create top toolbar
    topToolbar_ = std::make_unique<FLTopToolbar>();
    addAndMakeVisible(*topToolbar_);
    
    // Create side panel
    sidePanel_ = std::make_unique<FLSidePanel>();
    sidePanel_->onViewSelected = [this](FLSidePanel::View view) {
        switch (view) {
            case FLSidePanel::View::ChannelRack: showChannelRack(); break;
            case FLSidePanel::View::Recording: showRecording(); break;
            case FLSidePanel::View::Browser: showBrowser(); break;
            default: break;
        }
    };
    addAndMakeVisible(*sidePanel_);
    
    // Create transport bar
    transportBar_ = std::make_unique<FLTransportBar>();
    addAndMakeVisible(*transportBar_);
    
    // Create Channel Rack
    channelRackEngine_ = std::make_unique<OmegaStudio::Sequencer::ChannelRackEngine>();
    channelRackUI_ = std::make_unique<ChannelRackUI>(*channelRackEngine_);
    
    // Create Recording Panel
    recordingPanel_ = std::make_unique<FLRecordingPanel>();
    
    // Show Channel Rack by default
    showChannelRack();
    
    setSize(1400, 900);
}

FLStudioMainWindow::~FLStudioMainWindow() {
    setLookAndFeel(nullptr);
}

void FLStudioMainWindow::paint(juce::Graphics& g) {
    g.fillAll(FLColors::DarkBg);
}

void FLStudioMainWindow::resized() {
    auto bounds = getLocalBounds();
    
    // Top toolbar
    topToolbar_->setBounds(bounds.removeFromTop(40));
    
    // Transport bar at bottom
    transportBar_->setBounds(bounds.removeFromBottom(60));
    
    // Side panel
    sidePanel_->setBounds(bounds.removeFromLeft(200));
    
    // Current view takes remaining space
    if (currentView_) {
        currentView_->setBounds(bounds);
    }
}

void FLStudioMainWindow::setCurrentView(juce::Component* view) {
    if (currentView_) {
        removeChildComponent(currentView_);
    }
    
    currentView_ = view;
    
    if (currentView_) {
        addAndMakeVisible(currentView_);
        resized();
    }
}

void FLStudioMainWindow::showChannelRack() {
    setCurrentView(channelRackUI_.get());
}

void FLStudioMainWindow::showPlaylist() {
    // TODO: Implement Playlist view
}

void FLStudioMainWindow::showPianoRoll() {
    // TODO: Implement Piano Roll view
}

void FLStudioMainWindow::showMixer() {
    // TODO: Implement Mixer view
}

void FLStudioMainWindow::showBrowser() {
    // TODO: Implement Browser view
}

void FLStudioMainWindow::showRecording() {
    setCurrentView(recordingPanel_.get());
}

} // namespace GUI
} // namespace Omega
