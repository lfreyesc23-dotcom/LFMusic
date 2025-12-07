#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>

namespace OmegaStudio::GUI {

/**
 * @brief Step Button - Botón individual del step sequencer
 */
class StepButton : public juce::Component
{
public:
    StepButton(int stepIndex) : stepIndex_(stepIndex) {}
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        
        // Color based on state
        juce::Colour color;
        if (isCurrentStep_)
            color = juce::Colour(0xffff8c00); // Orange when playing
        else if (isActive_)
            color = juce::Colour(0xff00ff00); // Green when active
        else
            color = juce::Colour(0xff333333); // Dark grey when inactive
        
        // Draw step button
        g.setColour(color);
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Border
        g.setColour(color.brighter(0.3f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.5f);
        
        // Glow effect when active
        if (isActive_ || isCurrentStep_)
        {
            g.setColour(color.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds.expanded(2.0f), 4.0f);
        }
    }
    
    void mouseDown(const juce::MouseEvent&) override
    {
        isActive_ = !isActive_;
        repaint();
        
        if (onToggle)
            onToggle(stepIndex_, isActive_);
    }
    
    void setActive(bool active)
    {
        isActive_ = active;
        repaint();
    }
    
    void setCurrentStep(bool isCurrent)
    {
        isCurrentStep_ = isCurrent;
        repaint();
    }
    
    bool isActive() const { return isActive_; }
    
    std::function<void(int, bool)> onToggle;

private:
    int stepIndex_;
    bool isActive_{false};
    bool isCurrentStep_{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepButton)
};

/**
 * @brief Channel Row - Una fila del channel rack con 16 steps
 */
class ChannelRow : public juce::Component
{
public:
    ChannelRow(const juce::String& name, const juce::Colour& color)
        : channelName_(name), channelColor_(color)
    {
        // Create 16 step buttons
        for (int i = 0; i < 16; ++i)
        {
            auto* step = new StepButton(i);
            step->onToggle = [this, i](int index, bool active) {
                if (onStepToggled)
                    onStepToggled(channelIndex_, i, active);
            };
            steps_.add(step);
            addAndMakeVisible(step);
        }
        
        setSize(900, 40);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Channel name area (left side)
        auto nameArea = bounds.removeFromLeft(150);
        
        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(nameArea);
        
        // Channel color indicator
        g.setColour(channelColor_);
        g.fillRect(nameArea.removeFromLeft(5));
        
        // Channel name
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::plain));
        g.drawText(channelName_, nameArea.reduced(10, 0), 
                  juce::Justification::centredLeft, true);
        
        // Steps background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(bounds);
        
        // Draw beat markers (every 4 steps)
        g.setColour(juce::Colour(0xff444444));
        for (int i = 1; i < 4; ++i)
        {
            int x = bounds.getX() + (bounds.getWidth() * i / 4);
            g.drawVerticalLine(x, static_cast<float>(bounds.getY()), 
                             static_cast<float>(bounds.getBottom()));
        }
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromLeft(150); // Skip name area
        
        int stepWidth = bounds.getWidth() / 16;
        for (int i = 0; i < 16; ++i)
        {
            steps_[i]->setBounds(bounds.removeFromLeft(stepWidth).reduced(1));
        }
    }
    
    void setCurrentStep(int step)
    {
        for (int i = 0; i < 16; ++i)
        {
            steps_[i]->setCurrentStep(i == step);
        }
    }
    
    void setStepActive(int step, bool active)
    {
        if (step >= 0 && step < 16)
            steps_[step]->setActive(active);
    }
    
    bool isStepActive(int step) const
    {
        if (step >= 0 && step < 16)
            return steps_[step]->isActive();
        return false;
    }
    
    void setChannelIndex(int index) { channelIndex_ = index; }
    
    std::function<void(int, int, bool)> onStepToggled;

private:
    juce::String channelName_;
    juce::Colour channelColor_;
    int channelIndex_{0};
    juce::OwnedArray<StepButton> steps_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRow)
};

/**
 * @brief Interactive Channel Rack - Secuenciador de pasos completo
 */
