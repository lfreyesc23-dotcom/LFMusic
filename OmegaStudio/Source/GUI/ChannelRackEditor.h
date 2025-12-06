#pragma once

#include <JuceHeader.h>
#include "../Memory/MemoryPool.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Step Sequencer Component - FL Studio style
 * 16-step grid per instrument with velocity per step
 */
class StepComponent : public juce::Component
{
public:
    StepComponent(int stepIndex, int channelIndex)
        : stepIdx(stepIndex), channelIdx(channelIndex)
    {
        setSize(40, 40);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        
        // Background
        if (isActive)
        {
            auto brightness = velocity / 127.0f;
            g.setColour(channelColor.withMultipliedBrightness(0.5f + brightness * 0.5f));
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Velocity indicator
            auto velHeight = bounds.getHeight() * (velocity / 127.0f);
            auto velBounds = bounds.removeFromBottom(velHeight);
            g.setColour(channelColor);
            g.fillRoundedRectangle(velBounds, 4.0f);
        }
        else
        {
            g.setColour(juce::Colours::darkgrey.darker());
            g.fillRoundedRectangle(bounds, 4.0f);
        }
        
        // Border
        g.setColour(isCurrent ? juce::Colours::yellow : juce::Colours::grey);
        g.drawRoundedRectangle(bounds, 4.0f, 2.0f);
        
        // Step number
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(10.0f);
        g.drawText(juce::String(stepIdx + 1), bounds, juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            isActive = !isActive;
            if (isActive && velocity == 0)
                velocity = 100;
            
            if (onStepChanged)
                onStepChanged(stepIdx, channelIdx, isActive, velocity);
            
            repaint();
        }
        else if (e.mods.isRightButtonDown())
        {
            isActive = false;
            velocity = 0;
            
            if (onStepChanged)
                onStepChanged(stepIdx, channelIdx, isActive, velocity);
            
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (isActive && e.mods.isLeftButtonDown())
        {
            // Drag vertical to change velocity
            auto dragDelta = e.getDistanceFromDragStartY();
            auto newVel = juce::jlimit(1, 127, velocity - static_cast<int>(dragDelta * 0.5f));
            
            if (newVel != velocity)
            {
                velocity = newVel;
                
                if (onStepChanged)
                    onStepChanged(stepIdx, channelIdx, isActive, velocity);
                
                repaint();
            }
        }
    }

    void setActive(bool active, uint8_t vel = 100)
    {
        isActive = active;
        velocity = active ? vel : 0;
        repaint();
    }

    void setCurrent(bool current)
    {
        isCurrent = current;
        repaint();
    }

    void setChannelColor(juce::Colour color)
    {
        channelColor = color;
        repaint();
    }

    std::function<void(int step, int channel, bool active, uint8_t velocity)> onStepChanged;

private:
    int stepIdx;
    int channelIdx;
    bool isActive = false;
    bool isCurrent = false;
    uint8_t velocity = 100;
    juce::Colour channelColor = juce::Colours::orange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepComponent)
};

//==============================================================================
/**
 * Channel Strip in Channel Rack
 * Contains instrument name, mute/solo, volume, pan, 16-step grid
 */
