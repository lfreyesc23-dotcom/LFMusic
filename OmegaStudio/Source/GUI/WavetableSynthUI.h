#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * 3D-style Wavetable Oscilloscope Display - Serum style
 */
class WavetableDisplay : public juce::Component,
                          private juce::Timer
{
public:
    WavetableDisplay()
    {
        startTimerHz(60); // 60 FPS animation
        
        // Generate test waveform
        generateTestWaveform();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background gradient
        juce::ColourGradient gradient(juce::Colour(0xff1a1a1a), 0.0f, 0.0f,
                                     juce::Colour(0xff0a0a0a), 0.0f, bounds.getHeight(), false);
        g.setGradientFill(gradient);
        g.fillRect(bounds);
        
        // Draw grid
        g.setColour(juce::Colours::grey.withAlpha(0.1f));
        int numLines = 8;
        for (int i = 0; i <= numLines; ++i)
        {
            float y = bounds.getHeight() * (i / static_cast<float>(numLines));
            g.drawLine(0.0f, y, bounds.getWidth(), y, 1.0f);
        }
        
        // Draw 3D waveform layers
        draw3DWaveform(g, bounds);
        
        // Border
        g.setColour(juce::Colours::orange);
        g.drawRect(bounds, 2.0f);
    }

private:
    void timerCallback() override
    {
        // Animate morphing
        morphPosition += 0.01f;
        if (morphPosition > 1.0f)
            morphPosition = 0.0f;
        
        repaint();
    }

    void draw3DWaveform(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        const int numLayers = 5;
        const float layerSpacing = 15.0f;
        
        for (int layer = 0; layer < numLayers; ++layer)
        {
            float layerAlpha = 1.0f - (layer / static_cast<float>(numLayers));
            float layerOffset = layer * layerSpacing;
            
            juce::Path path;
            bool first = true;
            
            float centerY = bounds.getCentreY() + layerOffset;
            float amplitude = (bounds.getHeight() * 0.3f) * layerAlpha;
            
            for (int i = 0; i < waveformData.size(); ++i)
            {
                float x = bounds.getX() + (i / static_cast<float>(waveformData.size())) * bounds.getWidth();
                float sample = waveformData[i];
                float y = centerY - (sample * amplitude);
                
                if (first)
                {
                    path.startNewSubPath(x, y);
                    first = false;
                }
                else
                {
                    path.lineTo(x, y);
                }
            }
            
            // Color gradient based on layer
            juce::Colour layerColor = juce::Colour::fromHSV(0.08f + layer * 0.05f, 0.8f, 1.0f, layerAlpha);
            
            g.setColour(layerColor);
            g.strokePath(path, juce::PathStrokeType(2.0f - layer * 0.3f));
        }
    }

    void generateTestWaveform()
    {
        waveformData.resize(512);
        for (int i = 0; i < 512; ++i)
        {
            float phase = i / 512.0f * juce::MathConstants<float>::twoPi;
            waveformData[i] = std::sin(phase) * 0.8f + std::sin(phase * 3.0f) * 0.2f;
        }
    }

    std::vector<float> waveformData;
    float morphPosition = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableDisplay)
};

//==============================================================================
/**
 * ADSR Envelope Display with Draggable Points
 */
class ADSRDisplay : public juce::Component
{
public:
    struct ADSRParams
    {
        float attack = 0.1f;
        float decay = 0.2f;
        float sustain = 0.7f;
        float release = 0.3f;
    };

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(5.0f);
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(bounds);
        
        // Draw ADSR curve
        juce::Path envelope;
        
        float totalWidth = bounds.getWidth();
        float height = bounds.getHeight();
        
        // Calculate positions
        float attackX = params.attack * totalWidth * 0.3f;
        float decayX = attackX + (params.decay * totalWidth * 0.3f);
        float releaseStart = totalWidth * 0.7f;
        float releaseX = releaseStart + (params.release * totalWidth * 0.3f);
        
        // Draw envelope path
        envelope.startNewSubPath(bounds.getX(), bounds.getBottom());
        envelope.lineTo(bounds.getX() + attackX, bounds.getY());
        envelope.lineTo(bounds.getX() + decayX, bounds.getY() + height * (1.0f - params.sustain));
        envelope.lineTo(bounds.getX() + releaseStart, bounds.getY() + height * (1.0f - params.sustain));
        envelope.lineTo(bounds.getX() + releaseX, bounds.getBottom());
        
