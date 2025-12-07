//==============================================================================
// FunctionalChannelRack.h
// Channel Rack funcional estilo FL Studio con step sequencer
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Canal individual con step sequencer
//==============================================================================
class ChannelStrip : public juce::Component {
public:
    struct StepData {
        bool active = false;
        float velocity = 0.8f;
    };
    
    ChannelStrip(const juce::String& name, int numSteps = 16) 
        : channelName(name) {
        
        // Initialize steps array
        for (int i = 0; i < numSteps; ++i) {
            steps.add(StepData());
        }
        
        // Channel name button
        nameButton.setButtonText(name);
        nameButton.setColour(juce::TextButton::buttonColourId, getRandomColor());
        addAndMakeVisible(nameButton);
        
        // Mute button
        muteButton.setButtonText("M");
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        addAndMakeVisible(muteButton);
        
        // Solo button
        soloButton.setButtonText("S");
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        addAndMakeVisible(soloButton);
        
        // Volume slider
        volumeSlider.setRange(0.0, 1.0, 0.01);
        volumeSlider.setValue(0.8);
        volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible(volumeSlider);
        
        // Pan slider
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(0.0);
        panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        panSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible(panSlider);
        
        // Step buttons
        for (int i = 0; i < numSteps; ++i) {
            auto* stepButton = new juce::TextButton();
            stepButton->setClickingTogglesState(true);
            stepButton->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00BCD4));
            stepButton->onClick = [this, i] {
                steps.getReference(i).active = stepButtons[i]->getToggleState();
            };
            addAndMakeVisible(stepButton);
            stepButtons.add(stepButton);
        }
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        // Border
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(2);
        
        // Channel controls (left side)
        auto controlArea = area.removeFromLeft(250);
        nameButton.setBounds(controlArea.removeFromLeft(120).reduced(2));
        muteButton.setBounds(controlArea.removeFromLeft(30).reduced(2));
        soloButton.setBounds(controlArea.removeFromLeft(30).reduced(2));
        volumeSlider.setBounds(controlArea.removeFromLeft(35).reduced(2));
        panSlider.setBounds(controlArea.removeFromLeft(35).reduced(2));
        
        // Step sequencer (right side)
        auto stepWidth = area.getWidth() / stepButtons.size();
        for (auto* button : stepButtons) {
            button->setBounds(area.removeFromLeft(stepWidth).reduced(1));
        }
    }
    
    const juce::Array<StepData>& getSteps() const { return steps; }
    bool isMuted() const { return muteButton.getToggleState(); }
    bool isSoloed() const { return soloButton.getToggleState(); }
    float getVolume() const { return (float)volumeSlider.getValue(); }
    float getPan() const { return (float)panSlider.getValue(); }
    
private:
    juce::Colour getRandomColor() {
        static juce::Random random;
        return juce::Colour::fromHSV(random.nextFloat(), 0.6f, 0.8f, 1.0f);
    }
    
    juce::String channelName;
    juce::TextButton nameButton;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::Slider volumeSlider;
    juce::Slider panSlider;
    
    juce::OwnedArray<juce::TextButton> stepButtons;
    juce::Array<StepData> steps;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};

//==============================================================================
// Channel Rack completo
//==============================================================================
class FunctionalChannelRack : public juce::Component {
public:
    //==========================================================================
    std::function<void(int)> onChannelSelected;
    std::function<void()> onChannelsChanged;
    
