//==============================================================================
// ChannelRackUI.cpp
// Professional Channel Rack UI Implementation
//==============================================================================

#include "ChannelRackUI.h"

namespace Omega {
namespace GUI {

//==============================================================================
// ChannelStripComponent Implementation
//==============================================================================
ChannelStripComponent::ChannelStripComponent(int channelIndex)
    : channelIndex_(channelIndex) {
    
    channelColor_ = FLColors::getChannelColor(channelIndex);
    channelName_ = "Channel " + juce::String(channelIndex + 1);
    
    // Mute button
    muteButton_ = std::make_unique<juce::TextButton>("M");
    muteButton_->setTooltip("Mute channel");
    muteButton_->onClick = [this]() {
        isMuted_ = !isMuted_;
        muteButton_->setToggleState(isMuted_, juce::dontSendNotification);
        if (onMuteClicked) onMuteClicked();
        repaint();
    };
    addAndMakeVisible(muteButton_.get());
    
    // Solo button
    soloButton_ = std::make_unique<juce::TextButton>("S");
    soloButton_->setTooltip("Solo channel");
    soloButton_->onClick = [this]() {
        isSolo_ = !isSolo_;
        soloButton_->setToggleState(isSolo_, juce::dontSendNotification);
        if (onSoloClicked) onSoloClicked();
        repaint();
    };
    addAndMakeVisible(soloButton_.get());
    
    // Volume knob
    volumeKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag,
                                                  juce::Slider::NoTextBox);
    volumeKnob_->setRange(0.0, 1.0, 0.01);
    volumeKnob_->setValue(volume_);
    volumeKnob_->setTooltip("Volume");
    volumeKnob_->onValueChange = [this]() {
        volume_ = volumeKnob_->getValue();
        if (onVolumeChanged) onVolumeChanged(volume_);
    };
    addAndMakeVisible(volumeKnob_.get());
    
    // Pan knob
    panKnob_ = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag,
                                               juce::Slider::NoTextBox);
    panKnob_->setRange(0.0, 1.0, 0.01);
    panKnob_->setValue(pan_);
    panKnob_->setTooltip("Pan (L-R)");
    panKnob_->onValueChange = [this]() {
        pan_ = panKnob_->getValue();
        if (onPanChanged) onPanChanged(pan_);
    };
    addAndMakeVisible(panKnob_.get());
}

void ChannelStripComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Background with channel color
    g.setColour(channelColor_.withAlpha(0.1f));
    g.fillRect(bounds);
    
    // Left color bar
    g.setColour(channelColor_);
    g.fillRect(bounds.removeFromLeft(4));
    
    // Selection highlight
    if (isSelected_) {
        g.setColour(FLColors::Orange.withAlpha(0.3f));
        g.fillRect(bounds);
        
        g.setColour(FLColors::Orange);
        g.drawRect(getLocalBounds(), 2);
    }
    
    // Muted overlay
    if (isMuted_) {
        g.setColour(FLColors::DarkBg.withAlpha(0.5f));
        g.fillRect(bounds);
    }
    
    // Channel name
    g.setColour(isMuted_ ? FLColors::TextDisabled : FLColors::TextPrimary);
    g.setFont(14.0f);
    auto nameArea = bounds.removeFromLeft(150).reduced(8, 4);
    g.drawText(channelName_, nameArea, juce::Justification::centredLeft, true);
    
    // Border
    g.setColour(FLColors::Border);
    g.drawRect(getLocalBounds(), 1);
}

void ChannelStripComponent::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(4); // Color bar space
    bounds.reduce(4, 4);
    
    // Channel name area
    bounds.removeFromLeft(150);
    
    // Buttons
    muteButton_->setBounds(bounds.removeFromLeft(30).reduced(2));
    bounds.removeFromLeft(4);
    soloButton_->setBounds(bounds.removeFromLeft(30).reduced(2));
    bounds.removeFromLeft(8);
    
    // Knobs
    volumeKnob_->setBounds(bounds.removeFromLeft(40).reduced(2));
    bounds.removeFromLeft(4);
    panKnob_->setBounds(bounds.removeFromLeft(40).reduced(2));
}