        // Fill under curve
        auto fillPath = envelope;
        fillPath.lineTo(bounds.getRight(), bounds.getBottom());
        fillPath.closeSubPath();
        
        g.setColour(juce::Colours::orange.withAlpha(0.3f));
        g.fillPath(fillPath);
        
        // Draw outline
        g.setColour(juce::Colours::orange);
        g.strokePath(envelope, juce::PathStrokeType(2.0f));
        
        // Draw control points
        drawControlPoint(g, bounds.getX() + attackX, bounds.getY(), "A");
        drawControlPoint(g, bounds.getX() + decayX, bounds.getY() + height * (1.0f - params.sustain), "D");
        drawControlPoint(g, bounds.getX() + releaseStart, bounds.getY() + height * (1.0f - params.sustain), "S");
        drawControlPoint(g, bounds.getX() + releaseX, bounds.getBottom(), "R");
    }

    void setParams(const ADSRParams& newParams)
    {
        params = newParams;
        repaint();
    }

    std::function<void(const ADSRParams&)> onParamsChanged;

private:
    void drawControlPoint(juce::Graphics& g, float x, float y, const juce::String& label)
    {
        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colours::orange);
        g.drawEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f, 2.0f);
        
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText(label, x - 10.0f, y - 20.0f, 20.0f, 15.0f, juce::Justification::centred);
    }

    ADSRParams params;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRDisplay)
};

//==============================================================================
/**
 * LFO Display with Animation
 */
class LFODisplay : public juce::Component,
                    private juce::Timer
{
public:
    enum class WaveShape { Sine, Square, Saw, Triangle };

    LFODisplay()
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(bounds);
        
        // Draw LFO waveform
        juce::Path lfoPath;
        bool first = true;
        
        for (int i = 0; i < 100; ++i)
        {
            float x = bounds.getX() + (i / 100.0f) * bounds.getWidth();
            float phase = (i / 100.0f + lfoPhase) * juce::MathConstants<float>::twoPi;
            float value = getLFOValue(phase);
            float y = bounds.getCentreY() - (value * bounds.getHeight() * 0.4f);
            
            if (first)
            {
                lfoPath.startNewSubPath(x, y);
                first = false;
            }
            else
            {
                lfoPath.lineTo(x, y);
            }
        }
        
        g.setColour(juce::Colours::cyan);
        g.strokePath(lfoPath, juce::PathStrokeType(2.0f));
        
        // Draw center line
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawLine(bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY());
    }

    void setWaveShape(WaveShape shape)
    {
        waveShape = shape;
        repaint();
    }

private:
    void timerCallback() override
    {
        lfoPhase += 0.02f;
        if (lfoPhase > 1.0f)
            lfoPhase -= 1.0f;
        repaint();
    }

    float getLFOValue(float phase) const
    {
        switch (waveShape)
        {
            case WaveShape::Sine:
                return std::sin(phase);
            case WaveShape::Square:
                return (std::sin(phase) > 0.0f) ? 1.0f : -1.0f;
            case WaveShape::Saw:
                return 2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f;
            case WaveShape::Triangle:
                return 2.0f * std::abs(2.0f * (phase / juce::MathConstants<float>::twoPi) - 1.0f) - 1.0f;
            default:
                return 0.0f;
        }
    }

    WaveShape waveShape = WaveShape::Sine;
    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFODisplay)
};

//==============================================================================
/**
 * Wavetable Synth UI - Main Component (Serum-style)
 */
