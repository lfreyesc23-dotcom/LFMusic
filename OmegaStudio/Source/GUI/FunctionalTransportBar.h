//==============================================================================
// FunctionalTransportBar.h
// Barra de transporte funcional estilo FL Studio 2025
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Barra de transporte completa con todos los controles
//==============================================================================
class FunctionalTransportBar : public juce::Component,
                               private juce::Timer {
public:
    //==========================================================================
    // Callbacks
    //==========================================================================
    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void()> onRecord;
    std::function<void(double)> onTempoChange;
    std::function<void(bool)> onMetronomeToggle;
    std::function<void(bool)> onLoopToggle;
    std::function<void(int, int)> onTimeSignatureChange;
    
    //==========================================================================
    FunctionalTransportBar() {
        // Play button
        playButton.setButtonText("▶");
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4CAF50));
        playButton.onClick = [this] {
            isPlaying = !isPlaying;
            playButton.setButtonText(isPlaying ? "⏸" : "▶");
            if (onPlay) onPlay();
        };
        addAndMakeVisible(playButton);
        
        // Stop button
        stopButton.setButtonText("⏹");
        stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffF44336));
        stopButton.onClick = [this] {
            isPlaying = false;
            playButton.setButtonText("▶");
            currentPosition = 0.0;
            if (onStop) onStop();
        };
        addAndMakeVisible(stopButton);
        
        // Record button
        recordButton.setButtonText("⏺");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffE91E63));
        recordButton.onClick = [this] {
            isRecording = !isRecording;
            recordButton.setColour(juce::TextButton::buttonColourId, 
                isRecording ? juce::Colour(0xffFF5722) : juce::Colour(0xffE91E63));
            if (onRecord) onRecord();
        };
        addAndMakeVisible(recordButton);
        
        // Loop button
        loopButton.setButtonText("🔁");
        loopButton.setClickingTogglesState(true);
        loopButton.onClick = [this] {
            if (onLoopToggle) onLoopToggle(loopButton.getToggleState());
        };
        addAndMakeVisible(loopButton);
        
        // Metronome button
        metronomeButton.setButtonText("🎵");
        metronomeButton.setClickingTogglesState(true);
        metronomeButton.onClick = [this] {
            if (onMetronomeToggle) onMetronomeToggle(metronomeButton.getToggleState());
        };
        addAndMakeVisible(metronomeButton);
        
        // Tempo slider
        tempoSlider.setRange(20.0, 300.0, 0.01);
        tempoSlider.setValue(120.0);
        tempoSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 70, 25);
        tempoSlider.onValueChange = [this] {
            if (onTempoChange) onTempoChange(tempoSlider.getValue());
        };
        addAndMakeVisible(tempoSlider);
        
        // Tempo label
        tempoLabel.setText("BPM:", juce::dontSendNotification);
        tempoLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(tempoLabel);
        
        // Time signature numerator
        timeSigNumerator.setRange(1, 16, 1);
        timeSigNumerator.setValue(4);
        timeSigNumerator.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 25);
        timeSigNumerator.onValueChange = [this] {
            if (onTimeSignatureChange) 
                onTimeSignatureChange((int)timeSigNumerator.getValue(), 
                                     (int)timeSigDenominator.getValue());
        };
        addAndMakeVisible(timeSigNumerator);
        
        // Time signature denominator
        timeSigDenominator.setRange(1, 16, 1);
        timeSigDenominator.setValue(4);
        timeSigDenominator.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 25);
        timeSigDenominator.onValueChange = [this] {
            if (onTimeSignatureChange) 
                onTimeSignatureChange((int)timeSigNumerator.getValue(), 
                                     (int)timeSigDenominator.getValue());
        };
        addAndMakeVisible(timeSigDenominator);
        
        // Time signature label
        timeSigLabel.setText("Time:", juce::dontSendNotification);
        timeSigLabel.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(timeSigLabel);
        
        // Position display
        positionLabel.setText("00:00:000", juce::dontSendNotification);
        positionLabel.setJustificationType(juce::Justification::centred);
        positionLabel.setFont(juce::Font(16.0f, juce::Font::bold));
        addAndMakeVisible(positionLabel);
        
        // CPU meter
        cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
        cpuLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(cpuLabel);
        
        startTimer(50); // Update position display
    }
    
    ~FunctionalTransportBar() override {
        stopTimer();
    }
    
    //==========================================================================
    void paint(juce::Graphics& g) override {
        // Background oscuro estilo FL Studio
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        // Border inferior
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawLine(0, 0, (float)getWidth(), 0, 2.0f);
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(5);
        
        // Botones de control (izquierda)
        auto buttonArea = area.removeFromLeft(200);
        auto buttonWidth = 45;
        playButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        stopButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        recordButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        loopButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        metronomeButton.setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
        
        area.removeFromLeft(10); // Spacer
        
        // Position display (centro izquierda)
        positionLabel.setBounds(area.removeFromLeft(120).reduced(2));
        
        area.removeFromLeft(10); // Spacer
        
        // Tempo control
        tempoLabel.setBounds(area.removeFromLeft(50).reduced(2));
        tempoSlider.setBounds(area.removeFromLeft(150).reduced(2));
        
        area.removeFromLeft(10); // Spacer
        
        // Time signature
        timeSigLabel.setBounds(area.removeFromLeft(50).reduced(2));
        timeSigNumerator.setBounds(area.removeFromLeft(50).reduced(2));
        timeSigDenominator.setBounds(area.removeFromLeft(50).reduced(2));
        
        // CPU meter (derecha)
        area.removeFromRight(10); // Spacer from right
        cpuLabel.setBounds(area.removeFromRight(100).reduced(2));
    }
    
    //==========================================================================
    // Public API
    //==========================================================================
    void setPosition(double seconds) {
        currentPosition = seconds;
        updatePositionDisplay();
    }
    
    void setCPULoad(double percentage) {
        cpuLabel.setText(juce::String::formatted("CPU: %.1f%%", percentage), 
                        juce::dontSendNotification);
        
        // Color based on load
        if (percentage > 80.0)
            cpuLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        else if (percentage > 60.0)
            cpuLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        else
            cpuLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    }
    
    void setTempo(double bpm) {
        tempoSlider.setValue(bpm, juce::dontSendNotification);
    }
    
    bool isCurrentlyPlaying() const { return isPlaying; }
    bool isCurrentlyRecording() const { return isRecording; }
    
private:
    //==========================================================================
    void timerCallback() override {
        if (isPlaying) {
            currentPosition += 0.05; // 50ms timer
            updatePositionDisplay();
        }
    }
    
    void updatePositionDisplay() {
        int totalMs = (int)(currentPosition * 1000.0);
        int minutes = totalMs / 60000;
        int seconds = (totalMs % 60000) / 1000;
        int milliseconds = totalMs % 1000;
        
        positionLabel.setText(
            juce::String::formatted("%02d:%02d:%03d", minutes, seconds, milliseconds),
            juce::dontSendNotification
        );
    }
    
    //==========================================================================
    // Components
    //==========================================================================
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton recordButton;
    juce::TextButton loopButton;
    juce::TextButton metronomeButton;
    
    juce::Slider tempoSlider;
    juce::Label tempoLabel;
    
    juce::Slider timeSigNumerator;
    juce::Slider timeSigDenominator;
    juce::Label timeSigLabel;
    
    juce::Label positionLabel;
    juce::Label cpuLabel;
    
    // State
    bool isPlaying = false;
    bool isRecording = false;
    double currentPosition = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionalTransportBar)
};

} // namespace GUI
} // namespace OmegaStudio
