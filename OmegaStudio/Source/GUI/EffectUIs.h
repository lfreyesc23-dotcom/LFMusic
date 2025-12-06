#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Reverb Effect UI - Visual room parameters
 */
class ReverbUI : public juce::Component
{
public:
    ReverbUI()
    {
        setupComponents();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Title
        bounds.removeFromTop(30);
        
        // Room visualization
        roomDisplay.setBounds(bounds.removeFromTop(150));
        
        bounds.removeFromTop(10);
        
        // Controls (3 columns)
        auto row1 = bounds.removeFromTop(80);
        
        auto col1 = row1.removeFromLeft(row1.getWidth() / 3).reduced(5);
        roomSizeLabel.setBounds(col1.removeFromTop(20));
        roomSizeSlider.setBounds(col1);
        
        auto col2 = row1.removeFromLeft(row1.getWidth() / 2).reduced(5);
        dampingLabel.setBounds(col2.removeFromTop(20));
        dampingSlider.setBounds(col2);
        
        preDelayLabel.setBounds(row1.removeFromTop(20));
        preDelaySlider.setBounds(row1.reduced(5));
        
        auto row2 = bounds.removeFromTop(80);
        
        auto col3 = row2.removeFromLeft(row2.getWidth() / 3).reduced(5);
        wetDryLabel.setBounds(col3.removeFromTop(20));
        wetDrySlider.setBounds(col3);
        
        auto col4 = row2.removeFromLeft(row2.getWidth() / 2).reduced(5);
        widthLabel.setBounds(col4.removeFromTop(20));
        widthSlider.setBounds(col4);
        
        freezeButton.setBounds(row2.reduced(5));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("REVERB", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void setupComponents()
    {
        // Room display
        addAndMakeVisible(roomDisplay);
        
        // Room size
        addAndMakeVisible(roomSizeLabel);
        roomSizeLabel.setText("Room Size", juce::dontSendNotification);
        roomSizeLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(roomSizeSlider);
        roomSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        roomSizeSlider.setRange(0.0, 1.0, 0.01);
        roomSizeSlider.setValue(0.5);
        roomSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Damping
        addAndMakeVisible(dampingLabel);
        dampingLabel.setText("Damping", juce::dontSendNotification);
        dampingLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(dampingSlider);
        dampingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        dampingSlider.setRange(0.0, 1.0, 0.01);
        dampingSlider.setValue(0.5);
        dampingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Pre-delay
        addAndMakeVisible(preDelayLabel);
        preDelayLabel.setText("Pre-Delay", juce::dontSendNotification);
        preDelayLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(preDelaySlider);
        preDelaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        preDelaySlider.setRange(0.0, 100.0, 1.0);
        preDelaySlider.setValue(0.0);
        preDelaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        preDelaySlider.setTextValueSuffix(" ms");
        
        // Wet/Dry
        addAndMakeVisible(wetDryLabel);
        wetDryLabel.setText("Wet/Dry", juce::dontSendNotification);
        wetDryLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(wetDrySlider);
        wetDrySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        wetDrySlider.setRange(0.0, 1.0, 0.01);
        wetDrySlider.setValue(0.3);
        wetDrySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Width
        addAndMakeVisible(widthLabel);
        widthLabel.setText("Width", juce::dontSendNotification);
        widthLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(widthSlider);
        widthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        widthSlider.setRange(0.0, 1.0, 0.01);
        widthSlider.setValue(1.0);
        widthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Freeze
        addAndMakeVisible(freezeButton);
        freezeButton.setButtonText("FREEZE");
        freezeButton.setClickingTogglesState(true);
        freezeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan);
    }

    // Room visualization component
    class RoomDisplay : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(getLocalBounds());
            
            // Draw room visualization
            auto bounds = getLocalBounds().toFloat().reduced(20.0f);
            
            g.setColour(juce::Colours::orange.withAlpha(0.3f));
            g.fillRect(bounds);
            
            g.setColour(juce::Colours::orange);
            g.drawRect(bounds, 2.0f);
            
            // Draw sound source
            g.setColour(juce::Colours::yellow);
            g.fillEllipse(bounds.getCentreX() - 10.0f, bounds.getCentreY() - 10.0f, 20.0f, 20.0f);
            
            // Draw reverb rays
            for (int i = 0; i < 8; ++i)
            {
                float angle = i * juce::MathConstants<float>::pi / 4.0f;
                float x = bounds.getCentreX() + std::cos(angle) * 40.0f;
                float y = bounds.getCentreY() + std::sin(angle) * 40.0f;
                
                g.setColour(juce::Colours::cyan.withAlpha(0.5f));
                g.drawLine(bounds.getCentreX(), bounds.getCentreY(), x, y, 2.0f);
            }
        }
    };

    RoomDisplay roomDisplay;
    juce::Label roomSizeLabel, dampingLabel, preDelayLabel, wetDryLabel, widthLabel;
    juce::Slider roomSizeSlider, dampingSlider, preDelaySlider, wetDrySlider, widthSlider;
    juce::TextButton freezeButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbUI)
};