void ChannelStripComponent::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        // Show context menu
        juce::PopupMenu menu;
        menu.addItem(1, "Rename channel");
        menu.addItem(2, "Change color");
        menu.addSeparator();
        menu.addItem(3, "Duplicate channel");
        menu.addItem(4, "Delete channel");
        menu.addSeparator();
        menu.addItem(5, "Route to mixer...");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result > 0) {
                // Handle menu selection
            }
        });
    } else {
        isSelected_ = true;
        if (onChannelClicked) onChannelClicked();
        repaint();
    }
}

void ChannelStripComponent::mouseUp(const juce::MouseEvent& e) {
}

void ChannelStripComponent::mouseDrag(const juce::MouseEvent& e) {
    // Drag to reorder channels
    if (e.getDistanceFromDragStart() > 10) {
        auto dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (dragContainer != nullptr) {
            dragContainer->startDragging("channel_" + juce::String(channelIndex_),
                                        this, juce::ScaledImage(), true);
        }
    }
}

void ChannelStripComponent::setChannelName(const juce::String& name) {
    channelName_ = name;
    repaint();
}

void ChannelStripComponent::setChannelColor(juce::Colour color) {
    channelColor_ = color;
    repaint();
}

void ChannelStripComponent::setMuted(bool shouldBeMuted) {
    isMuted_ = shouldBeMuted;
    muteButton_->setToggleState(isMuted_, juce::dontSendNotification);
    repaint();
}

void ChannelStripComponent::setSolo(bool shouldBeSolo) {
    isSolo_ = shouldBeSolo;
    soloButton_->setToggleState(isSolo_, juce::dontSendNotification);
    repaint();
}

void ChannelStripComponent::setVolume(float volume) {
    volume_ = volume;
    volumeKnob_->setValue(volume, juce::dontSendNotification);
}

void ChannelStripComponent::setPan(float pan) {
    pan_ = pan;
    panKnob_->setValue(pan, juce::dontSendNotification);
}

//==============================================================================
// StepButton Implementation
//==============================================================================
StepButton::StepButton(int step, int channel)
    : step_(step), channel_(channel) {
    setSize(20, 20);
}

void StepButton::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(1);
    
    // Background
    juce::Colour bgColor = FLColors::MediumBg;
    if (isActive_) {
        bgColor = FLColors::Orange.withAlpha(velocity_);
    } else if (isHovered_) {
        bgColor = FLColors::LightBg;
    }
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Playback highlight
    if (isHighlighted_) {
        g.setColour(FLColors::Info);
        g.drawRoundedRectangle(bounds, 2.0f, 2.0f);
    }
    
    // Border
    g.setColour(FLColors::Border);
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    
    // Step number on every 4th beat
    if (step_ % 4 == 0) {
        g.setColour(FLColors::TextSecondary);
        g.setFont(8.0f);
        g.drawText(juce::String(step_ + 1), bounds.toNearestInt(),
                  juce::Justification::centred, false);
    }
}

void StepButton::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        // Show velocity editor
        juce::PopupMenu menu;
        menu.addSectionHeader("Step Velocity");
        menu.addItem(1, "100%", true, velocity_ == 1.0f);
        menu.addItem(2, "75%", true, velocity_ == 0.75f);
        menu.addItem(3, "50%", true, velocity_ == 0.5f);
        menu.addItem(4, "25%", true, velocity_ == 0.25f);
        menu.addSeparator();
        menu.addItem(5, "Custom...");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result > 0 && result <= 4) {
                float velocities[] = {1.0f, 0.75f, 0.5f, 0.25f};
                velocity_ = velocities[result - 1];
                if (onVelocityChanged) onVelocityChanged(step_, channel_, velocity_);
                repaint();
            }
        });
    } else {
        isActive_ = !isActive_;
        if (onStepToggled) onStepToggled(step_, channel_, isActive_);
        repaint();
    }
}

void StepButton::mouseUp(const juce::MouseEvent& e) {
}

void StepButton::mouseEnter(const juce::MouseEvent& e) {
    isHovered_ = true;
    repaint();
}

void StepButton::mouseExit(const juce::MouseEvent& e) {
    isHovered_ = false;
    repaint();
}

