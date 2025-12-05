/**
 * @file ProcessorPanels.cpp
 * @brief Implementation of processor GUI panels
 */

#include "ProcessorPanels.h"

namespace omega {

// ============================================================================
// PitchCorrectionPanel Implementation
// ============================================================================

PitchCorrectionPanel::PitchCorrectionPanel(PitchCorrection* processor)
    : m_processor(processor) {
    
    // Strength slider
    m_strengthSlider.setRange(0.0, 1.0, 0.01);
    m_strengthSlider.setValue(0.5);
    m_strengthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_strengthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    m_strengthSlider.onValueChange = [this]() {
        if (m_processor) {
            m_processor->setStrength(static_cast<float>(m_strengthSlider.getValue()));
        }
    };
    addAndMakeVisible(m_strengthSlider);

    m_strengthLabel.setText("Strength", juce::dontSendNotification);
    m_strengthLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_strengthLabel);

    // Speed slider
    m_speedSlider.setRange(0.0, 1.0, 0.01);
    m_speedSlider.setValue(0.5);
    m_speedSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    m_speedSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    m_speedSlider.onValueChange = [this]() {
        if (m_processor) {
            m_processor->setSpeed(static_cast<float>(m_speedSlider.getValue()));
        }
    };
    addAndMakeVisible(m_speedSlider);

    m_speedLabel.setText("Speed", juce::dontSendNotification);
    m_speedLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_speedLabel);

    // Scale combo
    m_scaleCombo.addItem("Chromatic", 1);
    m_scaleCombo.addItem("Major", 2);
    m_scaleCombo.addItem("Minor", 3);
    m_scaleCombo.addItem("Pentatonic", 4);
    m_scaleCombo.addItem("Blues", 5);
    m_scaleCombo.setSelectedId(1);
    m_scaleCombo.onChange = [this]() {
        if (m_processor) {
            m_processor->setScale(static_cast<PitchCorrection::Scale>(m_scaleCombo.getSelectedId() - 1));
        }
    };
    addAndMakeVisible(m_scaleCombo);

    m_scaleLabel.setText("Scale", juce::dontSendNotification);
    addAndMakeVisible(m_scaleLabel);

    // Root note combo
    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < 12; ++i) {
        m_rootNoteCombo.addItem(notes[i], i + 1);
    }
    m_rootNoteCombo.setSelectedId(1);
    m_rootNoteCombo.onChange = [this]() {
        if (m_processor) {
            m_processor->setRootNote(m_rootNoteCombo.getSelectedId() - 1);
        }
    };
    addAndMakeVisible(m_rootNoteCombo);

    m_rootNoteLabel.setText("Key", juce::dontSendNotification);
    addAndMakeVisible(m_rootNoteLabel);

    // Mode combo
    m_modeCombo.addItem("Automatic", 1);
    m_modeCombo.addItem("Manual", 2);
    m_modeCombo.addItem("Off", 3);
    m_modeCombo.setSelectedId(1);
    m_modeCombo.onChange = [this]() {
        if (m_processor) {
            m_processor->setMode(static_cast<PitchCorrection::Mode>(m_modeCombo.getSelectedId() - 1));
        }
    };
    addAndMakeVisible(m_modeCombo);

    // Formant toggle
    m_formantToggle.setButtonText("Preserve Formants");
    m_formantToggle.setToggleState(true, juce::dontSendNotification);
    m_formantToggle.onClick = [this]() {
        if (m_processor) {
            m_processor->setFormantPreservation(m_formantToggle.getToggleState());
        }
    };
    addAndMakeVisible(m_formantToggle);

    // Detected pitch label
    m_detectedPitchLabel.setText("Detected: -- Hz", juce::dontSendNotification);
    m_detectedPitchLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_detectedPitchLabel);

    // Update timer
    m_updateTimer.onCallback = [this]() { updateParameters(); };
    m_updateTimer.startTimerHz(30);
}

void PitchCorrectionPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
    
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("AUTO-TUNE", getLocalBounds().removeFromTop(40), juce::Justification::centred);
}

void PitchCorrectionPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40); // Title

    auto row1 = bounds.removeFromTop(120);
    m_strengthLabel.setBounds(row1.removeFromLeft(120).removeFromTop(20));
    m_strengthSlider.setBounds(row1.removeFromLeft(120).reduced(10));

    m_speedLabel.setBounds(row1.removeFromLeft(120).removeFromTop(20));
    m_speedSlider.setBounds(row1.removeFromLeft(120).reduced(10));

    bounds.removeFromTop(20);

    auto row2 = bounds.removeFromTop(30);
    m_scaleLabel.setBounds(row2.removeFromLeft(80));
    m_scaleCombo.setBounds(row2.removeFromLeft(150));
    row2.removeFromLeft(20);
    m_rootNoteLabel.setBounds(row2.removeFromLeft(80));
    m_rootNoteCombo.setBounds(row2.removeFromLeft(100));

    bounds.removeFromTop(10);
    m_modeCombo.setBounds(bounds.removeFromTop(30).removeFromLeft(200));
    
    bounds.removeFromTop(10);
    m_formantToggle.setBounds(bounds.removeFromTop(30));
    
    bounds.removeFromTop(20);
    m_detectedPitchLabel.setBounds(bounds.removeFromTop(30));
}

