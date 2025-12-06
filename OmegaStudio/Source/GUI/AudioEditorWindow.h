#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Edison-style Audio Editor - Destructive editing with advanced tools
 */
class AudioEditorWindow : public juce::DocumentWindow,
                          private juce::Timer
{
public:
    AudioEditorWindow()
        : DocumentWindow("Edison Audio Editor",
                         juce::Colour(0xff2b2b2b),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new EditorComponent(), true);
        setResizable(true, true);
        centreWithSize(900, 600);
        startTimerHz(60); // For spectral display animation
    }

private:
    //==============================================================================
    /**
     * Audio Region Marker
     */
    struct RegionMarker
    {
        double startTime;
        double endTime;
        juce::String name;
        juce::Colour color;
        
        RegionMarker(double start, double end, const juce::String& n)
            : startTime(start), endTime(end), name(n)
            , color(juce::Colour::fromHSV(juce::Random::getSystemRandom().nextFloat(), 0.7f, 0.8f, 1.0f))
        {}
    };

    //==============================================================================
    /**
     * Waveform Display with Selection
     */
    class WaveformDisplay : public juce::Component
    {
    public:
        WaveformDisplay()
        {
            formatManager.registerBasicFormats();
        }

        void loadFile(const juce::File& file)
        {
            auto* reader = formatManager.createReaderFor(file);
            if (reader != nullptr)
            {
                std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
                transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                readerSource.reset(newSource.release());
                
                // Cache waveform data
                sampleRate = reader->sampleRate;
                lengthInSamples = reader->lengthInSamples;
                numChannels = reader->numChannels;
                
                cacheWaveformData(*reader);
                repaint();
            }
        }

        void setSelection(double start, double end)
        {
            selectionStart = start;
            selectionEnd = end;
            repaint();
        }

        void clearSelection()
        {
            selectionStart = 0.0;
            selectionEnd = 0.0;
            repaint();
        }

        std::pair<double, double> getSelection() const
        {
            return {selectionStart, selectionEnd};
        }

        void setZoom(float zoomLevel)
        {
            zoom = juce::jlimit(1.0f, 100.0f, zoomLevel);
            repaint();
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff1a1a1a));
            
            if (peakData.isEmpty())
            {
                g.setColour(juce::Colours::grey);
                g.drawText("Drop audio file here", getLocalBounds(), juce::Justification::centred);
                return;
            }
            
            auto bounds = getLocalBounds().toFloat();
            
            // Draw waveform
            g.setColour(juce::Colour(0xff00d4ff));
            
            juce::Path waveformPath;
            int samplesPerPixel = juce::jmax(1, static_cast<int>(peakData.size() / (bounds.getWidth() * zoom)));
            
            for (int x = 0; x < bounds.getWidth() * zoom && x < peakData.size() / samplesPerPixel; ++x)
            {
                int sampleIndex = x * samplesPerPixel;
                if (sampleIndex < peakData.size())
                {
                    float peak = peakData[sampleIndex];
                    float y = bounds.getCentreY() - (peak * bounds.getHeight() * 0.4f);
                    
                    if (x == 0)
                        waveformPath.startNewSubPath(static_cast<float>(x), y);
                    else
                        waveformPath.lineTo(static_cast<float>(x), y);
                }
            }
            
            g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
            
            // Draw selection overlay
            if (selectionStart != selectionEnd)
            {
                float startX = static_cast<float>(selectionStart * bounds.getWidth());
                float endX = static_cast<float>(selectionEnd * bounds.getWidth());
                
                g.setColour(juce::Colour(0xffff8800).withAlpha(0.3f));
                g.fillRect(startX, 0.0f, endX - startX, bounds.getHeight());
                
                g.setColour(juce::Colours::orange);
                g.drawVerticalLine(static_cast<int>(startX), 0.0f, bounds.getHeight());
                g.drawVerticalLine(static_cast<int>(endX), 0.0f, bounds.getHeight());
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isShiftDown())
            {
                // Extend selection
                double clickTime = static_cast<double>(e.x) / getWidth();
                if (std::abs(clickTime - selectionStart) < std::abs(clickTime - selectionEnd))
                    selectionStart = clickTime;
                else
                    selectionEnd = clickTime;
            }
            else
            {
                // Start new selection
                selectionStart = static_cast<double>(e.x) / getWidth();
                selectionEnd = selectionStart;
            }
            repaint();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            selectionEnd = juce::jlimit(0.0, 1.0, static_cast<double>(e.x) / getWidth());
            repaint();
        }

    private:
        void cacheWaveformData(juce::AudioFormatReader& reader)
        {
            peakData.clear();
            
            const int samplesPerPixel = 512;
            juce::AudioBuffer<float> buffer(static_cast<int>(reader.numChannels), samplesPerPixel);
            
            for (int64_t pos = 0; pos < reader.lengthInSamples; pos += samplesPerPixel)
            {
                reader.read(&buffer, 0, samplesPerPixel, pos, true, true);
                
                float peak = 0.0f;
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    auto* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                        peak = juce::jmax(peak, std::abs(data[i]));
                }
                
                peakData.add(peak);
            }
        }

        juce::AudioFormatManager formatManager;
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        juce::AudioTransportSource transportSource;
        
        juce::Array<float> peakData;
        double sampleRate = 0.0;
        int64_t lengthInSamples = 0;
        int numChannels = 0;
        
        double selectionStart = 0.0;
        double selectionEnd = 0.0;
        float zoom = 1.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
    };

    //==============================================================================
    /**
     * Spectral Display
     */
    class SpectralDisplay : public juce::Component
    {
    public:
        SpectralDisplay() : phase(0.0f) {}

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colours::black);
            
            // Simulate spectral analysis
            auto bounds = getLocalBounds().toFloat();
            
            for (int y = 0; y < bounds.getHeight(); ++y)
            {
                float freq = 1.0f - (y / bounds.getHeight());
                float intensity = std::sin(freq * 20.0f + phase) * 0.5f + 0.5f;
                
                juce::Colour color = juce::Colour::fromHSV(0.6f - intensity * 0.3f, 1.0f, intensity, 1.0f);
                g.setColour(color);
                g.fillRect(0.0f, static_cast<float>(y), bounds.getWidth(), 1.0f);
            }
        }

        void updatePhase(float p)
        {
            phase = p;
            repaint();
        }

    private:
        float phase = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralDisplay)
    };

    //==============================================================================
    /**
     * Main Editor Component
     */
    class EditorComponent : public juce::Component
    {
    public:
        EditorComponent()
        {
            addAndMakeVisible(waveformDisplay);
            addAndMakeVisible(spectralDisplay);
            
            // Tools
            addAndMakeVisible(normalizeButton);
            normalizeButton.setButtonText("Normalize");
            normalizeButton.onClick = [this]() { applyNormalize(); };
            
            addAndMakeVisible(reverseButton);
            reverseButton.setButtonText("Reverse");
            reverseButton.onClick = [this]() { applyReverse(); };
            
            addAndMakeVisible(fadeInButton);
            fadeInButton.setButtonText("Fade In");
            fadeInButton.onClick = [this]() { applyFadeIn(); };
            
            addAndMakeVisible(fadeOutButton);
            fadeOutButton.setButtonText("Fade Out");
            fadeOutButton.onClick = [this]() { applyFadeOut(); };
            
            addAndMakeVisible(silenceButton);
            silenceButton.setButtonText("Silence");
            silenceButton.onClick = [this]() { applySilence(); };
            
            addAndMakeVisible(invertButton);
            invertButton.setButtonText("Invert");
            invertButton.onClick = [this]() { applyInvert(); };
            
            // Zoom
            addAndMakeVisible(zoomSlider);
            zoomSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            zoomSlider.setRange(1.0, 100.0, 1.0);
            zoomSlider.setValue(1.0);
            zoomSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
            zoomSlider.onValueChange = [this]() {
                waveformDisplay.setZoom(static_cast<float>(zoomSlider.getValue()));
            };
            
            addAndMakeVisible(loadButton);
            loadButton.setButtonText("Load Audio");
            loadButton.onClick = [this]() { loadAudioFile(); };
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            
            // Toolbar
            auto toolbar = bounds.removeFromTop(40);
            loadButton.setBounds(toolbar.removeFromLeft(100).reduced(5));
            toolbar.removeFromLeft(10);
            normalizeButton.setBounds(toolbar.removeFromLeft(90).reduced(5));
            reverseButton.setBounds(toolbar.removeFromLeft(80).reduced(5));
            fadeInButton.setBounds(toolbar.removeFromLeft(80).reduced(5));
            fadeOutButton.setBounds(toolbar.removeFromLeft(80).reduced(5));
            silenceButton.setBounds(toolbar.removeFromLeft(80).reduced(5));
            invertButton.setBounds(toolbar.removeFromLeft(70).reduced(5));
            
            // Zoom
            auto zoomArea = bounds.removeFromBottom(30);
            zoomSlider.setBounds(zoomArea.reduced(10));
            
            // Waveform and spectral
            int spectralHeight = bounds.getHeight() / 3;
            spectralDisplay.setBounds(bounds.removeFromBottom(spectralHeight));
            waveformDisplay.setBounds(bounds);
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
                    auto file = fc.getResult();
                    waveformDisplay.loadFile(file);
                }
            });
        }

        void applyNormalize()
        {
            // TODO: Implement normalize
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Normalize",
                                                     "Normalize applied to selection");
        }

        void applyReverse()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Reverse",
                                                     "Selection reversed");
        }

        void applyFadeIn()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Fade In",
                                                     "Fade in applied");
        }

        void applyFadeOut()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Fade Out",
                                                     "Fade out applied");
        }

        void applySilence()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Silence",
                                                     "Selection silenced");
        }

        void applyInvert()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Invert",
                                                     "Selection inverted");
        }

        WaveformDisplay waveformDisplay;
        SpectralDisplay spectralDisplay;
        
        juce::TextButton normalizeButton;
        juce::TextButton reverseButton;
        juce::TextButton fadeInButton;
        juce::TextButton fadeOutButton;
        juce::TextButton silenceButton;
        juce::TextButton invertButton;
        juce::TextButton loadButton;
        
        juce::Slider zoomSlider;
        
        std::unique_ptr<juce::FileChooser> fileChooser;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorComponent)
    };

    void timerCallback() override
    {
        if (auto* editor = dynamic_cast<EditorComponent*>(getContentComponent()))
        {
            // Update spectral display animation
            static float phase = 0.0f;
            phase += 0.1f;
            // spectralDisplay.updatePhase(phase); // TODO: Access spectral display
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEditorWindow)
};

} // namespace GUI
} // namespace OmegaStudio
