#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * FM Operator Display - Single operator with envelope
 */
class FMOperatorComponent : public juce::Component
{
public:
    FMOperatorComponent(int opIndex)
        : operatorIndex(opIndex)
    {
        setupControls();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(5);
        
        // Title
        titleLabel.setBounds(bounds.removeFromTop(20));
        
        // Waveform display
        waveformArea = bounds.removeFromTop(60);
        
        // Controls
        auto controlRow1 = bounds.removeFromTop(60);
        ratioSlider.setBounds(controlRow1.removeFromLeft(controlRow1.getWidth() / 3).reduced(2));
        detuneSlider.setBounds(controlRow1.removeFromLeft(controlRow1.getWidth() / 2).reduced(2));
        levelSlider.setBounds(controlRow1.reduced(2));
        
        auto controlRow2 = bounds.removeFromTop(60);
        attackSlider.setBounds(controlRow2.removeFromLeft(controlRow2.getWidth() / 4).reduced(2));
        decaySlider.setBounds(controlRow2.removeFromLeft(controlRow2.getWidth() / 3).reduced(2));
        sustainSlider.setBounds(controlRow2.removeFromLeft(controlRow2.getWidth() / 2).reduced(2));
        releaseSlider.setBounds(controlRow2.reduced(2));
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(isActive ? juce::Colour(0xff2a2a3a) : juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 5.0f);
        
        // Border
        g.setColour(isActive ? juce::Colours::orange : juce::Colours::darkgrey);
        g.drawRoundedRectangle(bounds, 5.0f, 2.0f);
        
        // Draw operator waveform
        drawOperatorWaveform(g, waveformArea.toFloat());
    }

    void setActive(bool active)
    {
        isActive = active;
        repaint();
    }

    std::function<void(int opIndex, float ratio, float detune, float level)> onParamsChanged;

private:
    void setupControls()
    {
        // Title
        addAndMakeVisible(titleLabel);
        titleLabel.setText("OP " + juce::String(operatorIndex + 1), juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        titleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        
        // Ratio
        addAndMakeVisible(ratioSlider);
        ratioSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ratioSlider.setRange(0.5, 16.0, 0.5);
        ratioSlider.setValue(1.0);
        ratioSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        ratioSlider.setTextValueSuffix(" x");
        
        // Detune
        addAndMakeVisible(detuneSlider);
        detuneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        detuneSlider.setRange(-100.0, 100.0, 1.0);
        detuneSlider.setValue(0.0);
        detuneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        detuneSlider.setTextValueSuffix(" ¢");
        
        // Level
        addAndMakeVisible(levelSlider);
        levelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        levelSlider.setRange(0.0, 1.0, 0.01);
        levelSlider.setValue(0.8);
        levelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        
        // ADSR
        addAndMakeVisible(attackSlider);
        attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        attackSlider.setRange(0.001, 5.0, 0.001);
        attackSlider.setValue(0.01);
        attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
        attackSlider.setTextValueSuffix(" A");
        
        addAndMakeVisible(decaySlider);
        decaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        decaySlider.setRange(0.001, 5.0, 0.001);
        decaySlider.setValue(0.1);
        decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
        decaySlider.setTextValueSuffix(" D");
        
        addAndMakeVisible(sustainSlider);
        sustainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        sustainSlider.setRange(0.0, 1.0, 0.01);
        sustainSlider.setValue(0.7);
        sustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
        sustainSlider.setTextValueSuffix(" S");
        
        addAndMakeVisible(releaseSlider);
        releaseSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        releaseSlider.setRange(0.001, 5.0, 0.001);
        releaseSlider.setValue(0.5);
        releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
        releaseSlider.setTextValueSuffix(" R");
    }

    void drawOperatorWaveform(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(juce::Colours::black);
        g.fillRect(bounds);
        
        // Draw sine wave
        juce::Path wavePath;
        bool first = true;
        
        for (int i = 0; i < 100; ++i)
        {
            float x = bounds.getX() + (i / 100.0f) * bounds.getWidth();
            float phase = (i / 100.0f) * juce::MathConstants<float>::twoPi;
            float ratio = static_cast<float>(ratioSlider.getValue());
            float value = std::sin(phase * ratio);
            float y = bounds.getCentreY() - (value * bounds.getHeight() * 0.4f);
            
            if (first)
            {
                wavePath.startNewSubPath(x, y);
                first = false;
            }
            else
            {
                wavePath.lineTo(x, y);
            }
        }
        
        g.setColour(juce::Colours::cyan);
        g.strokePath(wavePath, juce::PathStrokeType(2.0f));
        
        // Center line
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawLine(bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY());
    }

    int operatorIndex;
    bool isActive = true;
    
    juce::Label titleLabel;
    juce::Rectangle<int> waveformArea;
    
    juce::Slider ratioSlider, detuneSlider, levelSlider;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMOperatorComponent)
};