class InteractiveChannelRack : public juce::Component,
                               private juce::Timer
{
public:
    InteractiveChannelRack()
    {
        // Create default channels
        addChannel("Kick", juce::Colour(0xffff0000));
        addChannel("Snare", juce::Colour(0xff00ff00));
        addChannel("Hi-Hat", juce::Colour(0xff00ffff));
        addChannel("Clap", juce::Colour(0xffff00ff));
        addChannel("Bass", juce::Colour(0xffffff00));
        addChannel("Lead", juce::Colour(0xffff8c00));
        addChannel("Pad", juce::Colour(0xff8c00ff));
        addChannel("FX", juce::Colour(0xff00ff8c));
        
        // Default pattern (4/4 kick)
        channels_[0]->setStepActive(0, true);
        channels_[0]->setStepActive(4, true);
        channels_[0]->setStepActive(8, true);
        channels_[0]->setStepActive(12, true);
        
        // Default hi-hat pattern
        for (int i = 0; i < 16; i += 2)
            channels_[2]->setStepActive(i, true);
        
        setSize(900, 400);
    }
    
    void addChannel(const juce::String& name, const juce::Colour& color)
    {
        auto* channel = new ChannelRow(name, color);
        channel->setChannelIndex(channels_.size());
        channel->onStepToggled = [this](int ch, int step, bool active) {
            DBG("Channel " << ch << " Step " << step << " = " << active);
        };
        channels_.add(channel);
        addAndMakeVisible(channel);
        resized();
    }
    
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Header
        auto header = getLocalBounds().removeFromTop(35);
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillRect(header);
        
        // Title
        g.setColour(juce::Colour(0xffff8c00));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("CHANNEL RACK", header.reduced(10, 0), 
                  juce::Justification::centredLeft);
        
        // Play button indicator
        if (isPlaying_)
        {
            g.setColour(juce::Colour(0xff00ff00));
            g.fillEllipse(header.removeFromRight(30).reduced(8).toFloat());
        }
        
        // BPM display
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::plain));
        g.drawText(juce::String(bpm_) + " BPM", 
                  header.removeFromRight(100).reduced(5, 0),
                  juce::Justification::centredRight);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(35); // Skip header
        
        int rowHeight = 45;
        for (auto* channel : channels_)
        {
            channel->setBounds(bounds.removeFromTop(rowHeight));
        }
    }
    
    // Playback control
    void play()
    {
        if (!isPlaying_)
        {
            isPlaying_ = true;
            currentStep_ = 0;
            startTimer(calculateStepInterval());
            repaint();
        }
    }
    
    void stop()
    {
        if (isPlaying_)
        {
            isPlaying_ = false;
            stopTimer();
            
            // Clear current step indicators
            for (auto* channel : channels_)
                channel->setCurrentStep(-1);
            
            repaint();
        }
    }
    
    void setBPM(double bpm)
    {
        bpm_ = bpm;
        if (isPlaying_)
        {
            stopTimer();
            startTimer(calculateStepInterval());
        }
        repaint();
    }
    
    bool isPlaying() const { return isPlaying_; }

private:
    void timerCallback() override
    {
        // Update current step
        for (auto* channel : channels_)
            channel->setCurrentStep(currentStep_);
        
        // Trigger active steps (aquí iría el MIDI output)
        for (int ch = 0; ch < channels_.size(); ++ch)
        {
            if (channels_[ch]->isStepActive(currentStep_))
            {
                DBG("🎵 Trigger: Channel " << ch << " Step " << currentStep_);
                // TODO: Send MIDI note on
            }
        }
        
        // Advance to next step
        currentStep_ = (currentStep_ + 1) % 16;
    }
    
    int calculateStepInterval() const
    {
        // Calculate milliseconds per 16th note
        double beatsPerSecond = bpm_ / 60.0;
        double sixteenthsPerSecond = beatsPerSecond * 4.0; // 4 sixteenth notes per beat
        return static_cast<int>(1000.0 / sixteenthsPerSecond);
    }
    
    juce::OwnedArray<ChannelRow> channels_;
    bool isPlaying_{false};
    int currentStep_{0};
    double bpm_{120.0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveChannelRack)
};

} // namespace OmegaStudio::GUI
