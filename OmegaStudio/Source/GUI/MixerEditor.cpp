/*
  ==============================================================================
    MixerEditor.cpp
    Implementación del Mixer Visual
  ==============================================================================
*/

#include "MixerEditor.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// PeakMeterComponent
//==============================================================================

PeakMeterComponent::PeakMeterComponent() {
    startTimer(30); // 33 FPS
}

PeakMeterComponent::~PeakMeterComponent() {
    stopTimer();
}

void PeakMeterComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    float channelWidth = bounds.getWidth() / 2.0f;
    
    // Background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Left channel
    paintMeterChannel(g, juce::Rectangle<float>(0, 0, channelWidth - 1, bounds.getHeight()), 
                     leftPeak_, leftRMS_, leftPeakHold_);
    
    // Right channel
    paintMeterChannel(g, juce::Rectangle<float>(channelWidth + 1, 0, channelWidth - 1, bounds.getHeight()), 
                     rightPeak_, rightRMS_, rightPeakHold_);
}

void PeakMeterComponent::paintMeterChannel(juce::Graphics& g, juce::Rectangle<float> area, 
                                          float peak, float rms, float peakHold) {
    float height = area.getHeight();
    
    // RMS bar (darker)
    float rmsHeight = rms * height;
    juce::Colour rmsColour = juce::Colours::green;
    if (rms > 0.8f) rmsColour = juce::Colours::orange;
    if (rms > 0.95f) rmsColour = juce::Colours::red;
    
    g.setColour(rmsColour.darker(0.5f));
    g.fillRect(area.getX(), area.getBottom() - rmsHeight, area.getWidth(), rmsHeight);
    
    // Peak bar (brighter)
    float peakHeight = peak * height;
    juce::Colour peakColour = juce::Colours::green.brighter();
    if (peak > 0.8f) peakColour = juce::Colours::orange.brighter();
    if (peak > 0.95f) peakColour = juce::Colours::red;
    
    g.setColour(peakColour.withAlpha(0.8f));
    g.fillRect(area.getX(), area.getBottom() - peakHeight, area.getWidth(), peakHeight);
    
    // Peak hold line
    if (peakHold > 0.01f) {
        float holdY = area.getBottom() - peakHold * height;
        g.setColour(juce::Colours::white);
        g.drawHorizontalLine((int)holdY, area.getX(), area.getRight());
    }
    
    // Clip indicator
    if (peak >= 0.995f) {
        g.setColour(juce::Colours::red);
        g.fillRect(area.getX(), 0.0f, area.getWidth(), 5.0f);
    }
}

void PeakMeterComponent::resized() {
}

void PeakMeterComponent::timerCallback() {
    // Decay peaks
    leftPeak_ *= PEAK_DECAY;
    rightPeak_ *= PEAK_DECAY;
    
    // Peak hold
    if (++peakHoldCounter_ > PEAK_HOLD_TIME) {
        leftPeakHold_ *= 0.95f;
        rightPeakHold_ *= 0.95f;
    }
    
    repaint();
}

void PeakMeterComponent::setPeakLevel(float leftPeak, float rightPeak) {
    if (leftPeak > leftPeak_) {
        leftPeak_ = leftPeak;
        if (leftPeak > leftPeakHold_) {
            leftPeakHold_ = leftPeak;
            peakHoldCounter_ = 0;
        }
    }
    
    if (rightPeak > rightPeak_) {
        rightPeak_ = rightPeak;
        if (rightPeak > rightPeakHold_) {
            rightPeakHold_ = rightPeak;
            peakHoldCounter_ = 0;
        }
    }
}

void PeakMeterComponent::setRMSLevel(float leftRMS, float rightRMS) {
    leftRMS_ = leftRMS;
    rightRMS_ = rightRMS;
}

void PeakMeterComponent::reset() {
    leftPeak_ = rightPeak_ = 0.0f;
    leftRMS_ = rightRMS_ = 0.0f;
    leftPeakHold_ = rightPeakHold_ = 0.0f;
}

//==============================================================================
// PluginSlotComponent
//==============================================================================

PluginSlotComponent::PluginSlotComponent(int slotIndex)
    : slotIndex_(slotIndex) {
}

void PluginSlotComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    if (plugin_) {
        // Plugin loaded
        g.setColour(juce::Colour(0xff4a4a4a));
        g.fillRoundedRectangle(bounds, 3.0f);
        
        if (hovering_) {
            g.setColour(juce::Colour(0xff6a6a6a));
            g.drawRoundedRectangle(bounds, 3.0f, 2.0f);
        }
        
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText(plugin_->getName(), bounds, juce::Justification::centred);
        
        // Bypass indicator
        if (plugin_->isBypassed()) {
            g.setColour(juce::Colours::red.withAlpha(0.5f));
            g.fillRect(bounds.reduced(2.0f));
        }
    } else {
        // Empty slot
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 3.0f);
        
        if (hovering_) {
            g.setColour(juce::Colour(0xff3a3a3a));
            g.fillRoundedRectangle(bounds, 3.0f);
        }
        
        g.setColour(juce::Colours::grey);
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        
        g.setFont(18.0f);
        g.drawText("+", bounds, juce::Justification::centred);
    }
}

void PluginSlotComponent::mouseDown(const juce::MouseEvent& e) {
    if (e.getNumberOfClicks() == 2) {
        if (onDoubleClick) onDoubleClick(slotIndex_);
    } else if (e.mods.isRightButtonDown()) {
        if (onRightClick) onRightClick(slotIndex_);
    }
}

void PluginSlotComponent::mouseEnter(const juce::MouseEvent& e) {
    hovering_ = true;
    repaint();
}

void PluginSlotComponent::mouseExit(const juce::MouseEvent& e) {
    hovering_ = false;
    repaint();
}

void PluginSlotComponent::setPlugin(std::shared_ptr<PluginInstance> plugin) {
    plugin_ = plugin;
    repaint();
}

//==============================================================================
// MixerKnob
//==============================================================================

MixerKnob::MixerKnob(const juce::String& label)
    : label_(label) {
}

void MixerKnob::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    int knobSize = std::min(bounds.getWidth(), bounds.getHeight() - 20);
    auto knobArea = juce::Rectangle<int>(0, 0, knobSize, knobSize)
                       .withCentre(bounds.getCentre().withY(knobSize / 2));
    
    // Knob circle
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillEllipse(knobArea.toFloat());
    
    // Value arc
    float angle = -2.4f + value_ * 4.8f; // -135° to +135°
    juce::Path arc;
    arc.addCentredArc(knobArea.getCentreX(), knobArea.getCentreY(),
                     knobSize / 2.0f - 3, knobSize / 2.0f - 3,
                     0.0f, -2.4f, angle, true);
    
    g.setColour(juce::Colour(0xffff8c42)); // FL Orange
    g.strokePath(arc, juce::PathStrokeType(3.0f));
    
    // Pointer
    float pointerAngle = angle;
    float pointerLength = knobSize / 2.0f - 5;
    float pointerX = knobArea.getCentreX() + pointerLength * std::sin(pointerAngle);
    float pointerY = knobArea.getCentreY() - pointerLength * std::cos(pointerAngle);
    
    g.setColour(juce::Colours::white);
    g.drawLine(knobArea.getCentreX(), knobArea.getCentreY(), pointerX, pointerY, 2.0f);
    
    // Label
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);
    g.drawText(label_, bounds.withTop(knobSize + 5), juce::Justification::centredTop);
    
    // Value
    juce::String valueText = juce::String(value_, 2);
    g.setFont(9.0f);
    g.setColour(juce::Colours::grey);
    g.drawText(valueText, bounds.withTop(knobSize + 18), juce::Justification::centredTop);
}

void MixerKnob::mouseDown(const juce::MouseEvent& e) {
    dragStart_ = e.getPosition();
    dragStartValue_ = value_;
}

void MixerKnob::mouseDrag(const juce::MouseEvent& e) {
    float delta = (dragStart_.y - e.getPosition().y) / 100.0f;
    if (e.mods.isShiftDown()) delta *= 0.1f; // Fine control
    
    setValue(std::clamp(dragStartValue_ + delta, 0.0f, 1.0f));
    
    if (onValueChanged) onValueChanged(value_);
}

void MixerKnob::mouseDoubleClick(const juce::MouseEvent& e) {
    setValue(defaultValue_);
    if (onValueChanged) onValueChanged(value_);
}

void MixerKnob::setValue(float value) {
    value_ = std::clamp(value, minValue_, maxValue_);
    repaint();
}

void MixerKnob::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
}

//==============================================================================
// ChannelStripComponent
//==============================================================================

