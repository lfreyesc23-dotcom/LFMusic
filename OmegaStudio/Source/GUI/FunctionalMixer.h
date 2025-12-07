//==============================================================================
// FunctionalMixer.h
// Mixer profesional estilo FL Studio con faders y medidores
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Medidor de nivel estéreo
//==============================================================================
class LevelMeter : public juce::Component,
                   private juce::Timer {
public:
    LevelMeter() {
        startTimer(50); // 20 FPS
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff0a0a0a));
        
        int halfWidth = getWidth() / 2;
        
        // Left channel
        drawMeter(g, 0, 0, halfWidth - 1, getHeight(), leftLevel);
        
        // Right channel
        drawMeter(g, halfWidth + 1, 0, halfWidth - 1, getHeight(), rightLevel);
        
        // Border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void setLevels(float left, float right) {
        leftLevel = left;
        rightLevel = right;
    }
    
private:
    void timerCallback() override {
        // Decay levels
        leftLevel *= 0.95f;
        rightLevel *= 0.95f;
        repaint();
    }
    
    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level) {
        int levelHeight = (int)(level * height);
        int startY = height - levelHeight;
        
        // Green zone (0 to -12dB)
        int greenHeight = juce::jmin(levelHeight, (int)(height * 0.7f));
        if (greenHeight > 0) {
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff00FF00), (float)x, (float)(height - greenHeight),
                juce::Colour(0xff00AA00), (float)x, (float)height,
                false
            ));
            g.fillRect(x, height - greenHeight, width, greenHeight);
        }
        
        // Yellow zone (-12 to -6dB)
        if (levelHeight > height * 0.7f) {
            int yellowHeight = juce::jmin((int)(levelHeight - height * 0.7f), (int)(height * 0.2f));
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xffFFFF00), (float)x, (float)(height * 0.3f - yellowHeight),
                juce::Colour(0xffAAAA00), (float)x, (float)(height * 0.3f),
                false
            ));
            g.fillRect(x, (int)(height * 0.3f - yellowHeight), width, yellowHeight);
        }
        
        // Red zone (-6 to 0dB)
        if (levelHeight > height * 0.9f) {
            int redHeight = levelHeight - (int)(height * 0.9f);
            g.setColour(juce::Colour(0xffFF0000));
            g.fillRect(x, startY, width, redHeight);
        }
    }
    
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

//==============================================================================
// Canal de mixer individual
//==============================================================================
class MixerChannel : public juce::Component {
public:
    std::function<void(float)> onVolumeChange;
    std::function<void(float)> onPanChange;
    std::function<void(bool)> onMuteToggle;
    std::function<void(bool)> onSoloToggle;
    
