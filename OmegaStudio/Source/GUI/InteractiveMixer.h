#pragma once

#include <JuceHeader.h>
#include <array>

namespace OmegaStudio::GUI {

/**
 * @brief Interactive Mixer Channel Strip - Un canal individual con fader, pan, mute, solo
 */
class InteractiveMixerChannelStrip : public juce::Component
{
public:
    InteractiveMixerChannelStrip(int channelNumber) 
        : channelNum_(channelNumber)
    {
        // Volume fader
        volumeSlider_.setSliderStyle(juce::Slider::LinearVertical);
        volumeSlider_.setRange(0.0, 1.0, 0.01);
        volumeSlider_.setValue(0.8);
        volumeSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volumeSlider_.setColour(juce::Slider::trackColourId, juce::Colour(0xff00ff00));
        volumeSlider_.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
        volumeSlider_.onValueChange = [this]() {
            if (onVolumeChanged)
                onVolumeChanged(channelNum_, volumeSlider_.getValue());
        };
        addAndMakeVisible(volumeSlider_);
        
        // Pan knob
        panSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider_.setRange(-1.0, 1.0, 0.01);
        panSlider_.setValue(0.0);
        panSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panSlider_.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ffff));
        panSlider_.onValueChange = [this]() {
            if (onPanChanged)
                onPanChanged(channelNum_, panSlider_.getValue());
        };
        addAndMakeVisible(panSlider_);
        
        // Mute button
        muteButton_.setButtonText("M");
        muteButton_.setClickingTogglesState(true);
        muteButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff0000));
        muteButton_.onClick = [this]() {
            if (onMuteChanged)
                onMuteChanged(channelNum_, muteButton_.getToggleState());
        };
        addAndMakeVisible(muteButton_);
        
        // Solo button
        soloButton_.setButtonText("S");
        soloButton_.setClickingTogglesState(true);
        soloButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffff00));
        soloButton_.onClick = [this]() {
            if (onSoloChanged)
                onSoloChanged(channelNum_, soloButton_.getToggleState());
        };
        addAndMakeVisible(soloButton_);
        
        // Record button
        recordButton_.setButtonText("R");
        recordButton_.setClickingTogglesState(true);
        recordButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff00ff));
        addAndMakeVisible(recordButton_);
        
        setSize(80, 400);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(bounds);
        
        // Border
        g.setColour(juce::Colour(0xff444444));
        g.drawRect(bounds);
        
        // Channel number at top
        auto numberArea = bounds.removeFromTop(30);
        g.setColour(channelColor_);
        g.fillRect(numberArea);
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText(juce::String(channelNum_ + 1), numberArea, 
                  juce::Justification::centred);
        
        // Peak meter background
        auto meterArea = bounds.removeFromRight(15).reduced(2, 100);
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(meterArea);
        
        // Peak meter
        float peakHeight = meterArea.getHeight() * peakLevel_;
        auto peakBar = meterArea.removeFromBottom(static_cast<int>(peakHeight));
        
        // Gradient from green to red
        juce::ColourGradient gradient(
            juce::Colour(0xff00ff00), 0, peakBar.getBottom(),
            juce::Colour(0xffff0000), 0, peakBar.getY(),
            false
        );
        g.setGradientFill(gradient);
        g.fillRect(peakBar);
        
        // Channel name at bottom
        auto nameArea = bounds.removeFromBottom(25);
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        g.drawText(channelName_, nameArea, juce::Justification::centred, true);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(30); // Skip number area
        
        // Buttons at top
        auto buttonArea = bounds.removeFromTop(80);
        muteButton_.setBounds(buttonArea.removeFromTop(25).reduced(10, 2));
        soloButton_.setBounds(buttonArea.removeFromTop(25).reduced(10, 2));
        recordButton_.setBounds(buttonArea.removeFromTop(25).reduced(10, 2));
        
        bounds.removeFromBottom(25); // Skip name area
        bounds.removeFromRight(15);  // Skip meter
        
        // Pan knob
        panSlider_.setBounds(bounds.removeFromTop(60).reduced(10));
        
        // Volume fader takes remaining space
        volumeSlider_.setBounds(bounds.reduced(15, 10));
    }
    
    void setPeakLevel(float level)
    {
        peakLevel_ = juce::jlimit(0.0f, 1.0f, level);
        repaint();
    }
    
    void setChannelName(const juce::String& name)
    {
        channelName_ = name;
        repaint();
    }
    
    void setChannelColor(const juce::Colour& color)
    {
        channelColor_ = color;
        repaint();
    }
    
    float getVolume() const { return volumeSlider_.getValue(); }
    float getPan() const { return panSlider_.getValue(); }
    bool isMuted() const { return muteButton_.getToggleState(); }
    bool isSoloed() const { return soloButton_.getToggleState(); }
    
    std::function<void(int, float)> onVolumeChanged;
    std::function<void(int, float)> onPanChanged;
    std::function<void(int, bool)> onMuteChanged;
    std::function<void(int, bool)> onSoloChanged;