ChannelStripComponent::ChannelStripComponent(int channelIndex)
    : channelIndex_(channelIndex),
      channelName_("Ch " + juce::String(channelIndex + 1)),
      channelColour_(juce::Colour::fromHSV(channelIndex / 16.0f, 0.7f, 0.8f, 1.0f)) {
    
    sendLevels_.fill(0.0f);
    setupComponents();
    startTimer(30);
}

ChannelStripComponent::~ChannelStripComponent() {
    stopTimer();
}

void ChannelStripComponent::setupComponents() {
    // Peak meter
    peakMeter_ = std::make_unique<PeakMeterComponent>();
    addAndMakeVisible(peakMeter_.get());
    
    // Volume fader
    volumeSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, 
                                                   juce::Slider::NoTextBox);
    volumeSlider_->setRange(0.0, 1.0);
    volumeSlider_->setValue(volume_);
    volumeSlider_->onValueChange = [this]() {
        volume_ = volumeSlider_->getValue();
        if (onVolumeChanged) onVolumeChanged(channelIndex_, volume_);
    };
    addAndMakeVisible(volumeSlider_.get());
    
    // Pan knob
    panKnob_ = std::make_unique<MixerKnob>("Pan");
    panKnob_->setValue(pan_);
    panKnob_->setDefaultValue(0.5f);
    panKnob_->onValueChanged = [this](float value) {
        pan_ = value;
        if (onPanChanged) onPanChanged(channelIndex_, pan_);
    };
    addAndMakeVisible(panKnob_.get());
    
    // Send knobs
    for (int i = 0; i < 4; ++i) {
        sendKnobs_[i] = std::make_unique<MixerKnob>("S" + juce::String(i + 1));
        sendKnobs_[i]->setValue(0.0f);
        addAndMakeVisible(sendKnobs_[i].get());
    }
    
    // Plugin slots
    for (int i = 0; i < 8; ++i) {
        pluginSlots_[i] = std::make_unique<PluginSlotComponent>(i);
        pluginSlots_[i]->onDoubleClick = [this](int slot) {
            if (onPluginSlotClicked) onPluginSlotClicked(channelIndex_, slot);
        };
        addAndMakeVisible(pluginSlots_[i].get());
    }
    
    // Buttons
    soloButton_ = std::make_unique<juce::TextButton>("S");
    soloButton_->setClickingTogglesState(true);
    soloButton_->onClick = [this]() {
        solo_ = soloButton_->getToggleState();
        if (onSoloChanged) onSoloChanged(channelIndex_, solo_);
    };
    addAndMakeVisible(soloButton_.get());
    
    muteButton_ = std::make_unique<juce::TextButton>("M");
    muteButton_->setClickingTogglesState(true);
    muteButton_->onClick = [this]() {
        mute_ = muteButton_->getToggleState();
        if (onMuteChanged) onMuteChanged(channelIndex_, mute_);
    };
    addAndMakeVisible(muteButton_.get());
    
    armButton_ = std::make_unique<juce::TextButton>("R");
    armButton_->setClickingTogglesState(true);
    addAndMakeVisible(armButton_.get());
    
    // Name label
    nameLabel_ = std::make_unique<juce::Label>("name", channelName_);
    nameLabel_->setJustificationType(juce::Justification::centred);
    nameLabel_->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel_.get());
}

void ChannelStripComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Background
    g.setColour(selected_ ? juce::Colour(0xff3a3a3a) : juce::Colour(0xff2a2a2a));
    g.fillRect(bounds);
    
    // Color stripe
    g.setColour(channelColour_);
    g.fillRect(0, 0, 4, getHeight());
    
    // Border
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(bounds, 1);
}

void ChannelStripComponent::resized() {
    auto bounds = getLocalBounds().reduced(5);
    
    // Name at top
    nameLabel_->setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(5);
    
    // Peak meter
    peakMeter_->setBounds(bounds.removeFromTop(100));
    bounds.removeFromTop(5);
    
    // Plugin slots (4 visible)
    for (int i = 0; i < 4; ++i) {
        pluginSlots_[i]->setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(2);
    }
    bounds.removeFromTop(5);
    
    // Volume fader
    volumeSlider_->setBounds(bounds.removeFromTop(150));
    bounds.removeFromTop(5);
    
    // Pan knob
    panKnob_->setBounds(bounds.removeFromTop(50));
    bounds.removeFromTop(5);
    
    // Buttons
    auto buttonArea = bounds.removeFromTop(25);
    int buttonWidth = buttonArea.getWidth() / 3;
    soloButton_->setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    muteButton_->setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    armButton_->setBounds(buttonArea.reduced(2));
}