void PitchCorrectionPanel::updateParameters() {
    if (m_processor) {
        float detected = m_processor->getDetectedPitch();
        float corrected = m_processor->getCorrectedPitch();
        
        juce::String text = "Detected: ";
        if (detected > 0.0f) {
            text += juce::String(detected, 1) + " Hz → " + juce::String(corrected, 1) + " Hz";
        } else {
            text += "-- Hz";
        }
        m_detectedPitchLabel.setText(text, juce::dontSendNotification);
    }
}

// ============================================================================
// VocalEnhancerPanel Implementation
// ============================================================================

VocalEnhancerPanel::VocalEnhancerPanel(VocalEnhancer* processor)
    : m_processor(processor) {
    
    m_titleLabel.setText("AI VOCAL ENHANCER", juce::dontSendNotification);
    m_titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    m_titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_titleLabel);

    // Amount slider
    m_amountSlider.setRange(0.0, 1.0, 0.01);
    m_amountSlider.setValue(0.7);
    m_amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_amountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    m_amountSlider.onValueChange = [this]() {
        if (m_processor) {
            m_processor->setAmount(static_cast<float>(m_amountSlider.getValue()));
        }
    };
    addAndMakeVisible(m_amountSlider);

    // Mode combo
    m_modeCombo.addItem("Natural", 1);
    m_modeCombo.addItem("Podcast", 2);
    m_modeCombo.addItem("Radio", 3);
    m_modeCombo.addItem("Studio", 4);
    m_modeCombo.addItem("Custom", 5);
    m_modeCombo.setSelectedId(1);
    m_modeCombo.onChange = [this]() {
        if (m_processor) {
            m_processor->setMode(static_cast<VocalEnhancer::Mode>(m_modeCombo.getSelectedId() - 1));
        }
    };
    addAndMakeVisible(m_modeCombo);

    // Toggles
    m_deEsserToggle.setButtonText("De-Esser");
    m_deEsserToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(m_deEsserToggle);

    m_breathToggle.setButtonText("Breath Removal");
    m_breathToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(m_breathToggle);

    m_proximityToggle.setButtonText("Proximity Comp");
    m_proximityToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(m_proximityToggle);

    m_autoEQToggle.setButtonText("Auto EQ");
    m_autoEQToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(m_autoEQToggle);

    m_updateTimer.onCallback = [this]() { updateParameters(); };
    m_updateTimer.startTimerHz(30);
}

void VocalEnhancerPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void VocalEnhancerPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    
    m_titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    auto row1 = bounds.removeFromTop(30);
    row1.removeFromLeft(80);
    m_amountSlider.setBounds(row1);
    
    bounds.removeFromTop(20);
    m_modeCombo.setBounds(bounds.removeFromTop(30).removeFromLeft(200));
    
    bounds.removeFromTop(20);
    auto toggleArea = bounds.removeFromTop(120);
    m_deEsserToggle.setBounds(toggleArea.removeFromTop(30));
    m_breathToggle.setBounds(toggleArea.removeFromTop(30));
    m_proximityToggle.setBounds(toggleArea.removeFromTop(30));
    m_autoEQToggle.setBounds(toggleArea.removeFromTop(30));
}

void VocalEnhancerPanel::updateParameters() {
    if (m_processor) {
        m_inputLevel = m_processor->getInputLevel();
        m_outputLevel = m_processor->getOutputLevel();
        repaint();
    }
}

// ============================================================================
// RecorderPanel Implementation
// ============================================================================