void StepButton::setActive(bool shouldBeActive) {
    isActive_ = shouldBeActive;
    repaint();
}

void StepButton::setVelocity(float velocity) {
    velocity_ = juce::jlimit(0.0f, 1.0f, velocity);
    repaint();
}

void StepButton::setHighlight(bool shouldHighlight) {
    isHighlighted_ = shouldHighlight;
    repaint();
}

//==============================================================================
// PatternGrid Implementation
//==============================================================================
PatternGrid::PatternGrid(int numChannels, int numSteps)
    : numChannels_(numChannels), numSteps_(numSteps) {
    createStepButtons();
}

void PatternGrid::paint(juce::Graphics& g) {
    g.fillAll(FLColors::DarkBg);
    
    // Draw grid lines
    g.setColour(FLColors::Border);
    
    auto stepWidth = getWidth() / (float)numSteps_;
    auto channelHeight = getHeight() / (float)numChannels_;
    
    // Vertical lines (every 4 beats highlighted)
    for (int i = 0; i <= numSteps_; ++i) {
        auto x = i * stepWidth;
        auto lineWidth = (i % 4 == 0) ? 1.5f : 0.5f;
        auto color = (i % 4 == 0) ? FLColors::BorderLight : FLColors::Border;
        g.setColour(color);
        g.drawLine(x, 0, x, getHeight(), lineWidth);
    }
    
    // Horizontal lines
    for (int i = 0; i <= numChannels_; ++i) {
        auto y = i * channelHeight;
        g.drawLine(0, y, getWidth(), y, 0.5f);
    }
}

void PatternGrid::resized() {
    auto stepWidth = getWidth() / numSteps_;
    auto channelHeight = getHeight() / numChannels_;
    
    for (int channel = 0; channel < numChannels_; ++channel) {
        for (int step = 0; step < numSteps_; ++step) {
            auto index = channel * numSteps_ + step;
            if (index < stepButtons_.size()) {
                auto x = step * stepWidth + 2;
                auto y = channel * channelHeight + 2;
                stepButtons_[index]->setBounds(x, y, stepWidth - 4, channelHeight - 4);
            }
        }
    }
}

void PatternGrid::setNumSteps(int steps) {
    numSteps_ = steps;
    createStepButtons();
    resized();
}

void PatternGrid::setStep(int channel, int step, bool active, float velocity) {
    auto index = channel * numSteps_ + step;
    if (index < stepButtons_.size()) {
        stepButtons_[index]->setActive(active);
        stepButtons_[index]->setVelocity(velocity);
    }
}

void PatternGrid::clearPattern() {
    for (auto* button : stepButtons_) {
        button->setActive(false);
    }
}

void PatternGrid::randomizePattern() {
    juce::Random random;
    for (auto* button : stepButtons_) {
        button->setActive(random.nextFloat() > 0.6f);
        button->setVelocity(random.nextFloat() * 0.5f + 0.5f);
    }
}

void PatternGrid::setPlaybackPosition(int step) {
    // Clear previous highlight
    if (currentPlaybackStep_ >= 0) {
        for (int channel = 0; channel < numChannels_; ++channel) {
            auto index = channel * numSteps_ + currentPlaybackStep_;
            if (index < stepButtons_.size()) {
                stepButtons_[index]->setHighlight(false);
            }
        }
    }
    
    // Set new highlight
    currentPlaybackStep_ = step;
    if (step >= 0) {
        for (int channel = 0; channel < numChannels_; ++channel) {
            auto index = channel * numSteps_ + step;
            if (index < stepButtons_.size()) {
                stepButtons_[index]->setHighlight(true);
            }
        }
    }
}

void PatternGrid::createStepButtons() {
    stepButtons_.clear();
    
    for (int channel = 0; channel < numChannels_; ++channel) {
        for (int step = 0; step < numSteps_; ++step) {
            auto* button = new StepButton(step, channel);
            button->onStepToggled = [this](int s, int c, bool active) {
                if (onStepChanged) onStepChanged(c, s, active);
            };
            stepButtons_.add(button);
            addAndMakeVisible(button);
        }
    }
}