class ChannelRackStripComponent : public juce::Component
{
public:
    ChannelRackStripComponent(int channelIndex)
        : channelIdx(channelIndex)
    {
        // Channel name
        addAndMakeVisible(nameLabel);
        nameLabel.setText("Channel " + juce::String(channelIdx + 1), juce::dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey.darker());
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        // Mute button
        addAndMakeVisible(muteButton);
        muteButton.setButtonText("M");
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        muteButton.onClick = [this]() {
            if (onMuteToggled)
                onMuteToggled(channelIdx, muteButton.getToggleState());
        };
        
        // Solo button
        addAndMakeVisible(soloButton);
        soloButton.setButtonText("S");
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
        soloButton.onClick = [this]() {
            if (onSoloToggled)
                onSoloToggled(channelIdx, soloButton.getToggleState());
        };
        
        // Volume slider
        addAndMakeVisible(volumeSlider);
        volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        volumeSlider.setRange(0.0, 1.0, 0.01);
        volumeSlider.setValue(0.8);
        volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volumeSlider.onValueChange = [this]() {
            if (onVolumeChanged)
                onVolumeChanged(channelIdx, volumeSlider.getValue());
        };
        
        // Pan slider
        addAndMakeVisible(panSlider);
        panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(0.0);
        panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panSlider.onValueChange = [this]() {
            if (onPanChanged)
                onPanChanged(channelIdx, panSlider.getValue());
        };
        
        // Create 16 steps
        for (int i = 0; i < 16; ++i)
        {
            auto step = std::make_unique<StepComponent>(i, channelIdx);
            step->setChannelColor(getChannelColor(channelIdx));
            step->onStepChanged = [this](int stepIdx, int chanIdx, bool active, uint8_t vel) {
                if (onStepChanged)
                    onStepChanged(stepIdx, chanIdx, active, vel);
            };
            addAndMakeVisible(step.get());
            steps.push_back(std::move(step));
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Left controls area (200px)
        auto controlsArea = bounds.removeFromLeft(200);
        
        nameLabel.setBounds(controlsArea.removeFromTop(30).reduced(2));
        
        auto buttonArea = controlsArea.removeFromTop(30).reduced(2);
        muteButton.setBounds(buttonArea.removeFromLeft(30));
        buttonArea.removeFromLeft(5);
        soloButton.setBounds(buttonArea.removeFromLeft(30));
        
        controlsArea.removeFromTop(5);
        volumeSlider.setBounds(controlsArea.removeFromTop(20).reduced(5, 0));
        panSlider.setBounds(controlsArea.removeFromTop(20).reduced(5, 0));
        
        // Steps area
        auto stepsArea = bounds.reduced(5);
        int stepWidth = stepsArea.getWidth() / 16;
        
        for (int i = 0; i < 16; ++i)
        {
            steps[i]->setBounds(stepsArea.removeFromLeft(stepWidth).reduced(2));
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey.darker().darker());
        
        // Separator line
        g.setColour(juce::Colours::grey);
        g.drawLine(200.0f, 0.0f, 200.0f, static_cast<float>(getHeight()), 1.0f);
    }

    void setCurrentStep(int stepIndex)
    {
        for (int i = 0; i < 16; ++i)
        {
            steps[i]->setCurrent(i == stepIndex);
        }
    }

    void setStepActive(int stepIndex, bool active, uint8_t velocity = 100)
    {
        if (stepIndex >= 0 && stepIndex < 16)
        {
            steps[stepIndex]->setActive(active, velocity);
        }
    }

    void setChannelName(const juce::String& name)
    {
        nameLabel.setText(name, juce::dontSendNotification);
    }

    std::function<void(int channel, bool muted)> onMuteToggled;
    std::function<void(int channel, bool soloed)> onSoloToggled;
    std::function<void(int channel, float volume)> onVolumeChanged;
    std::function<void(int channel, float pan)> onPanChanged;
    std::function<void(int step, int channel, bool active, uint8_t velocity)> onStepChanged;

private:
    int channelIdx;
    juce::Label nameLabel;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::Slider volumeSlider;
    juce::Slider panSlider;
    std::vector<std::unique_ptr<StepComponent>> steps;

    juce::Colour getChannelColor(int index)
    {
        const juce::Colour colors[] = {
            juce::Colours::orange, juce::Colours::blue, juce::Colours::green,
            juce::Colours::purple, juce::Colours::yellow, juce::Colours::red,
            juce::Colours::cyan, juce::Colours::magenta
        };
        return colors[index % 8];
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackStripComponent)
};

//==============================================================================
/**
 * Channel Rack Editor - Main Component
 * FL Studio style step sequencer with multiple channels
 */
class ChannelRackEditor : public juce::Component,
                          private juce::Timer
{
public:
    ChannelRackEditor()
    {
        setupToolbar();
        
        // Create viewport for scrolling
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&channelContainer, false);
        viewport.setScrollBarsShown(true, false);
        
        // Create default 8 channels
        setNumChannels(8);
        
        // Start playback position timer
        startTimerHz(30);
    }

    ~ChannelRackEditor() override
    {
        stopTimer();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Toolbar at top
        auto toolbar = bounds.removeFromTop(40);
        toolbarArea = toolbar;
        
        // Pattern selector
        patternSelector.setBounds(toolbar.removeFromLeft(150).reduced(5));
        toolbar.removeFromLeft(10);
        
        // Swing slider
        auto swingArea = toolbar.removeFromLeft(200).reduced(5);
        swingLabel.setBounds(swingArea.removeFromLeft(60));
        swingSlider.setBounds(swingArea);
        
        toolbar.removeFromLeft(10);
        
        // Add/remove channel buttons
        addChannelButton.setBounds(toolbar.removeFromLeft(30).reduced(2));
        removeChannelButton.setBounds(toolbar.removeFromLeft(30).reduced(2));
        
        // Viewport for channels
        viewport.setBounds(bounds);
        
        // Update container size
        updateContainerSize();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        // Toolbar background
        g.setColour(juce::Colours::darkgrey.darker());
        g.fillRect(toolbarArea);
    }