private:
    int channelNum_;
    juce::String channelName_{"Channel"};
    juce::Colour channelColor_{0xff888888};
    float peakLevel_{0.0f};
    
    juce::Slider volumeSlider_;
    juce::Slider panSlider_;
    juce::TextButton muteButton_;
    juce::TextButton soloButton_;
    juce::TextButton recordButton_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveMixerChannelStrip)
};

/**
 * @brief Interactive Mixer - Mezclador completo con múltiples canales
 */
class InteractiveMixer : public juce::Component,
                         private juce::Timer
{
public:
    InteractiveMixer()
    {
        // Create 16 channels for now (can expand to 128)
        for (int i = 0; i < 16; ++i)
        {
            auto* channel = new InteractiveMixerChannelStrip(i);
            channel->setChannelName("Ch " + juce::String(i + 1));
            
            // Assign colors
            juce::Colour channelColor = juce::Colour::fromHSV(
                i / 16.0f, 0.7f, 0.8f, 1.0f
            );
            channel->setChannelColor(channelColor);
            
            // Callbacks
            channel->onVolumeChanged = [this](int ch, float vol) {
                DBG("Channel " << ch << " Volume: " << vol);
            };
            
            channel->onPanChanged = [this](int ch, float pan) {
                DBG("Channel " << ch << " Pan: " << pan);
            };
            
            channel->onMuteChanged = [this](int ch, bool muted) {
                DBG("Channel " << ch << " Mute: " << muted);
            };
            
            channel->onSoloChanged = [this](int ch, bool soloed) {
                DBG("Channel " << ch << " Solo: " << soloed);
                handleSolo(ch, soloed);
            };
            
            channels_.add(channel);
            addAndMakeVisible(channel);
        }
        
        // Start peak meter animation
        startTimerHz(30); // 30 FPS for smooth meters
        
        setSize(1280, 500);
    }
    
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Header
        auto header = getLocalBounds().removeFromTop(40);
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillRect(header);
        
        // Title
        g.setColour(juce::Colour(0xffff8c00));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("🎚️ MIXER - 128 CHANNELS", header.reduced(10, 0), 
                  juce::Justification::centredLeft);
        
        // Master volume display
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawText("Master: " + juce::String(masterVolume_, 2) + " dB", 
                  header.removeFromRight(150).reduced(5, 0),
                  juce::Justification::centredRight);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Skip header
        
        int channelWidth = 80;
        int x = 10;
        
        for (auto* channel : channels_)
        {
            channel->setBounds(x, bounds.getY(), channelWidth, bounds.getHeight());
            x += channelWidth + 5;
        }
    }
    
    void setMasterVolume(float volumeDB)
    {
        masterVolume_ = volumeDB;
        repaint();
    }

private:
    void timerCallback() override
    {
        // Simulate peak meters (in real implementation, get from audio engine)
        for (auto* channel : channels_)
        {
            float randomPeak = juce::Random::getSystemRandom().nextFloat() * 0.8f;
            channel->setPeakLevel(randomPeak);
        }
    }
    
    void handleSolo(int channelIndex, bool soloed)
    {
        if (soloed)
        {
            // Mute all other channels
            for (int i = 0; i < channels_.size(); ++i)
            {
                if (i != channelIndex && !channels_[i]->isSoloed())
                {
                    // Visually indicate muted by solo
                }
            }
        }
    }
    
    juce::OwnedArray<InteractiveMixerChannelStrip> channels_;
    float masterVolume_{0.0f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveMixer)
};

} // namespace OmegaStudio::GUI