void ChannelStripComponent::timerCallback() {
    // Update meters from engine
    repaint();
}

void ChannelStripComponent::setChannelName(const juce::String& name) {
    channelName_ = name;
    nameLabel_->setText(name, juce::dontSendNotification);
}

void ChannelStripComponent::setColour(juce::Colour colour) {
    channelColour_ = colour;
    repaint();
}

void ChannelStripComponent::setSelected(bool selected) {
    selected_ = selected;
    repaint();
}

void ChannelStripComponent::updateMeters(float leftPeak, float rightPeak, float leftRMS, float rightRMS) {
    peakMeter_->setPeakLevel(leftPeak, rightPeak);
    peakMeter_->setRMSLevel(leftRMS, rightRMS);
}

void ChannelStripComponent::setVolume(float volume) {
    volume_ = volume;
    volumeSlider_->setValue(volume, juce::dontSendNotification);
}

void ChannelStripComponent::setPan(float pan) {
    pan_ = pan;
    panKnob_->setValue(pan);
}

void ChannelStripComponent::setSolo(bool solo) {
    solo_ = solo;
    soloButton_->setToggleState(solo, juce::dontSendNotification);
}

void ChannelStripComponent::setMute(bool mute) {
    mute_ = mute;
    muteButton_->setToggleState(mute, juce::dontSendNotification);
}

void ChannelStripComponent::setArm(bool arm) {
    arm_ = arm;
    armButton_->setToggleState(arm, juce::dontSendNotification);
}

void ChannelStripComponent::setSendLevel(int sendIndex, float level) {
    if (sendIndex >= 0 && sendIndex < 4) {
        sendLevels_[sendIndex] = level;
        sendKnobs_[sendIndex]->setValue(level);
    }
}

float ChannelStripComponent::getSendLevel(int sendIndex) const {
    return (sendIndex >= 0 && sendIndex < 4) ? sendLevels_[sendIndex] : 0.0f;
}

void ChannelStripComponent::setPlugin(int slotIndex, std::shared_ptr<PluginInstance> plugin) {
    if (slotIndex >= 0 && slotIndex < 8) {
        pluginSlots_[slotIndex]->setPlugin(plugin);
    }
}

std::shared_ptr<PluginInstance> ChannelStripComponent::getPlugin(int slotIndex) const {
    if (slotIndex >= 0 && slotIndex < 8) {
        return pluginSlots_[slotIndex]->getPlugin();
    }
    return nullptr;
}

//==============================================================================
// MasterStripComponent
//==============================================================================

MasterStripComponent::MasterStripComponent() {
    peakMeter_ = std::make_unique<PeakMeterComponent>();
    addAndMakeVisible(peakMeter_.get());
    
    volumeSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearVertical,
                                                   juce::Slider::NoTextBox);
    volumeSlider_->setRange(0.0, 1.0);
    volumeSlider_->setValue(volume_);
    volumeSlider_->onValueChange = [this]() {
        volume_ = volumeSlider_->getValue();
        if (onVolumeChanged) onVolumeChanged(volume_);
    };
    addAndMakeVisible(volumeSlider_.get());
    
    nameLabel_ = std::make_unique<juce::Label>("name", "MASTER");
    nameLabel_->setJustificationType(juce::Justification::centred);
    nameLabel_->setFont(juce::Font(14.0f, juce::Font::bold));
    nameLabel_->setColour(juce::Label::textColourId, juce::Colour(0xffff8c42));
    addAndMakeVisible(nameLabel_.get());
    
    startTimer(30);
}

MasterStripComponent::~MasterStripComponent() {
    stopTimer();
}

void MasterStripComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    g.setColour(juce::Colour(0xffff8c42));
    g.fillRect(0, 0, 4, getHeight());
    
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawRect(getLocalBounds(), 2);
}

void MasterStripComponent::resized() {
    auto bounds = getLocalBounds().reduced(5);
    
    nameLabel_->setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(5);
    
    peakMeter_->setBounds(bounds.removeFromTop(150));
    bounds.removeFromTop(10);
    
    volumeSlider_->setBounds(bounds);
}

void MasterStripComponent::timerCallback() {
    repaint();
}

