/**
 * @file ProcessorPanels_Simple.h
 * @brief Simplified GUI panels that will compile
 */

#pragma once

#include <JuceHeader.h>
#include "../Audio/DSP/PitchCorrection.h"
#include "../Audio/AI/VocalEnhancer.h"
#include "../Audio/Recording/AudioRecorder.h"
#include "../Audio/Library/SampleManager.h"

namespace omega {

/**
 * @class PitchCorrectionPanel
 * @brief Minimal autotune panel
 */
class PitchCorrectionPanel : public juce::Component {
public:
    PitchCorrectionPanel(PitchCorrection* processor);
    ~PitchCorrectionPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    PitchCorrection* m_processor;
    juce::Slider m_strengthSlider;
    juce::Label m_strengthLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchCorrectionPanel)
};

/**
 * @class VocalEnhancerPanel
 * @brief Minimal AI vocal enhancement panel
 */
class VocalEnhancerPanel : public juce::Component {
public:
    VocalEnhancerPanel(VocalEnhancer* processor);
    ~VocalEnhancerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    VocalEnhancer* m_processor;
    juce::Slider m_amountSlider;
    juce::Label m_titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalEnhancerPanel)
};

/**
 * @class RecorderPanel
 * @brief Minimal recording panel
 */
class RecorderPanel : public juce::Component {
public:
    RecorderPanel(AudioRecorder* recorder);
    ~RecorderPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    AudioRecorder* m_recorder;
    juce::TextButton m_recordButton;
    juce::TextButton m_stopButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecorderPanel)
};

/**
 * @class SampleBrowserPanel
 * @brief Minimal sample browser panel
 */
class SampleBrowserPanel : public juce::Component {
public:
    SampleBrowserPanel(SampleManager* manager);
    ~SampleBrowserPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SampleManager* m_manager;
    juce::TextButton m_loadButton;
    juce::Label m_statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowserPanel)
};

} // namespace omega