class WavetableSynthUI : public juce::Component
{
public:
    WavetableSynthUI()
    {
        setupComponents();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Top: Wavetable display (large)
        wavetableDisplay.setBounds(bounds.removeFromTop(200));
        
        bounds.removeFromTop(10);
        
        // Middle row: Oscillator controls
        auto oscRow = bounds.removeFromTop(80);
        
        auto col1 = oscRow.removeFromLeft(oscRow.getWidth() / 4);
        positionLabel.setBounds(col1.removeFromTop(20));
        positionSlider.setBounds(col1);
        
        auto col2 = oscRow.removeFromLeft(oscRow.getWidth() / 3);
        detuneLabel.setBounds(col2.removeFromTop(20));
        detuneSlider.setBounds(col2);
        
        auto col3 = oscRow.removeFromLeft(oscRow.getWidth() / 2);
        unisonLabel.setBounds(col3.removeFromTop(20));
        unisonSlider.setBounds(col3);
        
        levelLabel.setBounds(oscRow.removeFromTop(20));
        levelSlider.setBounds(oscRow);
        
        bounds.removeFromTop(10);
        
        // ADSR Section
        auto adsrSection = bounds.removeFromTop(150);
        adsrDisplay.setBounds(adsrSection);
        
        bounds.removeFromTop(10);
        
        // Bottom: LFO + Filter
        auto bottomRow = bounds.removeFromTop(120);
        
        auto lfoArea = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
        lfoLabel.setBounds(lfoArea.removeFromTop(20));
        lfoDisplay.setBounds(lfoArea.removeFromTop(80));
        lfoRateSlider.setBounds(lfoArea);
        
        bottomRow.removeFromLeft(10);
        
        filterLabel.setBounds(bottomRow.removeFromTop(20));
        auto filterRow = bottomRow.removeFromTop(60).reduced(5);
        cutoffLabel.setBounds(filterRow.removeFromTop(15));
        cutoffSlider.setBounds(filterRow.removeFromTop(20));
        resonanceLabel.setBounds(filterRow.removeFromTop(15));
        resonanceSlider.setBounds(filterRow);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("WAVETABLE SYNTH", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    void setupComponents()
    {
        // Wavetable display
        addAndMakeVisible(wavetableDisplay);
        
        // Position slider
        addAndMakeVisible(positionLabel);
        positionLabel.setText("Position", juce::dontSendNotification);
        positionLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(positionSlider);
        positionSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        positionSlider.setRange(0.0, 1.0, 0.01);
        positionSlider.setValue(0.0);
        positionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Detune slider
        addAndMakeVisible(detuneLabel);
        detuneLabel.setText("Detune", juce::dontSendNotification);
        detuneLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(detuneSlider);
        detuneSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        detuneSlider.setRange(-100.0, 100.0, 1.0);
        detuneSlider.setValue(0.0);
        detuneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Unison slider
        addAndMakeVisible(unisonLabel);
        unisonLabel.setText("Unison", juce::dontSendNotification);
        unisonLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(unisonSlider);
        unisonSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        unisonSlider.setRange(1.0, 8.0, 1.0);
        unisonSlider.setValue(1.0);
        unisonSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Level slider
        addAndMakeVisible(levelLabel);
        levelLabel.setText("Level", juce::dontSendNotification);
        levelLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(levelSlider);
        levelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        levelSlider.setRange(0.0, 1.0, 0.01);
        levelSlider.setValue(0.8);
        levelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // ADSR display
        addAndMakeVisible(adsrDisplay);
        
        // LFO
        addAndMakeVisible(lfoLabel);
        lfoLabel.setText("LFO", juce::dontSendNotification);
        lfoLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(lfoDisplay);
        
        addAndMakeVisible(lfoRateSlider);
        lfoRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        lfoRateSlider.setRange(0.1, 20.0, 0.1);
        lfoRateSlider.setValue(2.0);
        lfoRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        
        // Filter
        addAndMakeVisible(filterLabel);
        filterLabel.setText("FILTER", juce::dontSendNotification);
        filterLabel.setJustificationType(juce::Justification::centred);
        
        addAndMakeVisible(cutoffLabel);
        cutoffLabel.setText("Cutoff", juce::dontSendNotification);
        cutoffLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(cutoffSlider);
        cutoffSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        cutoffSlider.setRange(20.0, 20000.0, 1.0);
        cutoffSlider.setValue(2000.0);
        cutoffSlider.setSkewFactorFromMidPoint(1000.0);
        cutoffSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        
        addAndMakeVisible(resonanceLabel);
        resonanceLabel.setText("Resonance", juce::dontSendNotification);
        resonanceLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(resonanceSlider);
        resonanceSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        resonanceSlider.setRange(0.0, 1.0, 0.01);
        resonanceSlider.setValue(0.3);
        resonanceSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    }

    // Display components
    WavetableDisplay wavetableDisplay;
    ADSRDisplay adsrDisplay;
    LFODisplay lfoDisplay;
    
    // Oscillator controls
    juce::Label positionLabel, detuneLabel, unisonLabel, levelLabel;
    juce::Slider positionSlider, detuneSlider, unisonSlider, levelSlider;
    
    // LFO controls
    juce::Label lfoLabel;
    juce::Slider lfoRateSlider;
    
    // Filter controls
    juce::Label filterLabel, cutoffLabel, resonanceLabel;
    juce::Slider cutoffSlider, resonanceSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableSynthUI)
};

} // namespace GUI
} // namespace OmegaStudio