    void setNumChannels(int numChannels)
    {
        channels.clear();
        
        for (int i = 0; i < numChannels; ++i)
        {
            auto channel = std::make_unique<ChannelRackStripComponent>(i);
            
            channel->onMuteToggled = [this](int idx, bool muted) {
                handleMuteToggled(idx, muted);
            };
            
            channel->onSoloToggled = [this](int idx, bool soloed) {
                handleSoloToggled(idx, soloed);
            };
            
            channel->onVolumeChanged = [this](int idx, float vol) {
                handleVolumeChanged(idx, vol);
            };
            
            channel->onPanChanged = [this](int idx, float pan) {
                handlePanChanged(idx, pan);
            };
            
            channel->onStepChanged = [this](int step, int chan, bool active, uint8_t vel) {
                handleStepChanged(step, chan, active, vel);
            };
            
            channelContainer.addAndMakeVisible(channel.get());
            channels.push_back(std::move(channel));
        }
        
        updateContainerSize();
    }

    void setCurrentStep(int stepIndex)
    {
        currentStep = stepIndex;
        
        for (auto& channel : channels)
        {
            channel->setCurrentStep(stepIndex);
        }
    }

private:
    void setupToolbar()
    {
        // Pattern selector
        addAndMakeVisible(patternSelector);
        patternSelector.addItem("Pattern 1", 1);
        patternSelector.addItem("Pattern 2", 2);
        patternSelector.addItem("Pattern 3", 3);
        patternSelector.addItem("Pattern 4", 4);
        patternSelector.setSelectedId(1);
        
        // Swing control
        addAndMakeVisible(swingLabel);
        swingLabel.setText("Swing:", juce::dontSendNotification);
        swingLabel.setJustificationType(juce::Justification::centredRight);
        
        addAndMakeVisible(swingSlider);
        swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        swingSlider.setRange(0.0, 1.0, 0.01);
        swingSlider.setValue(0.0);
        swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        
        // Add/Remove channel buttons
        addAndMakeVisible(addChannelButton);
        addChannelButton.setButtonText("+");
        addChannelButton.onClick = [this]() {
            setNumChannels(channels.size() + 1);
        };
        
        addAndMakeVisible(removeChannelButton);
        removeChannelButton.setButtonText("-");
        removeChannelButton.onClick = [this]() {
            if (channels.size() > 1)
                setNumChannels(channels.size() - 1);
        };
    }

    void updateContainerSize()
    {
        int totalHeight = channels.size() * 120;
        channelContainer.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), totalHeight);
        
        int y = 0;
        for (auto& channel : channels)
        {
            channel->setBounds(0, y, channelContainer.getWidth(), 120);
            y += 120;
        }
    }

    void timerCallback() override
    {
        // Update playback position visualization
        // This would connect to SequencerEngine for real playback position
    }

    void handleMuteToggled(int channelIndex, bool muted)
    {
        DBG("Channel " << channelIndex << " mute: " << muted);
        // Connect to audio engine
    }

    void handleSoloToggled(int channelIndex, bool soloed)
    {
        DBG("Channel " << channelIndex << " solo: " << soloed);
        // Connect to audio engine
    }

    void handleVolumeChanged(int channelIndex, float volume)
    {
        DBG("Channel " << channelIndex << " volume: " << volume);
        // Connect to audio engine
    }

    void handlePanChanged(int channelIndex, float pan)
    {
        DBG("Channel " << channelIndex << " pan: " << pan);
        // Connect to audio engine
    }

    void handleStepChanged(int step, int channel, bool active, uint8_t velocity)
    {
        DBG("Step " << step << " channel " << channel << " active: " << active << " vel: " << (int)velocity);
        // Connect to sequencer engine
    }

    // Toolbar components
    juce::Rectangle<int> toolbarArea;
    juce::ComboBox patternSelector;
    juce::Label swingLabel;
    juce::Slider swingSlider;
    juce::TextButton addChannelButton;
    juce::TextButton removeChannelButton;
    
    // Channel container
    juce::Viewport viewport;
    juce::Component channelContainer;
    std::vector<std::unique_ptr<ChannelRackStripComponent>> channels;
    
    int currentStep = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackEditor)
};

//==============================================================================
/**
 * Channel Rack Window
 */
class ChannelRackWindow : public juce::DocumentWindow
{
public:
    ChannelRackWindow()
        : DocumentWindow("Channel Rack", 
                        juce::Colours::darkgrey,
                        DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(&editor, true);
        setResizable(true, false);
        centreWithSize(1000, 600);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    ChannelRackEditor editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackWindow)
};

} // namespace GUI
} // namespace OmegaStudio