    //==========================================================================
    FunctionalChannelRack() {
        // Add channel button
        addChannelButton.setButtonText("+ Add Channel");
        addChannelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4CAF50));
        addChannelButton.onClick = [this] { showAddChannelMenu(); };
        addAndMakeVisible(addChannelButton);
        
        // Add some default channels
        addChannel("Kick", "Drum");
        addChannel("Snare", "Drum");
        addChannel("Hi-Hat", "Drum");
        addChannel("Bass", "Synth");
        
        // Viewport for scrolling
        viewport.setViewedComponent(&channelContainer, false);
        viewport.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        // Add button at top
        addChannelButton.setBounds(area.removeFromTop(35).reduced(5));
        
        // Viewport for channels
        viewport.setBounds(area);
        
        // Layout channels in container
        auto containerArea = juce::Rectangle<int>(0, 0, area.getWidth(), 
                                                   channels.size() * 50);
        channelContainer.setBounds(containerArea);
        
        int y = 0;
        for (auto* channel : channels) {
            channel->setBounds(0, y, area.getWidth(), 48);
            y += 50;
        }
    }
    
    //==========================================================================
    // API pública
    //==========================================================================
    void addChannel(const juce::String& name, const juce::String& type) {
        auto* channel = new ChannelStrip(name + " (" + type + ")");
        channels.add(channel);
        channelContainer.addAndMakeVisible(channel);
        resized();
        
        if (onChannelsChanged) onChannelsChanged();
    }
    
    void removeChannel(int index) {
        if (juce::isPositiveAndBelow(index, channels.size())) {
            channels.remove(index);
            resized();
            if (onChannelsChanged) onChannelsChanged();
        }
    }
    
    int getNumChannels() const { return channels.size(); }
    
    ChannelStrip* getChannel(int index) {
        return channels[index];
    }
    
private:
    //==========================================================================
    void showAddChannelMenu() {
        juce::PopupMenu menu;
        
        juce::PopupMenu drumsMenu;
        drumsMenu.addItem(1, "Kick Drum");
        drumsMenu.addItem(2, "Snare");
        drumsMenu.addItem(3, "Hi-Hat");
        drumsMenu.addItem(4, "Tom");
        drumsMenu.addItem(5, "Cymbal");
        
        juce::PopupMenu synthMenu;
        synthMenu.addItem(10, "Lead Synth");
        synthMenu.addItem(11, "Pad");
        synthMenu.addItem(12, "Bass Synth");
        synthMenu.addItem(13, "Pluck");
        synthMenu.addItem(14, "Brass");
        
        juce::PopupMenu samplerMenu;
        samplerMenu.addItem(20, "Audio Clip");
        samplerMenu.addItem(21, "Vocal Sample");
        samplerMenu.addItem(22, "FX Sample");
        
        menu.addSubMenu("Drums", drumsMenu);
        menu.addSubMenu("Synths", synthMenu);
        menu.addSubMenu("Sampler", samplerMenu);
        menu.addSeparator();
        menu.addItem(100, "MIDI Input");
        menu.addItem(101, "Audio Input");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result == 0) return;
            
            juce::String name;
            juce::String type;
            
            // Drums
            if (result == 1) { name = "Kick"; type = "Drum"; }
            else if (result == 2) { name = "Snare"; type = "Drum"; }
            else if (result == 3) { name = "Hi-Hat"; type = "Drum"; }
            else if (result == 4) { name = "Tom"; type = "Drum"; }
            else if (result == 5) { name = "Cymbal"; type = "Drum"; }
            // Synths
            else if (result == 10) { name = "Lead"; type = "Synth"; }
            else if (result == 11) { name = "Pad"; type = "Synth"; }
            else if (result == 12) { name = "Bass"; type = "Synth"; }
            else if (result == 13) { name = "Pluck"; type = "Synth"; }
            else if (result == 14) { name = "Brass"; type = "Synth"; }
            // Sampler
            else if (result == 20) { name = "Audio"; type = "Sampler"; }
            else if (result == 21) { name = "Vocal"; type = "Sampler"; }
            else if (result == 22) { name = "FX"; type = "Sampler"; }
            // Inputs
            else if (result == 100) { name = "MIDI In"; type = "Input"; }
            else if (result == 101) { name = "Audio In"; type = "Input"; }
            
            if (name.isNotEmpty()) {
                addChannel(name, type);
            }
        });
    }
    
    //==========================================================================
    // Components
    //==========================================================================
    juce::TextButton addChannelButton;
    juce::Viewport viewport;
    juce::Component channelContainer;
    juce::OwnedArray<ChannelStrip> channels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionalChannelRack)
};

} // namespace GUI
} // namespace OmegaStudio