//==============================================================================
// ChannelRackUI Implementation
//==============================================================================
ChannelRackUI::ChannelRackUI(OmegaStudio::Sequencer::ChannelRackEngine& channelRack)
    : channelRack_(channelRack) {
    
    setLookAndFeel(&flLookAndFeel_);
    
    // Create toolbar
    createToolbar();
    
    // Pattern selector
    patternSelector_ = std::make_unique<juce::ComboBox>();
    patternSelector_->addItem("Pattern 1", 1);
    patternSelector_->setSelectedId(1);
    patternSelector_->onChange = [this]() { onPatternSelected(); };
    addAndMakeVisible(patternSelector_.get());
    
    // Pattern name label
    patternNameLabel_ = std::make_unique<juce::Label>("Pattern Name", "Pattern 1");
    patternNameLabel_->setEditable(true);
    addAndMakeVisible(patternNameLabel_.get());
    
    // Channel viewport
    channelViewport_ = std::make_unique<juce::Viewport>();
    channelContainer_ = std::make_unique<juce::Component>();
    channelViewport_->setViewedComponent(channelContainer_.get(), false);
    addAndMakeVisible(channelViewport_.get());
    
    // Pattern grid
    patternGrid_ = std::make_unique<PatternGrid>(8, 16);
    patternGrid_->onStepChanged = [this](int channel, int step, bool active) {
        onStepToggled(channel, step, active);
    };
    addAndMakeVisible(*patternGrid_);
    
    // Add initial channels
    for (int i = 0; i < 8; ++i) {
        addChannel("Kick");
    }
    
    // Start timer for playback position updates
    startTimer(50);
    
    setSize(1200, 600);
}

ChannelRackUI::~ChannelRackUI() {
    setLookAndFeel(nullptr);
}

void ChannelRackUI::paint(juce::Graphics& g) {
    g.fillAll(FLColors::DarkBg);
    
    // Title bar
    auto titleBounds = getLocalBounds().removeFromTop(40);
    g.setColour(FLColors::PanelBg);
    g.fillRect(titleBounds);
    
    g.setColour(FLColors::Orange);
    g.setFont(18.0f);
    g.drawText("CHANNEL RACK", titleBounds.reduced(10, 0),
              juce::Justification::centredLeft, false);
}

void ChannelRackUI::resized() {
    auto bounds = getLocalBounds();
    
    // Title bar
    auto titleBar = bounds.removeFromTop(40);
    titleBar.removeFromLeft(150); // Title space
    
    patternSelector_->setBounds(titleBar.removeFromLeft(120).reduced(5));
    patternNameLabel_->setBounds(titleBar.removeFromLeft(150).reduced(5));
    
    addChannelButton_->setBounds(titleBar.removeFromRight(100).reduced(5));
    newPatternButton_->setBounds(titleBar.removeFromRight(100).reduced(5));
    
    // Main area
    auto channelListWidth = 300;
    auto channelListBounds = bounds.removeFromLeft(channelListWidth);
    
    channelViewport_->setBounds(channelListBounds);
    patternGrid_->setBounds(bounds);
    
    layoutChannels();
}

void ChannelRackUI::timerCallback() {
    // Update playback position from audio engine
    // auto currentStep = channelRack_.getCurrentStep();
    // updatePlaybackPosition(currentStep);
}

void ChannelRackUI::addChannel(const juce::String& instrumentName) {
    auto* strip = new ChannelStripComponent(channelStrips_.size());
    strip->setChannelName(instrumentName);
    strip->onChannelClicked = [this, index = channelStrips_.size()]() {
        onChannelStripClicked(index);
    };
    
    channelStrips_.add(strip);
    channelContainer_->addAndMakeVisible(strip);
    
    layoutChannels();
    
    // Update pattern grid
    patternGrid_ = std::make_unique<PatternGrid>(channelStrips_.size(), numSteps_);
    addAndMakeVisible(*patternGrid_);
    resized();
}

void ChannelRackUI::removeChannel(int channelIndex) {
    if (channelIndex >= 0 && channelIndex < channelStrips_.size()) {
        channelStrips_.remove(channelIndex);
        layoutChannels();
        
        patternGrid_ = std::make_unique<PatternGrid>(channelStrips_.size(), numSteps_);
        addAndMakeVisible(*patternGrid_);
        resized();
    }
}

