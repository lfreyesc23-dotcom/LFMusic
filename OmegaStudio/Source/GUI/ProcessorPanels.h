/**
 * @file ProcessorPanels.h
 * @brief GUI panels for controlling audio processors
 * 
 * Professional UI components for:
 * - PitchCorrection (Autotune)
 * - VocalEnhancer (AI)
 * - AudioRecorder
 * - SampleManager/Browser
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
 * @brief GUI panel for auto-tune controls
 */
class PitchCorrectionPanel : public juce::Component {
public:
    PitchCorrectionPanel(PitchCorrection* processor);
    ~PitchCorrectionPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateParameters();

    PitchCorrection* m_processor;

    // Controls
    juce::Slider m_strengthSlider;
    juce::Slider m_speedSlider;
    juce::ComboBox m_scaleCombo;
    juce::ComboBox m_rootNoteCombo;
    juce::ToggleButton m_formantToggle;
    juce::ComboBox m_modeCombo;

    // Labels
    juce::Label m_strengthLabel;
    juce::Label m_speedLabel;
    juce::Label m_scaleLabel;
    juce::Label m_rootNoteLabel;
    juce::Label m_detectedPitchLabel;

    // Metering
    float m_detectedPitch = 0.0f;
    float m_correctedPitch = 0.0f;

    juce::Timer m_updateTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchCorrectionPanel)
};

/**
 * @class VocalEnhancerPanel
 * @brief GUI panel for AI vocal enhancement
 */
class VocalEnhancerPanel : public juce::Component {
public:
    VocalEnhancerPanel(VocalEnhancer* processor);
    ~VocalEnhancerPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateParameters();

    VocalEnhancer* m_processor;

    // Controls
    juce::Slider m_amountSlider;
    juce::ComboBox m_modeCombo;
    juce::ComboBox m_voiceTypeCombo;
    
    juce::Slider m_deEsserSlider;
    juce::Slider m_breathRemovalSlider;
    juce::Slider m_proximityCompSlider;
    
    juce::Slider m_presenceSlider;
    juce::Slider m_brightnessSlider;

    juce::ToggleButton m_deEsserToggle;
    juce::ToggleButton m_breathToggle;
    juce::ToggleButton m_proximityToggle;
    juce::ToggleButton m_autoEQToggle;

    // Labels
    juce::Label m_titleLabel;
    juce::Label m_formantLabel;

    // Metering
    float m_inputLevel = 0.0f;
    float m_outputLevel = 0.0f;

    juce::Timer m_updateTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalEnhancerPanel)
};

/**
 * @class RecorderPanel
 * @brief GUI panel for multi-track recording
 */
class RecorderPanel : public juce::Component {
public:
    RecorderPanel(AudioRecorder* recorder);
    ~RecorderPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void updateDisplay();

    AudioRecorder* m_recorder;

    // Transport controls
    juce::TextButton m_recordButton;
    juce::TextButton m_stopButton;
    juce::TextButton m_exportButton;

    // Track controls
    std::array<juce::ToggleButton, 8> m_armButtons;
    std::array<juce::Slider, 8> m_volumeSliders;
    std::array<juce::ToggleButton, 8> m_monitorButtons;

    // Display
    juce::Label m_timeLabel;
    juce::Label m_statusLabel;

    // Metering
    std::array<float, 8> m_trackLevels;

    juce::Timer m_updateTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecorderPanel)
};

/**
 * @class SampleBrowserPanel
 * @brief GUI panel for browsing and managing samples
 */
class SampleBrowserPanel : public juce::Component,
                           public juce::TableListBoxModel {
public:
    SampleBrowserPanel(SampleManager* manager);
    ~SampleBrowserPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // TableListBoxModel implementation
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height,
                           bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId,
                  int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;
    void cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& e) override;

private:
    void refreshSampleList();
    void searchSamples();
    void loadSelectedSample();

    SampleManager* m_manager;

    // Controls
    juce::TextEditor m_searchBox;
    juce::ComboBox m_categoryFilter;
    juce::TableListBox m_sampleTable;
    juce::TextButton m_importButton;
    juce::TextButton m_scanButton;

    // Display
    juce::Label m_sampleCountLabel;
    juce::Image m_waveformThumbnail;

    // Data
    std::vector<std::shared_ptr<Sample>> m_displayedSamples;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowserPanel)
};

/**
 * @class ProcessorWorkspace
 * @brief Main workspace containing all processor panels
 */
class ProcessorWorkspace : public juce::Component {
public:
    ProcessorWorkspace();
    ~ProcessorWorkspace() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Set processors
     */
    void setPitchCorrection(PitchCorrection* processor);
    void setVocalEnhancer(VocalEnhancer* processor);
    void setAudioRecorder(AudioRecorder* recorder);
    void setSampleManager(SampleManager* manager);

private:
    // Panels
    std::unique_ptr<PitchCorrectionPanel> m_pitchCorrectionPanel;
    std::unique_ptr<VocalEnhancerPanel> m_vocalEnhancerPanel;
    std::unique_ptr<RecorderPanel> m_recorderPanel;
    std::unique_ptr<SampleBrowserPanel> m_sampleBrowserPanel;

    // Tab component
    juce::TabbedComponent m_tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorWorkspace)
};

} // namespace omega
