/**
 * @file ProcessorPanels_Simple.cpp
 * @brief Simplified implementations
 */

#include "ProcessorPanels_Simple.h"

namespace omega {

// ============================================================================
// PitchCorrectionPanel
// ============================================================================

PitchCorrectionPanel::PitchCorrectionPanel(PitchCorrection* processor)
    : m_processor(processor) {
    
    m_strengthSlider.setRange(0.0, 1.0, 0.01);
    m_strengthSlider.setValue(0.8);
    m_strengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_strengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(&m_strengthSlider);
    
    m_strengthLabel.setText("Strength", juce::dontSendNotification);
    addAndMakeVisible(&m_strengthLabel);
}

void PitchCorrectionPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("AUTO-TUNE", getLocalBounds().removeFromTop(40), juce::Justification::centred);
}

void PitchCorrectionPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Title space
    
    auto sliderBounds = bounds.removeFromTop(30);
    m_strengthLabel.setBounds(sliderBounds.removeFromLeft(100));
    m_strengthSlider.setBounds(sliderBounds);
}

// ============================================================================
// VocalEnhancerPanel
// ============================================================================

VocalEnhancerPanel::VocalEnhancerPanel(VocalEnhancer* processor)
    : m_processor(processor) {
    
    m_titleLabel.setText("AI VOCAL ENHANCER", juce::dontSendNotification);
    m_titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&m_titleLabel);
    
    m_amountSlider.setRange(0.0, 1.0, 0.01);
    m_amountSlider.setValue(0.7);
    m_amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_amountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(&m_amountSlider);
}

void VocalEnhancerPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkslategrey);
}

void VocalEnhancerPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    
    m_titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    m_amountSlider.setBounds(bounds.removeFromTop(30));
}

// ============================================================================
// RecorderPanel
// ============================================================================

RecorderPanel::RecorderPanel(AudioRecorder* recorder)
    : m_recorder(recorder) {
    
    m_recordButton.setButtonText("RECORD");
    m_recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    addAndMakeVisible(&m_recordButton);
    
    m_stopButton.setButtonText("STOP");
    addAndMakeVisible(&m_stopButton);
}

void RecorderPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("RECORDER", getLocalBounds().removeFromTop(40), juce::Justification::centred);
}

void RecorderPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40);
    
    auto buttonArea = bounds.removeFromTop(40);
    m_recordButton.setBounds(buttonArea.removeFromLeft(100));
    buttonArea.removeFromLeft(10);
    m_stopButton.setBounds(buttonArea.removeFromLeft(100));
}

// ============================================================================
// SampleBrowserPanel
// ============================================================================

SampleBrowserPanel::SampleBrowserPanel(SampleManager* manager)
    : m_manager(manager) {
    
    m_loadButton.setButtonText("LOAD SAMPLE");
    addAndMakeVisible(&m_loadButton);
    
    m_statusLabel.setText("No sample loaded", juce::dontSendNotification);
    addAndMakeVisible(&m_statusLabel);
}

void SampleBrowserPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkblue);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("SAMPLE LIBRARY", getLocalBounds().removeFromTop(40), juce::Justification::centred);
}

void SampleBrowserPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40);
    
    m_loadButton.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    m_statusLabel.setBounds(bounds.removeFromTop(30));
}

} // namespace omega