//==============================================================================
/**
 * Delay Effect UI
 */
class DelayUI : public juce::Component
{
public:
    DelayUI()
    {
        setupComponents();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        bounds.removeFromTop(30);
        
        auto row1 = bounds.removeFromTop(80);
        timeLabel.setBounds(row1.removeFromTop(20));
        timeSlider.setBounds(row1.reduced(5));
        
        auto row2 = bounds.removeFromTop(80);
        
        auto col1 = row2.removeFromLeft(row2.getWidth() / 3).reduced(5);
        feedbackLabel.setBounds(col1.removeFromTop(20));
        feedbackSlider.setBounds(col1);
        
        auto col2 = row2.removeFromLeft(row2.getWidth() / 2).reduced(5);
        mixLabel.setBounds(col2.removeFromTop(20));
        mixSlider.setBounds(col2);
        
        filterLabel.setBounds(row2.removeFromTop(20));
        filterSlider.setBounds(row2.reduced(5));
        
        auto row3 = bounds.removeFromTop(40);
        syncButton.setBounds(row3.removeFromLeft(row3.getWidth() / 3).reduced(5));
        pingPongButton.setBounds(row3.removeFromLeft(row3.getWidth() / 2).reduced(5));
        tapTempoButton.setBounds(row3.reduced(5));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("DELAY", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void setupComponents()
    {
        addAndMakeVisible(timeLabel);
        timeLabel.setText("Time", juce::dontSendNotification);
        timeLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(timeSlider);
        timeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        timeSlider.setRange(1.0, 2000.0, 1.0);
        timeSlider.setValue(500.0);
        timeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
        timeSlider.setTextValueSuffix(" ms");
        
        addAndMakeVisible(feedbackLabel);
        feedbackLabel.setText("Feedback", juce::dontSendNotification);
        feedbackLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(feedbackSlider);
        feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        feedbackSlider.setRange(0.0, 0.95, 0.01);
        feedbackSlider.setValue(0.4);
        feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        addAndMakeVisible(mixLabel);
        mixLabel.setText("Mix", juce::dontSendNotification);
        mixLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(mixSlider);
        mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        mixSlider.setRange(0.0, 1.0, 0.01);
        mixSlider.setValue(0.3);
        mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        addAndMakeVisible(filterLabel);
        filterLabel.setText("Filter", juce::dontSendNotification);
        filterLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(filterSlider);
        filterSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        filterSlider.setRange(20.0, 20000.0, 1.0);
        filterSlider.setValue(20000.0);
        filterSlider.setSkewFactorFromMidPoint(1000.0);
        filterSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        addAndMakeVisible(syncButton);
        syncButton.setButtonText("Sync to BPM");
        syncButton.setClickingTogglesState(true);
        
        addAndMakeVisible(pingPongButton);
        pingPongButton.setButtonText("Ping-Pong");
        pingPongButton.setClickingTogglesState(true);
        
        addAndMakeVisible(tapTempoButton);
        tapTempoButton.setButtonText("Tap Tempo");
    }

    juce::Label timeLabel, feedbackLabel, mixLabel, filterLabel;
    juce::Slider timeSlider, feedbackSlider, mixSlider, filterSlider;
    juce::TextButton syncButton, pingPongButton, tapTempoButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayUI)
};

//==============================================================================
/**
 * Parametric EQ UI with Interactive Frequency Curve
 */
class ParametricEQUI : public juce::Component
{
public:
    ParametricEQUI()
    {
        setupComponents();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        bounds.removeFromTop(30);
        
        // Frequency response display
        frequencyDisplay.setBounds(bounds.removeFromTop(200));
        
        bounds.removeFromTop(10);
        
        // 3 band controls
        for (int i = 0; i < 3; ++i)
        {
            auto bandArea = bounds.removeFromLeft(bounds.getWidth() / (3 - i)).reduced(5);
            
            auto col1 = bandArea.removeFromLeft(bandArea.getWidth() / 3);
            freqLabels[i].setBounds(col1.removeFromTop(20));
            freqSliders[i].setBounds(col1);
            
            auto col2 = bandArea.removeFromLeft(bandArea.getWidth() / 2);
            gainLabels[i].setBounds(col2.removeFromTop(20));
            gainSliders[i].setBounds(col2);
            
            qLabels[i].setBounds(bandArea.removeFromTop(20));
            qSliders[i].setBounds(bandArea);
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("PARAMETRIC EQ", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void setupComponents()
    {
        // Frequency display
        addAndMakeVisible(frequencyDisplay);
        
        // 3 bands
        juce::Colour bandColors[] = { juce::Colours::red, juce::Colours::green, juce::Colours::blue };
        
        for (int i = 0; i < 3; ++i)
        {
            addAndMakeVisible(freqLabels[i]);
            freqLabels[i].setText("Freq " + juce::String(i + 1), juce::dontSendNotification);
            freqLabels[i].setJustificationType(juce::Justification::centred);
            freqLabels[i].setColour(juce::Label::textColourId, bandColors[i]);
            
            addAndMakeVisible(freqSliders[i]);
            freqSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            freqSliders[i].setRange(20.0, 20000.0, 1.0);
            freqSliders[i].setValue(100.0 * (i + 1));
            freqSliders[i].setSkewFactorFromMidPoint(1000.0);
            freqSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
            
            addAndMakeVisible(gainLabels[i]);
            gainLabels[i].setText("Gain " + juce::String(i + 1), juce::dontSendNotification);
            gainLabels[i].setJustificationType(juce::Justification::centred);
            
            addAndMakeVisible(gainSliders[i]);
            gainSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            gainSliders[i].setRange(-24.0, 24.0, 0.1);
            gainSliders[i].setValue(0.0);
            gainSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
            gainSliders[i].setTextValueSuffix(" dB");
            
            addAndMakeVisible(qLabels[i]);
            qLabels[i].setText("Q " + juce::String(i + 1), juce::dontSendNotification);
            qLabels[i].setJustificationType(juce::Justification::centred);
            
            addAndMakeVisible(qSliders[i]);
            qSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            qSliders[i].setRange(0.1, 10.0, 0.1);
            qSliders[i].setValue(1.0);
            qSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        }
    }

    class FrequencyDisplay : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(getLocalBounds());
            
            // Draw frequency grid
            g.setColour(juce::Colours::grey.withAlpha(0.2f));
            for (int i = 0; i < 10; ++i)
            {
                float y = getHeight() * (i / 10.0f);
                g.drawLine(0.0f, y, static_cast<float>(getWidth()), y);
            }
            
            // Draw flat response line
            g.setColour(juce::Colours::orange);
            g.drawLine(0.0f, getHeight() * 0.5f, static_cast<float>(getWidth()), getHeight() * 0.5f, 2.0f);
        }
    };

    FrequencyDisplay frequencyDisplay;
    juce::Label freqLabels[3], gainLabels[3], qLabels[3];
    juce::Slider freqSliders[3], gainSliders[3], qSliders[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQUI)
};

//==============================================================================
/**
 * Compressor UI with Gain Reduction Meter
 */
class CompressorUI : public juce::Component,
                      private juce::Timer
{
public:
    CompressorUI()
    {
        setupComponents();
        startTimerHz(30);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        bounds.removeFromTop(30);
        
        // GR meter
        grMeter.setBounds(bounds.removeFromLeft(60));
        
        bounds.removeFromLeft(10);
        
        // Controls
        auto row1 = bounds.removeFromTop(80);
        
        auto col1 = row1.removeFromLeft(row1.getWidth() / 4).reduced(5);
        thresholdLabel.setBounds(col1.removeFromTop(20));
        thresholdSlider.setBounds(col1);
        
        auto col2 = row1.removeFromLeft(row1.getWidth() / 3).reduced(5);
        ratioLabel.setBounds(col2.removeFromTop(20));
        ratioSlider.setBounds(col2);
        
        auto col3 = row1.removeFromLeft(row1.getWidth() / 2).reduced(5);
        attackLabel.setBounds(col3.removeFromTop(20));
        attackSlider.setBounds(col3);
        
        releaseLabel.setBounds(row1.removeFromTop(20));
        releaseSlider.setBounds(row1.reduced(5));
        
        auto row2 = bounds.removeFromTop(80);
        
        auto col4 = row2.removeFromLeft(row2.getWidth() / 2).reduced(5);
        kneeLabel.setBounds(col4.removeFromTop(20));
        kneeSlider.setBounds(col4);
        
        makeupLabel.setBounds(row2.removeFromTop(20));
        makeupSlider.setBounds(row2.reduced(5));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("COMPRESSOR", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        grMeter.repaint();
    }

    void setupComponents()
    {
        addAndMakeVisible(grMeter);
        
        addAndMakeVisible(thresholdLabel);
        thresholdLabel.setText("Threshold", juce::dontSendNotification);
        thresholdLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(thresholdSlider);
        thresholdSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        thresholdSlider.setRange(-60.0, 0.0, 0.1);
        thresholdSlider.setValue(-20.0);
        thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        thresholdSlider.setTextValueSuffix(" dB");
        
        addAndMakeVisible(ratioLabel);
        ratioLabel.setText("Ratio", juce::dontSendNotification);
        ratioLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(ratioSlider);
        ratioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ratioSlider.setRange(1.0, 20.0, 0.1);
        ratioSlider.setValue(4.0);
        ratioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        ratioSlider.setTextValueSuffix(":1");
        
        addAndMakeVisible(attackLabel);
        attackLabel.setText("Attack", juce::dontSendNotification);
        attackLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(attackSlider);
        attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        attackSlider.setRange(0.1, 100.0, 0.1);
        attackSlider.setValue(10.0);
        attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        attackSlider.setTextValueSuffix(" ms");
        
        addAndMakeVisible(releaseLabel);
        releaseLabel.setText("Release", juce::dontSendNotification);
        releaseLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(releaseSlider);
        releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        releaseSlider.setRange(10.0, 1000.0, 1.0);
        releaseSlider.setValue(100.0);
        releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        releaseSlider.setTextValueSuffix(" ms");
        
        addAndMakeVisible(kneeLabel);
        kneeLabel.setText("Knee", juce::dontSendNotification);
        kneeLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(kneeSlider);
        kneeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        kneeSlider.setRange(0.0, 1.0, 0.01);
        kneeSlider.setValue(0.5);
        kneeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        addAndMakeVisible(makeupLabel);
        makeupLabel.setText("Makeup", juce::dontSendNotification);
        makeupLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(makeupSlider);
        makeupSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        makeupSlider.setRange(0.0, 24.0, 0.1);
        makeupSlider.setValue(0.0);
        makeupSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        makeupSlider.setTextValueSuffix(" dB");
    }

    class GRMeter : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(getLocalBounds());
            
            // Draw gain reduction
            float grAmount = 0.3f; // Mock value
            float grHeight = getHeight() * grAmount;
            
            g.setColour(juce::Colours::red);
            g.fillRect(0.0f, getHeight() - grHeight, static_cast<float>(getWidth()), grHeight);
            
            g.setColour(juce::Colours::white);
            g.setFont(10.0f);
            g.drawText("GR", getLocalBounds(), juce::Justification::centredTop);
        }
    };