//==============================================================================
/**
 * FM Algorithm Selector - Visual routing matrix
 */
class FMAlgorithmSelector : public juce::Component
{
public:
    FMAlgorithmSelector()
    {
        setSize(300, 200);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(10.0f);
        
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(bounds);
        
        // Draw selected algorithm
        drawAlgorithm(g, bounds, selectedAlgorithm);
        
        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("Algorithm " + juce::String(selectedAlgorithm + 1), 
                  bounds.removeFromTop(20), juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            selectedAlgorithm = (selectedAlgorithm + 1) % 8;
            repaint();
            
            if (onAlgorithmChanged)
                onAlgorithmChanged(selectedAlgorithm);
        }
    }

    void setAlgorithm(int algorithm)
    {
        selectedAlgorithm = juce::jlimit(0, 7, algorithm);
        repaint();
    }

    std::function<void(int algorithm)> onAlgorithmChanged;

private:
    void drawAlgorithm(juce::Graphics& g, juce::Rectangle<float> bounds, int algorithm)
    {
        // Draw operator boxes and connections based on algorithm
        // Algorithm 0: OP1→OP2→OP3→OP4→OUT (stack)
        // Algorithm 1: OP1→OP2, OP3→OP4 → OUT (parallel pairs)
        // etc...
        
        const float opSize = 40.0f;
        const float spacing = 50.0f;
        
        g.setColour(juce::Colours::orange);
        
        switch (algorithm)
        {
            case 0: // Stack: 1→2→3→4→OUT
            {
                for (int i = 0; i < 4; ++i)
                {
                    float x = bounds.getCentreX() - opSize * 0.5f;
                    float y = bounds.getY() + 30.0f + i * spacing;
                    
                    g.fillRoundedRectangle(x, y, opSize, opSize, 5.0f);
                    g.setColour(juce::Colours::white);
                    g.drawText(juce::String(i + 1), x, y, opSize, opSize, juce::Justification::centred);
                    
                    if (i < 3)
                    {
                        g.setColour(juce::Colours::orange);
                        g.drawArrow(juce::Line<float>(x + opSize * 0.5f, y + opSize,
                                                     x + opSize * 0.5f, y + spacing), 2.0f, 10.0f, 10.0f);
                    }
                    
                    g.setColour(juce::Colours::orange);
                }
                break;
            }
            
            case 1: // Parallel: (1→2) + (3→4) → OUT
            {
                // Left branch
                for (int i = 0; i < 2; ++i)
                {
                    float x = bounds.getCentreX() - opSize - 10.0f;
                    float y = bounds.getY() + 30.0f + i * spacing;
                    
                    g.fillRoundedRectangle(x, y, opSize, opSize, 5.0f);
                    g.setColour(juce::Colours::white);
                    g.drawText(juce::String(i + 1), x, y, opSize, opSize, juce::Justification::centred);
                    g.setColour(juce::Colours::orange);
                }
                
                // Right branch
                for (int i = 0; i < 2; ++i)
                {
                    float x = bounds.getCentreX() + 10.0f;
                    float y = bounds.getY() + 30.0f + i * spacing;
                    
                    g.fillRoundedRectangle(x, y, opSize, opSize, 5.0f);
                    g.setColour(juce::Colours::white);
                    g.drawText(juce::String(i + 3), x, y, opSize, opSize, juce::Justification::centred);
                    g.setColour(juce::Colours::orange);
                }
                break;
            }
            
            default:
                // More algorithms...
                g.drawText("Algorithm " + juce::String(algorithm + 1), 
                          bounds, juce::Justification::centred);
                break;
        }
    }

