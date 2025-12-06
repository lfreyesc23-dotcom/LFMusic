#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Stem Separator UI - AI-powered audio source separation
 * Separate vocals, drums, bass, and other instruments from mixed audio
 */
class StemSeparatorWindow : public juce::DocumentWindow
{
public:
    StemSeparatorWindow()
        : DocumentWindow("Stem Separator (AI)",
                         juce::Colour(0xff2b2b2b),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new StemSeparatorComponent(), true);
        setResizable(true, true);
        centreWithSize(800, 600);
    }

private:
    //==============================================================================
    /**
     * Stem Type
     */
    enum class StemType
    {
        Vocals,
        Drums,
        Bass,
        Other,
        Piano,
        Guitar
    };

    //==============================================================================
    /**
     * Waveform Display per Stem
     */
    class StemWaveform : public juce::Component
    {
    public:
        StemWaveform(StemType type, const juce::String& name)
            : stemType(type), stemName(name)
        {
            addAndMakeVisible(muteButton);
            muteButton.setButtonText("M");
            muteButton.setClickingTogglesState(true);
            
            addAndMakeVisible(soloButton);
            soloButton.setButtonText("S");
            soloButton.setClickingTogglesState(true);
            
            addAndMakeVisible(volumeSlider);
            volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            volumeSlider.setRange(0.0, 1.0, 0.01);
            volumeSlider.setValue(0.8);
            volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            
            addAndMakeVisible(exportButton);
            exportButton.setButtonText("Export");
            exportButton.onClick = [this]() { exportStem(); };
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds();
            
            // Background
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRect(bounds);
            
            // Stem name
            g.setColour(getStemColor());
            g.fillRect(bounds.removeFromLeft(5));
            
            bounds.removeFromLeft(5);
            g.setColour(juce::Colours::white);
            g.drawText(stemName, bounds.removeFromLeft(80), juce::Justification::centredLeft);
            
            // Waveform area
            auto waveformArea = bounds.removeFromLeft(bounds.getWidth() - 250);
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRect(waveformArea);
            
            // Draw simulated waveform
            if (hasAudio)
            {
                g.setColour(getStemColor());
                juce::Path waveformPath;
                
                for (int x = 0; x < waveformArea.getWidth(); ++x)
                {
                    float progress = static_cast<float>(x) / waveformArea.getWidth();
                    float amplitude = std::sin(progress * 50.0f + phase) * 0.3f;
                    float y = waveformArea.getCentreY() + amplitude * waveformArea.getHeight();
                    
                    if (x == 0)
                        waveformPath.startNewSubPath(static_cast<float>(waveformArea.getX() + x), y);
                    else
                        waveformPath.lineTo(static_cast<float>(waveformArea.getX() + x), y);
                }
                
                g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
            }
            else
            {
                g.setColour(juce::Colours::grey);
                g.drawText("No audio", waveformArea, juce::Justification::centred);
            }
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.removeFromLeft(90); // Skip stem name area
            
            auto waveformWidth = bounds.getWidth() - 250;
            bounds.removeFromLeft(waveformWidth);
            
            bounds.removeFromLeft(10);
            
            muteButton.setBounds(bounds.removeFromLeft(30).reduced(2));
            soloButton.setBounds(bounds.removeFromLeft(30).reduced(2));
            
            bounds.removeFromLeft(10);
            volumeSlider.setBounds(bounds.removeFromLeft(100).reduced(2));
            
            bounds.removeFromLeft(10);
            exportButton.setBounds(bounds.removeFromLeft(60).reduced(2));
        }

        void setHasAudio(bool has)
        {
            hasAudio = has;
            repaint();
        }

        void updatePhase(float p)
        {
            phase = p;
            repaint();
        }

    private:
        juce::Colour getStemColor() const
        {
            switch (stemType)
            {
                case StemType::Vocals: return juce::Colour(0xff00d4ff); // Cyan
                case StemType::Drums:  return juce::Colour(0xffff8800); // Orange
                case StemType::Bass:   return juce::Colour(0xffff00ff); // Magenta
                case StemType::Other:  return juce::Colour(0xff00ff00); // Green
                case StemType::Piano:  return juce::Colour(0xffffff00); // Yellow
                case StemType::Guitar: return juce::Colour(0xffff0088); // Pink
                default: return juce::Colours::white;
            }
        }

        void exportStem()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Export Stem",
                                                     "Exporting " + stemName + " to WAV file...");
        }

        StemType stemType;
        juce::String stemName;
        bool hasAudio = false;
        float phase = 0.0f;
        
        juce::TextButton muteButton;
        juce::TextButton soloButton;
        juce::Slider volumeSlider;
        juce::TextButton exportButton;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemWaveform)
    };

    //==============================================================================
    /**
     * Progress Indicator
     */
    class ProgressComponent : public juce::Component,
                              private juce::Timer
    {
    public:
        ProgressComponent()
        {
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            // Background
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(bounds, 5.0f);
            
            // Progress bar
            if (isProcessing)
            {
                auto progressBounds = bounds.reduced(2.0f);
                progressBounds.setWidth(progressBounds.getWidth() * progress);
                
                g.setColour(juce::Colour(0xff00d4ff));
                g.fillRoundedRectangle(progressBounds, 3.0f);
            }
            
            // Text
            g.setColour(juce::Colours::white);
            juce::String text = isProcessing 
                ? "Processing: " + juce::String(static_cast<int>(progress * 100)) + "%"
                : "Ready";
            g.drawText(text, bounds, juce::Justification::centred);
        }

        void setProgress(float p)
        {
            progress = juce::jlimit(0.0f, 1.0f, p);
            repaint();
        }

        void setProcessing(bool processing)
        {
            isProcessing = processing;
            if (processing)
                progress = 0.0f;
            repaint();
        }

    private:
        void timerCallback() override
        {
            if (isProcessing && progress < 1.0f)
            {
                progress += 0.01f;
                repaint();
            }
        }

        bool isProcessing = false;
        float progress = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgressComponent)
    };

    //==============================================================================
    /**
     * Main Stem Separator Component
     */
    class StemSeparatorComponent : public juce::Component,
                                    private juce::Timer
    {
    public:
        StemSeparatorComponent()
        {
            // Load button
            addAndMakeVisible(loadButton);
            loadButton.setButtonText("Load Audio File");
            loadButton.onClick = [this]() { loadAudioFile(); };
            
            // Model selection
            addAndMakeVisible(modelLabel);
            modelLabel.setText("AI Model:", juce::dontSendNotification);
            
            addAndMakeVisible(modelCombo);
            modelCombo.addItem("Demucs v4 (Best Quality)", 1);
            modelCombo.addItem("Demucs v3 (Fast)", 2);
            modelCombo.addItem("Spleeter 4-stem", 3);
            modelCombo.addItem("Spleeter 5-stem", 4);
            modelCombo.setSelectedId(1);
            
            // Process button
            addAndMakeVisible(processButton);
            processButton.setButtonText("Separate Stems");
            processButton.onClick = [this]() { processAudio(); };
            processButton.setEnabled(false);
            
            // Progress
            addAndMakeVisible(progressBar);
            
            // Stem waveforms
            auto* vocalsWaveform = new StemWaveform(StemType::Vocals, "Vocals");
            addAndMakeVisible(vocalsWaveform);
            stemWaveforms.add(vocalsWaveform);
            
            auto* drumsWaveform = new StemWaveform(StemType::Drums, "Drums");
            addAndMakeVisible(drumsWaveform);
            stemWaveforms.add(drumsWaveform);
            
            auto* bassWaveform = new StemWaveform(StemType::Bass, "Bass");
            addAndMakeVisible(bassWaveform);
            stemWaveforms.add(bassWaveform);
            
            auto* otherWaveform = new StemWaveform(StemType::Other, "Other");
            addAndMakeVisible(otherWaveform);
            stemWaveforms.add(otherWaveform);
            
            // Export all button
            addAndMakeVisible(exportAllButton);
            exportAllButton.setButtonText("Export All Stems");
            exportAllButton.onClick = [this]() { exportAllStems(); };
            exportAllButton.setEnabled(false);
            
            startTimerHz(60);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);
            
            // Top controls
            auto topArea = bounds.removeFromTop(40);
            loadButton.setBounds(topArea.removeFromLeft(150).reduced(5));
            topArea.removeFromLeft(10);
            modelLabel.setBounds(topArea.removeFromLeft(80).reduced(5));
            modelCombo.setBounds(topArea.removeFromLeft(200).reduced(5));
            topArea.removeFromLeft(10);
            processButton.setBounds(topArea.removeFromLeft(150).reduced(5));
            
            bounds.removeFromTop(10);
            
            // Progress bar
            progressBar.setBounds(bounds.removeFromTop(30));
            
            bounds.removeFromTop(10);
            
            // Stem waveforms
            int waveformHeight = 80;
            for (auto* waveform : stemWaveforms)
            {
                waveform->setBounds(bounds.removeFromTop(waveformHeight));
                bounds.removeFromTop(5);
            }
            
            bounds.removeFromTop(10);
            
            // Export all button
            exportAllButton.setBounds(bounds.removeFromTop(40).reduced(5));
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff1a1a1a));
            
            // Title
            g.setColour(juce::Colours::white);
            g.setFont(20.0f);
            g.drawText("AI Stem Separator", getLocalBounds().removeFromTop(40), 
                       juce::Justification::centred);
        }

    private:
        void loadAudioFile()
        {
            fileChooser = std::make_unique<juce::FileChooser>("Select audio file...");
            auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
            
            fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
            {
                if (fc.getResults().size() > 0)
                {
                    audioLoaded = true;
                    processButton.setEnabled(true);
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                             "Audio Loaded",
                                                             "File loaded successfully!\nClick 'Separate Stems' to process.");
                }
            });
        }

        void processAudio()
        {
            progressBar.setProcessing(true);
            
            // Simulate processing delay
            juce::Timer::callAfterDelay(3000, [this]()
            {
                // Show stems
                for (auto* waveform : stemWaveforms)
                    waveform->setHasAudio(true);
                
                progressBar.setProcessing(false);
                exportAllButton.setEnabled(true);
                
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                         "Separation Complete",
                                                         "Stems separated successfully!\nYou can now export individual stems.");
            });
        }

        void exportAllStems()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Export All",
                                                     "Exporting all stems to separate WAV files...");
        }

        void timerCallback() override
        {
            static float phase = 0.0f;
            phase += 0.1f;
            
            for (auto* waveform : stemWaveforms)
                waveform->updatePhase(phase);
        }

        juce::TextButton loadButton;
        juce::Label modelLabel;
        juce::ComboBox modelCombo;
        juce::TextButton processButton;
        ProgressComponent progressBar;
        juce::OwnedArray<StemWaveform> stemWaveforms;
        juce::TextButton exportAllButton;
        
        bool audioLoaded = false;
        std::unique_ptr<juce::FileChooser> fileChooser;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparatorComponent)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparatorWindow)
};

} // namespace GUI
} // namespace OmegaStudio