    GRMeter grMeter;
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, kneeLabel, makeupLabel;
    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider, kneeSlider, makeupSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorUI)
};

//==============================================================================
/**
 * Limiter UI
 */
class LimiterUI : public juce::Component,
                   private juce::Timer
{
public:
    LimiterUI()
    {
        setupComponents();
        startTimerHz(30);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        bounds.removeFromTop(30);
        
        // GR history display
        grHistory.setBounds(bounds.removeFromTop(120));
        
        bounds.removeFromTop(10);
        
        // Controls
        auto row = bounds.removeFromTop(80);
        
        auto col1 = row.removeFromLeft(row.getWidth() / 3).reduced(5);
        ceilingLabel.setBounds(col1.removeFromTop(20));
        ceilingSlider.setBounds(col1);
        
        auto col2 = row.removeFromLeft(row.getWidth() / 2).reduced(5);
        releaseLabel.setBounds(col2.removeFromTop(20));
        releaseSlider.setBounds(col2);
        
        lookaheadLabel.setBounds(row.removeFromTop(20));
        lookaheadSlider.setBounds(row.reduced(5));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText("LIMITER", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        grHistory.repaint();
    }

    void setupComponents()
    {
        addAndMakeVisible(grHistory);
        
        addAndMakeVisible(ceilingLabel);
        ceilingLabel.setText("Ceiling", juce::dontSendNotification);
        ceilingLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(ceilingSlider);
        ceilingSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ceilingSlider.setRange(-20.0, 0.0, 0.1);
        ceilingSlider.setValue(-0.3);
        ceilingSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        ceilingSlider.setTextValueSuffix(" dB");
        
        addAndMakeVisible(releaseLabel);
        releaseLabel.setText("Release", juce::dontSendNotification);
        releaseLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(releaseSlider);
        releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        releaseSlider.setRange(1.0, 1000.0, 1.0);
        releaseSlider.setValue(100.0);
        releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        releaseSlider.setTextValueSuffix(" ms");
        
        addAndMakeVisible(lookaheadLabel);
        lookaheadLabel.setText("Lookahead", juce::dontSendNotification);
        lookaheadLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(lookaheadSlider);
        lookaheadSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        lookaheadSlider.setRange(0.0, 10.0, 0.1);
        lookaheadSlider.setValue(5.0);
        lookaheadSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        lookaheadSlider.setTextValueSuffix(" ms");
    }

    class GRHistory : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override
        {
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(getLocalBounds());
            
            // Draw mock gain reduction history
            juce::Path path;
            path.startNewSubPath(0.0f, static_cast<float>(getHeight()));
            
            for (int i = 0; i < getWidth(); ++i)
            {
                float gr = std::sin(i * 0.1f) * 0.3f + 0.3f;
                float y = getHeight() * (1.0f - gr);
                path.lineTo(static_cast<float>(i), y);
            }
            
            path.lineTo(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
            path.closeSubPath();
            
            g.setColour(juce::Colours::red.withAlpha(0.5f));
            g.fillPath(path);
            
            g.setColour(juce::Colours::red);
            g.strokePath(path, juce::PathStrokeType(2.0f));
        }
    };

    GRHistory grHistory;
    juce::Label ceilingLabel, releaseLabel, lookaheadLabel;
    juce::Slider ceilingSlider, releaseSlider, lookaheadSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterUI)
};

} // namespace GUI
} // namespace OmegaStudio