RecorderPanel::RecorderPanel(AudioRecorder* recorder)
    : m_recorder(recorder) {
    
    m_recordButton.setButtonText("● REC");
    m_recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    m_recordButton.onClick = [this]() {
        if (m_recorder) {
            if (m_recorder->isRecording()) {
                m_recorder->pauseRecording();
            } else {
                m_recorder->startRecording();
            }
        }
    };
    addAndMakeVisible(m_recordButton);

    m_stopButton.setButtonText("■ STOP");
    m_stopButton.onClick = [this]() {
        if (m_recorder) {
            m_recorder->stopRecording();
        }
    };
    addAndMakeVisible(m_stopButton);

    m_exportButton.setButtonText("Export");
    m_exportButton.onClick = [this]() {
        // Open file chooser for export
        juce::FileChooser chooser("Export Recording");
        if (chooser.browseForDirectory()) {
            auto dir = chooser.getResult();
            if (m_recorder) {
                m_recorder->exportAllTracks(dir, "Recording", AudioRecorder::FileFormat::WAV_24bit);
            }
        }
    };
    addAndMakeVisible(m_exportButton);

    // Track controls
    for (int i = 0; i < 8; ++i) {
        m_armButtons[i].setButtonText(juce::String(i + 1));
        m_armButtons[i].onClick = [this, i]() {
            if (m_recorder) {
                if (m_armButtons[i].getToggleState()) {
                    m_recorder->armTrack(i);
                } else {
                    m_recorder->disarmTrack(i);
                }
            }
        };
        addAndMakeVisible(m_armButtons[i]);

        m_volumeSliders[i].setRange(0.0, 1.0, 0.01);
        m_volumeSliders[i].setValue(1.0);
        m_volumeSliders[i].setSliderStyle(juce::Slider::LinearVertical);
        m_volumeSliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_volumeSliders[i].onValueChange = [this, i]() {
            if (m_recorder) {
                m_recorder->setTrackVolume(i, static_cast<float>(m_volumeSliders[i].getValue()));
            }
        };
        addAndMakeVisible(m_volumeSliders[i]);

        m_monitorButtons[i].setButtonText("M");
        addAndMakeVisible(m_monitorButtons[i]);
    }

    m_timeLabel.setText("00:00.000", juce::dontSendNotification);
    m_timeLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    m_timeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_timeLabel);

    m_updateTimer.onCallback = [this]() { updateDisplay(); };
    m_updateTimer.startTimerHz(30);
}

void RecorderPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
    
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("MULTI-TRACK RECORDER", getLocalBounds().removeFromTop(40), juce::Justification::centred);
}

void RecorderPanel::resized() {
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(40);
    
    auto transport = bounds.removeFromTop(60);
    m_recordButton.setBounds(transport.removeFromLeft(100).reduced(5));
    m_stopButton.setBounds(transport.removeFromLeft(100).reduced(5));
    m_exportButton.setBounds(transport.removeFromLeft(100).reduced(5));
    
    m_timeLabel.setBounds(bounds.removeFromTop(40));
    
    bounds.removeFromTop(20);
    
    auto trackArea = bounds;
    int trackWidth = trackArea.getWidth() / 8;
    
    for (int i = 0; i < 8; ++i) {
        auto track = trackArea.removeFromLeft(trackWidth).reduced(5);
        m_armButtons[i].setBounds(track.removeFromTop(30));
        m_volumeSliders[i].setBounds(track.removeFromTop(track.getHeight() - 30));
        m_monitorButtons[i].setBounds(track.removeFromTop(30));
    }
}

void RecorderPanel::updateDisplay() {
    if (m_recorder) {
        double time = m_recorder->getRecordingTime();
        int minutes = static_cast<int>(time / 60.0);
        int seconds = static_cast<int>(time) % 60;
        int millis = static_cast<int>((time - static_cast<int>(time)) * 1000);
        
        juce::String timeStr = juce::String::formatted("%02d:%02d.%03d", minutes, seconds, millis);
        m_timeLabel.setText(timeStr, juce::dontSendNotification);
        
        // Update levels
        for (int i = 0; i < 8; ++i) {
            m_trackLevels[i] = m_recorder->getTrackLevel(i);
        }
        
        repaint();
    }
}

// ============================================================================
// SampleBrowserPanel Implementation
// ============================================================================

SampleBrowserPanel::SampleBrowserPanel(SampleManager* manager)
    : m_manager(manager) {
    
    m_searchBox.setTextToShowWhenEmpty("Search samples...", juce::Colours::grey);
    m_searchBox.onReturnKey = [this]() { searchSamples(); };
    addAndMakeVisible(m_searchBox);

    m_categoryFilter.addItem("All Categories", 1);
    m_categoryFilter.setSelectedId(1);
    addAndMakeVisible(m_categoryFilter);

    m_sampleTable.setModel(this);
    m_sampleTable.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    addAndMakeVisible(m_sampleTable);

    m_importButton.setButtonText("Import");
    m_importButton.onClick = [this]() {
        juce::FileChooser chooser("Import Audio File");
        if (chooser.browseForFileToOpen()) {
            auto file = chooser.getResult();
            if (m_manager) {
                m_manager->importFile(file);
                refreshSampleList();
            }
        }
    };
    addAndMakeVisible(m_importButton);

    m_scanButton.setButtonText("Scan Folder");
    m_scanButton.onClick = [this]() {
        juce::FileChooser chooser("Scan Folder");
        if (chooser.browseForDirectory()) {
            auto dir = chooser.getResult();
            if (m_manager) {
                m_manager->scanDirectory(dir, true);
                refreshSampleList();
            }
        }
    };
    addAndMakeVisible(m_scanButton);

    refreshSampleList();
}

void SampleBrowserPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void SampleBrowserPanel::resized() {
    auto bounds = getLocalBounds().reduced(10);
    
    auto topBar = bounds.removeFromTop(40);
    m_searchBox.setBounds(topBar.removeFromLeft(200));
    topBar.removeFromLeft(10);
    m_categoryFilter.setBounds(topBar.removeFromLeft(150));
    
    auto buttons = topBar;
    m_importButton.setBounds(buttons.removeFromRight(100));
    m_scanButton.setBounds(buttons.removeFromRight(100));
    
    bounds.removeFromTop(10);
    m_sampleTable.setBounds(bounds);
}

int SampleBrowserPanel::getNumRows() {
    return static_cast<int>(m_displayedSamples.size());
}

void SampleBrowserPanel::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) {
    if (rowIsSelected) {
        g.fillAll(juce::Colour(0xff4a4a4a));
    } else if (rowNumber % 2 == 0) {
        g.fillAll(juce::Colour(0xff2a2a2a));
    }
}

void SampleBrowserPanel::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) {
    if (rowNumber >= m_displayedSamples.size()) return;
    
    auto& sample = m_displayedSamples[rowNumber];
    const auto& meta = sample->getMetadata();
    
    g.setColour(juce::Colours::white);
    juce::String text;
    
    switch (columnId) {
        case 1: text = meta.name; break;
        case 2: text = meta.category; break;
        case 3: text = juce::String(meta.bpm, 1) + " BPM"; break;
        case 4: text = meta.keyName; break;
        case 5: text = juce::String(meta.lengthInSeconds, 1) + " s"; break;
    }
    
    g.drawText(text, 5, 0, width - 10, height, juce::Justification::centredLeft);
}

void SampleBrowserPanel::cellClicked(int rowNumber, int columnId, const juce::MouseEvent& e) {
    // Selection handled by table
}

void SampleBrowserPanel::cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& e) {
    loadSelectedSample();
}

void SampleBrowserPanel::refreshSampleList() {
    if (m_manager) {
        m_displayedSamples.clear();
        
        auto libraries = m_manager->getAllLibraries();
        for (auto* lib : libraries) {
            auto samples = lib->getAllSamples();
            m_displayedSamples.insert(m_displayedSamples.end(), samples.begin(), samples.end());
        }
        
        m_sampleTable.updateContent();
    }
}

void SampleBrowserPanel::searchSamples() {
    if (m_manager) {
        juce::String query = m_searchBox.getText();
        if (query.isEmpty()) {
            refreshSampleList();
        } else {
            m_displayedSamples = m_manager->globalSearch(query);
            m_sampleTable.updateContent();
        }
    }
}

void SampleBrowserPanel::loadSelectedSample() {
    int row = m_sampleTable.getSelectedRow();
    if (row >= 0 && row < m_displayedSamples.size()) {
        auto& sample = m_displayedSamples[row];
        if (m_manager) {
            m_manager->loadSample(sample->getMetadata().uuid);
        }
    }
}

// ============================================================================
// ProcessorWorkspace Implementation
// ============================================================================

ProcessorWorkspace::ProcessorWorkspace()
    : m_tabs(juce::TabbedButtonBar::TabsAtTop) {
    
    addAndMakeVisible(m_tabs);
}

void ProcessorWorkspace::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void ProcessorWorkspace::resized() {
    m_tabs.setBounds(getLocalBounds());
}

void ProcessorWorkspace::setPitchCorrection(PitchCorrection* processor) {
    m_pitchCorrectionPanel = std::make_unique<PitchCorrectionPanel>(processor);
    m_tabs.addTab("Auto-Tune", juce::Colour(0xff2d2d2d), m_pitchCorrectionPanel.get(), false);
}

void ProcessorWorkspace::setVocalEnhancer(VocalEnhancer* processor) {
    m_vocalEnhancerPanel = std::make_unique<VocalEnhancerPanel>(processor);
    m_tabs.addTab("Vocal AI", juce::Colour(0xff2d2d2d), m_vocalEnhancerPanel.get(), false);
}

void ProcessorWorkspace::setAudioRecorder(AudioRecorder* recorder) {
    m_recorderPanel = std::make_unique<RecorderPanel>(recorder);
    m_tabs.addTab("Recorder", juce::Colour(0xff2d2d2d), m_recorderPanel.get(), false);
}

void ProcessorWorkspace::setSampleManager(SampleManager* manager) {
    m_sampleBrowserPanel = std::make_unique<SampleBrowserPanel>(manager);
    m_tabs.addTab("Samples", juce::Colour(0xff2d2d2d), m_sampleBrowserPanel.get(), false);
}

} // namespace omega