void MasterStripComponent::updateMeters(float leftPeak, float rightPeak, float leftRMS, float rightRMS) {
    peakMeter_->setPeakLevel(leftPeak, rightPeak);
    peakMeter_->setRMSLevel(leftRMS, rightRMS);
}

void MasterStripComponent::setVolume(float volume) {
    volume_ = volume;
    volumeSlider_->setValue(volume, juce::dontSendNotification);
}

//==============================================================================
// MixerEditor
//==============================================================================

MixerEditor::MixerEditor() {
    channelContainer_ = std::make_unique<juce::Component>();
    
    channelViewport_ = std::make_unique<juce::Viewport>();
    channelViewport_->setViewedComponent(channelContainer_.get(), false);
    addAndMakeVisible(channelViewport_.get());
    
    masterStrip_ = std::make_unique<MasterStripComponent>();
    addAndMakeVisible(masterStrip_.get());
    
    setNumChannels(16); // Default 16 channels
    
    startTimer(30);
}

MixerEditor::~MixerEditor() {
    stopTimer();
}

void MixerEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    if (routingVisible_) {
        paintRoutingLines(g);
    }
}

void MixerEditor::resized() {
    auto bounds = getLocalBounds();
    
    // Master strip on right
    masterStrip_->setBounds(bounds.removeFromRight(MASTER_STRIP_WIDTH));
    
    // Channel viewport
    channelViewport_->setBounds(bounds);
    
    // Resize channel container
    int totalWidth = (int)channelStrips_.size() * CHANNEL_STRIP_WIDTH;
    channelContainer_->setSize(totalWidth, bounds.getHeight());
    
    // Position channel strips
    int x = 0;
    for (auto& strip : channelStrips_) {
        strip->setBounds(x, 0, CHANNEL_STRIP_WIDTH, bounds.getHeight());
        x += CHANNEL_STRIP_WIDTH;
    }
}

void MixerEditor::timerCallback() {
    if (mixerEngine_) {
        updateFromEngine();
    }
}

void MixerEditor::setNumChannels(int numChannels) {
    channelStrips_.clear();
    
    for (int i = 0; i < numChannels; ++i) {
        auto strip = std::make_unique<ChannelStripComponent>(i);
        
        strip->onChannelSelected = [this](int channel) {
            selectChannel(channel);
        };
        
        channelContainer_->addAndMakeVisible(strip.get());
        channelStrips_.push_back(std::move(strip));
    }
    
    resized();
}

void MixerEditor::selectChannel(int channelIndex) {
    if (selectedChannel_ >= 0 && selectedChannel_ < (int)channelStrips_.size()) {
        channelStrips_[selectedChannel_]->setSelected(false);
    }
    
    selectedChannel_ = channelIndex;
    
    if (selectedChannel_ >= 0 && selectedChannel_ < (int)channelStrips_.size()) {
        channelStrips_[selectedChannel_]->setSelected(true);
    }
}

ChannelStripComponent* MixerEditor::getChannelStrip(int index) {
    if (index >= 0 && index < (int)channelStrips_.size()) {
        return channelStrips_[index].get();
    }
    return nullptr;
}

void MixerEditor::setRoutingVisible(bool visible) {
    routingVisible_ = visible;
    repaint();
}

void MixerEditor::setMixerEngine(OmegaStudio::MixerEngine* engine) {
    mixerEngine_ = engine;
}

void MixerEditor::updateFromEngine() {
    // Update meters and controls from engine
    // This would read from the MixerEngine
}

void MixerEditor::setupChannelStrips() {
}

void MixerEditor::paintRoutingLines(juce::Graphics& g) {
    // Draw routing connections between channels
    g.setColour(juce::Colour(0xffff8c42).withAlpha(0.5f));
    
    // Example: draw sends routing
    for (size_t i = 0; i < channelStrips_.size(); ++i) {
        auto* strip = channelStrips_[i].get();
        // Draw lines from sends to corresponding channels
    }
}

//==============================================================================
// MixerWindow
//==============================================================================

MixerWindow::MixerWindow(const juce::String& name)
    : DocumentWindow(name, juce::Colour(0xff2a2a2a), DocumentWindow::allButtons) {
    
    editor_ = new MixerEditor();
    setContentOwned(editor_, true);
    
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(1400, 800);
    
    setVisible(true);
}

MixerWindow::~MixerWindow() {
}

void MixerWindow::closeButtonPressed() {
    setVisible(false);
}

} // namespace GUI
} // namespace OmegaStudio