    int selectedAlgorithm = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMAlgorithmSelector)
};

//==============================================================================
/**
 * FM Synth UI - Main Component (Sytrus-style)
 */
class FMSynthUI : public juce::Component
{
public:
    FMSynthUI()
    {
        setupComponents();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Title
        bounds.removeFromTop(30);
        
        // Top: Algorithm selector
        algorithmSelector.setBounds(bounds.removeFromTop(220).reduced(5));
        
        bounds.removeFromTop(10);
        
        // Middle: Operators (2x2 grid)
        auto topRow = bounds.removeFromTop(200);
        operators[0]->setBounds(topRow.removeFromLeft(topRow.getWidth() / 2).reduced(5));
        operators[1]->setBounds(topRow.reduced(5));
        
        bounds.removeFromTop(10);
        
        auto bottomRow = bounds.removeFromTop(200);
        operators[2]->setBounds(bottomRow.removeFromLeft(bottomRow.getWidth() / 2).reduced(5));
        operators[3]->setBounds(bottomRow.reduced(5));
        
        bounds.removeFromTop(10);
        
        // Bottom: Global controls
        auto globalRow = bounds.removeFromTop(80).reduced(5);
        
        feedbackLabel.setBounds(globalRow.removeFromTop(20));
        feedbackSlider.setBounds(globalRow.removeFromTop(30));
        
        velocityLabel.setBounds(globalRow.removeFromTop(20));
        velocitySlider.setBounds(globalRow);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("FM SYNTHESIZER", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void setupComponents()
    {
        // Algorithm selector
        addAndMakeVisible(algorithmSelector);
        algorithmSelector.onAlgorithmChanged = [this](int algo) {
            DBG("Algorithm changed to: " << algo);
        };
        
        // Create 6 operators
        for (int i = 0; i < 6; ++i)
        {
            auto op = std::make_unique<FMOperatorComponent>(i);
            op->onParamsChanged = [i](float ratio, float detune, float level) {
                DBG("Operator " << i << " - Ratio: " << ratio << " Detune: " << detune << " Level: " << level);
            };
            addAndMakeVisible(op.get());
            operators.push_back(std::move(op));
        }
        
        // Only show first 4 operators initially
        for (int i = 4; i < 6; ++i)
        {
            operators[i]->setVisible(false);
        }
        
        // Global feedback
        addAndMakeVisible(feedbackLabel);
        feedbackLabel.setText("Feedback", juce::dontSendNotification);
        feedbackLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(feedbackSlider);
        feedbackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        feedbackSlider.setRange(0.0, 1.0, 0.01);
        feedbackSlider.setValue(0.0);
        feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        
        // Velocity sensitivity
        addAndMakeVisible(velocityLabel);
        velocityLabel.setText("Velocity Sensitivity", juce::dontSendNotification);
        velocityLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(velocitySlider);
        velocitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        velocitySlider.setRange(0.0, 1.0, 0.01);
        velocitySlider.setValue(0.5);
        velocitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    }

    FMAlgorithmSelector algorithmSelector;
    std::vector<std::unique_ptr<FMOperatorComponent>> operators;
    
    juce::Label feedbackLabel, velocityLabel;
    juce::Slider feedbackSlider, velocitySlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynthUI)
};

} // namespace GUI
} // namespace OmegaStudio