void ChannelRackUI::duplicateChannel(int channelIndex) {
    (void)channelIndex; // Suppress unused warning
    // TODO: Implement channel duplication
}

void ChannelRackUI::clearChannel(int channelIndex) {
    // Clear all steps for this channel
    for (int step = 0; step < numSteps_; ++step) {
        patternGrid_->setStep(channelIndex, step, false);
    }
}

void ChannelRackUI::setCurrentPattern(int patternIndex) {
    currentPatternIndex_ = patternIndex;
    patternSelector_->setSelectedId(patternIndex + 1, juce::dontSendNotification);
}

void ChannelRackUI::newPattern() {
    // Create new pattern
    auto patternId = patternSelector_->getNumItems() + 1;
    patternSelector_->addItem("Pattern " + juce::String(patternId), patternId);
    patternSelector_->setSelectedId(patternId);
}

void ChannelRackUI::duplicatePattern() {
    // TODO: Duplicate current pattern
}

void ChannelRackUI::clearPattern() {
    patternGrid_->clearPattern();
}

void ChannelRackUI::randomizePattern() {
    patternGrid_->randomizePattern();
}

void ChannelRackUI::updatePlaybackPosition(int step) {
    patternGrid_->setPlaybackPosition(step);
}

void ChannelRackUI::layoutChannels() {
    const int channelHeight = 50;
    int y = 0;
    
    for (auto* strip : channelStrips_) {
        strip->setBounds(0, y, channelViewport_->getWidth(), channelHeight);
        y += channelHeight;
    }
    
    channelContainer_->setSize(channelViewport_->getWidth(), y);
}

void ChannelRackUI::layoutPatternGrid() {
    // Pattern grid takes remaining space
}

void ChannelRackUI::createToolbar() {
    addChannelButton_ = std::make_unique<juce::TextButton>("+ Channel");
    addChannelButton_->onClick = [this]() { onAddChannel(); };
    addAndMakeVisible(*addChannelButton_);
    
    newPatternButton_ = std::make_unique<juce::TextButton>("+ Pattern");
    newPatternButton_->onClick = [this]() { onNewPattern(); };
    addAndMakeVisible(*newPatternButton_);
}

void ChannelRackUI::onChannelStripClicked(int channelIndex) {
    (void)channelIndex; // Suppress unused warning
    // Deselect all others
    for (auto* strip : channelStrips_) {
        (void)strip;
        // strip->setSelected(false);
    }
    
    // Select clicked channel
    if (channelIndex >= 0 && channelIndex < channelStrips_.size()) {
        // channelStrips_[channelIndex]->setSelected(true);
    }
}

void ChannelRackUI::onStepToggled(int channel, int step, bool active) {
    (void)channel; (void)step; (void)active; // Suppress unused warnings
    // Update backend
    // channelRack_.setStep(channel, step, active);
}

void ChannelRackUI::onAddChannel() {
    juce::PopupMenu menu;
    menu.addSectionHeader("Add Instrument");
    menu.addItem(1, "Kick Drum");
    menu.addItem(2, "Snare Drum");
    menu.addItem(3, "Hi-Hat");
    menu.addItem(4, "Clap");
    menu.addSeparator();
    menu.addItem(10, "ProSampler");
    menu.addItem(11, "ProSynth");
    menu.addItem(12, "WavetableSynth");
    menu.addItem(13, "FM Synth");
    
    menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
        if (result > 0) {
            juce::String name = "Channel";
            if (result == 1) name = "Kick";
            else if (result == 2) name = "Snare";
            else if (result == 3) name = "Hi-Hat";
            else if (result == 4) name = "Clap";
            else if (result >= 10) name = "Synth";
            
            addChannel(name);
        }
    });
}

void ChannelRackUI::onNewPattern() {
    newPattern();
}

void ChannelRackUI::onPatternSelected() {
    currentPatternIndex_ = patternSelector_->getSelectedId() - 1;
    // Load pattern from backend
}

} // namespace GUI
} // namespace Omega