    MixerChannel(const juce::String& name, int channelNum)
        : channelName(name), channelNumber(channelNum) {
        
        // Channel name
        nameLabel.setText(name, juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setColour(juce::Label::backgroundColourId, getChannelColor());
        addAndMakeVisible(nameLabel);
        
        // Volume fader
        volumeFader.setRange(0.0, 1.0, 0.01);
        volumeFader.setValue(0.8);
        volumeFader.setSliderStyle(juce::Slider::LinearVertical);
        volumeFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        volumeFader.setColour(juce::Slider::thumbColourId, juce::Colour(0xff4CAF50));
        volumeFader.onValueChange = [this] {
            if (onVolumeChange) onVolumeChange((float)volumeFader.getValue());
        };
        addAndMakeVisible(volumeFader);
        
        // Pan knob
        panKnob.setRange(-1.0, 1.0, 0.01);
        panKnob.setValue(0.0);
        panKnob.setSliderStyle(juce::Slider::Rotary);
        panKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        panKnob.setColour(juce::Slider::thumbColourId, juce::Colour(0xff2196F3));
        panKnob.onValueChange = [this] {
            if (onPanChange) onPanChange((float)panKnob.getValue());
        };
        addAndMakeVisible(panKnob);
        
        panLabel.setText("Pan", juce::dontSendNotification);
        panLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(panLabel);
        
        // Mute button
        muteButton.setButtonText("M");
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        muteButton.onClick = [this] {
            if (onMuteToggle) onMuteToggle(muteButton.getToggleState());
        };
        addAndMakeVisible(muteButton);
        
        // Solo button
        soloButton.setButtonText("S");
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        soloButton.onClick = [this] {
            if (onSoloToggle) onSoloToggle(soloButton.getToggleState());
        };
        addAndMakeVisible(soloButton);
        
        // Record button
        recordButton.setButtonText("●");
        recordButton.setClickingTogglesState(true);
        recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        addAndMakeVisible(recordButton);
        
        // Level meter
        addAndMakeVisible(levelMeter);
        
        // FX slots
        for (int i = 0; i < 3; ++i) {
            auto* fxButton = new juce::TextButton();
            fxButton->setButtonText("FX " + juce::String(i + 1));
            fxButton->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
            addAndMakeVisible(fxButton);
            fxButtons.add(fxButton);
        }
    }
    
    void paint(juce::Graphics& g) override {
        // Background
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        // Border
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(5);
        
        // Name at top
        nameLabel.setBounds(area.removeFromTop(25));
        area.removeFromTop(5);
        
        // FX slots
        for (auto* fx : fxButtons) {
            fx->setBounds(area.removeFromTop(20));
            area.removeFromTop(2);
        }
        
        area.removeFromTop(10);
        
        // Pan knob and label
        panLabel.setBounds(area.removeFromTop(15));
        panKnob.setBounds(area.removeFromTop(70));
        
        area.removeFromTop(10);
        
        // Volume fader (main area)
        auto faderArea = area.removeFromTop(area.getHeight() - 100);
        volumeFader.setBounds(faderArea.removeFromLeft(40).reduced(5));
        
        // Level meter next to fader
        levelMeter.setBounds(faderArea.reduced(2));
        
        area.removeFromTop(10);
        
        // Control buttons at bottom
        muteButton.setBounds(area.removeFromTop(25));
        area.removeFromTop(2);
        soloButton.setBounds(area.removeFromTop(25));
        area.removeFromTop(2);
        recordButton.setBounds(area.removeFromTop(25));
    }
    
    void updateLevels(float left, float right) {
        // Simulate audio levels based on volume
        float vol = (float)volumeFader.getValue();
        if (!muteButton.getToggleState()) {
            float randomLeft = juce::Random::getSystemRandom().nextFloat() * vol * 0.7f;
            float randomRight = juce::Random::getSystemRandom().nextFloat() * vol * 0.7f;
            levelMeter.setLevels(randomLeft, randomRight);
        } else {
            levelMeter.setLevels(0.0f, 0.0f);
        }
    }
    
private:
    juce::Colour getChannelColor() {
        juce::Random random(channelNumber);
        return juce::Colour::fromHSV(random.nextFloat(), 0.5f, 0.6f, 1.0f);
    }
    
    juce::String channelName;
    int channelNumber;
    
    juce::Label nameLabel;
    juce::Slider volumeFader;
    juce::Slider panKnob;
    juce::Label panLabel;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton recordButton;
    LevelMeter levelMeter;
    juce::OwnedArray<juce::TextButton> fxButtons;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};

//==============================================================================
// Mixer completo con múltiples canales
//==============================================================================
class FunctionalMixer : public juce::Component,
                        private juce::Timer {
public:
    FunctionalMixer() {
        // Add channel button
        addChannelButton.setButtonText("+ Add Channel");
        addChannelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4CAF50));
        addChannelButton.onClick = [this] {
            addChannel("Channel " + juce::String(channels.size()));
        };
        addAndMakeVisible(addChannelButton);
        
        // Viewport for scrolling
        viewport.setViewedComponent(&channelContainer, false);
        viewport.setScrollBarsShown(false, true);
        addAndMakeVisible(viewport);
        
        // Add default channels
        addChannel("Master");
        addChannel("Drums");
        addChannel("Bass");
        addChannel("Lead");
        addChannel("Pads");
        addChannel("FX");
        addChannel("Vocals");
        addChannel("Guitar");
        
        startTimer(100); // Update meters
    }
    
    ~FunctionalMixer() override {
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Title
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("MIXER", 10, 5, 100, 30, juce::Justification::centredLeft);
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        // Title bar
        area.removeFromTop(40);
        
        // Add button
        addChannelButton.setBounds(5, 45, 100, 25);
        area.removeFromTop(35);
        
        // Viewport
        viewport.setBounds(area);
        
        // Layout channels horizontally
        int channelWidth = 100;
        channelContainer.setBounds(0, 0, channels.size() * channelWidth, area.getHeight());
        
        for (int i = 0; i < channels.size(); ++i) {
            channels[i]->setBounds(i * channelWidth, 0, channelWidth - 2, area.getHeight());
        }
    }
    
    void addChannel(const juce::String& name) {
        auto* channel = new MixerChannel(name, channels.size());
        channels.add(channel);
        channelContainer.addAndMakeVisible(channel);
        resized();
    }
    
    int getNumChannels() const { return channels.size(); }
    
private:
    void timerCallback() override {
        // Update level meters with simulated audio
        for (auto* channel : channels) {
            float left = juce::Random::getSystemRandom().nextFloat();
            float right = juce::Random::getSystemRandom().nextFloat();
            channel->updateLevels(left, right);
        }
    }
    
    juce::TextButton addChannelButton;
    juce::Viewport viewport;
    juce::Component channelContainer;
    juce::OwnedArray<MixerChannel> channels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionalMixer)
};

} // namespace GUI
} // namespace OmegaStudio
