#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Drum Pad Component - MPC-style pad with velocity visualization
 */
class DrumPadComponent : public juce::Component
{
public:
    DrumPadComponent(int padIndex)
        : index(padIndex)
    {
        setSize(80, 80);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(3.0f);
        
        // Background gradient
        juce::ColourGradient gradient;
        if (isPressed || isPlaying)
        {
            gradient = juce::ColourGradient(padColor.brighter(0.5f), bounds.getX(), bounds.getY(),
                                           padColor, bounds.getX(), bounds.getBottom(), false);
        }
        else
        {
            gradient = juce::ColourGradient(padColor.darker(0.3f), bounds.getX(), bounds.getY(),
                                           padColor.darker(0.7f), bounds.getX(), bounds.getBottom(), false);
        }
        
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 8.0f);
        
        // Velocity indicator
        if (lastVelocity > 0)
        {
            float velHeight = bounds.getHeight() * (lastVelocity / 127.0f);
            auto velBounds = bounds.removeFromBottom(velHeight);
            g.setColour(padColor.brighter(0.3f).withAlpha(0.5f));
            g.fillRoundedRectangle(velBounds, 8.0f);
        }
        
        // Border
        g.setColour(isPressed || isPlaying ? juce::Colours::white : juce::Colours::grey);
        g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
        
        // Pad label
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(juce::String(index + 1), bounds, juce::Justification::centred);
        
        // Sample name (if assigned)
        if (sampleName.isNotEmpty())
        {
            g.setFont(9.0f);
            g.drawText(sampleName, bounds.removeFromBottom(15), juce::Justification::centred);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isLeftButtonDown())
        {
            isPressed = true;
            
            // Calculate velocity from click position (Y-axis)
            float relativeY = 1.0f - (e.position.y / getHeight());
            uint8_t velocity = static_cast<uint8_t>(juce::jlimit(0.0f, 127.0f, relativeY * 127.0f));
            lastVelocity = velocity;
            
            if (onPadTriggered)
                onPadTriggered(index, velocity);
            
            // Visual feedback
            startFlash();
            repaint();
        }
        else if (e.mods.isRightButtonDown())
        {
            // Show context menu for sample assignment
            showContextMenu();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isPressed = false;
        repaint();
    }

    void setSampleName(const juce::String& name)
    {
        sampleName = name;
        repaint();
    }

    void setPadColor(juce::Colour color)
    {
        padColor = color;
        repaint();
    }

    void triggerVisual(uint8_t velocity)
    {
        lastVelocity = velocity;
        startFlash();
    }

    std::function<void(int padIndex, uint8_t velocity)> onPadTriggered;
    std::function<void(int padIndex)> onSampleAssign;

private:
    void startFlash()
    {
        isPlaying = true;
        juce::Timer::callAfterDelay(100, [this]() {
            isPlaying = false;
            repaint();
        });
    }

    void showContextMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Load Sample...");
        menu.addItem(2, "Clear Sample");
        menu.addSeparator();
        menu.addItem(3, "Tune...");
        menu.addItem(4, "Pan...");
        menu.addItem(5, "Volume...");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result == 1 && onSampleAssign)
            {
                onSampleAssign(index);
            }
        });
    }

    int index;
    bool isPressed = false;
    bool isPlaying = false;
    uint8_t lastVelocity = 0;
    juce::String sampleName;
    juce::Colour padColor = juce::Colours::orange;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadComponent)
};

//==============================================================================
/**
 * DrumMachine UI - 16-pad MPC-style drum sampler
 */
class DrumMachineUI : public juce::Component
{
public:
    DrumMachineUI()
    {
        setupComponents();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        // Title
        bounds.removeFromTop(30);
        
        // Pad grid (4x4)
        auto padArea = bounds.removeFromLeft(bounds.getWidth() * 0.6f);
        const int padSize = 90;
        const int padSpacing = 10;
        
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                int padIndex = row * 4 + col;
                int x = col * (padSize + padSpacing);
                int y = row * (padSize + padSpacing);
                pads[padIndex]->setBounds(x, y, padSize, padSize);
            }
        }
        
        padArea.removeFromTop((padSize + padSpacing) * 4);
        
        // Right panel: Controls
        auto controlArea = bounds.reduced(10);
        
        // Master volume
        masterVolumeLabel.setBounds(controlArea.removeFromTop(20));
        masterVolumeSlider.setBounds(controlArea.removeFromTop(60).reduced(5));
        
        controlArea.removeFromTop(20);
        
        // Selected pad controls
        selectedPadLabel.setBounds(controlArea.removeFromTop(25));
        
        // Tuning
        tuningLabel.setBounds(controlArea.removeFromTop(20));
        tuningSlider.setBounds(controlArea.removeFromTop(40).reduced(5));
        
        // Pan
        panLabel.setBounds(controlArea.removeFromTop(20));
        panSlider.setBounds(controlArea.removeFromTop(40).reduced(5));
        
        // Volume
        volumeLabel.setBounds(controlArea.removeFromTop(20));
        volumeSlider.setBounds(controlArea.removeFromTop(40).reduced(5));
        
        controlArea.removeFromTop(20);
        
        // Buttons
        loadSampleButton.setBounds(controlArea.removeFromTop(30).reduced(5));
        clearSampleButton.setBounds(controlArea.removeFromTop(30).reduced(5));
        
        controlArea.removeFromTop(20);
        
        // MIDI learn
        midiLearnButton.setBounds(controlArea.removeFromTop(30).reduced(5));
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("DRUM MACHINE", getLocalBounds().removeFromTop(30), juce::Justification::centred);
        
        // Pad area background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(10.0f, 40.0f, 400.0f, 400.0f, 10.0f);
    }

    void triggerPad(int padIndex, uint8_t velocity)
    {
        if (padIndex >= 0 && padIndex < 16)
        {
            pads[padIndex]->triggerVisual(velocity);
        }
    }

private:
    void setupComponents()
    {
        // Create 16 pads
        for (int i = 0; i < 16; ++i)
        {
            auto pad = std::make_unique<DrumPadComponent>(i);
            
            pad->onPadTriggered = [this, i](int idx, uint8_t vel) {
                handlePadTriggered(idx, vel);
            };
            
            pad->onSampleAssign = [this, i](int idx) {
                handleSampleAssign(idx);
            };
            
            // Set pad colors (different per row)
            juce::Colour rowColors[] = {
                juce::Colours::red,
                juce::Colours::orange,
                juce::Colours::yellow,
                juce::Colours::green
            };
            pad->setPadColor(rowColors[i / 4]);
            
            addAndMakeVisible(pad.get());
            pads.push_back(std::move(pad));
        }
        
        // Master volume
        addAndMakeVisible(masterVolumeLabel);
        masterVolumeLabel.setText("Master Volume", juce::dontSendNotification);
        masterVolumeLabel.setJustificationType(juce::Justification::centred);
        masterVolumeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        
        addAndMakeVisible(masterVolumeSlider);
        masterVolumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        masterVolumeSlider.setRange(0.0, 1.0, 0.01);
        masterVolumeSlider.setValue(0.8);
        masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        
        // Selected pad label
        addAndMakeVisible(selectedPadLabel);
        selectedPadLabel.setText("Pad 1 Controls", juce::dontSendNotification);
        selectedPadLabel.setJustificationType(juce::Justification::centred);
        selectedPadLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        selectedPadLabel.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
        
        // Tuning
        addAndMakeVisible(tuningLabel);
        tuningLabel.setText("Tuning", juce::dontSendNotification);
        tuningLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(tuningSlider);
        tuningSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        tuningSlider.setRange(-24.0, 24.0, 1.0);
        tuningSlider.setValue(0.0);
        tuningSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        tuningSlider.setTextValueSuffix(" st");
        
        // Pan
        addAndMakeVisible(panLabel);
        panLabel.setText("Pan", juce::dontSendNotification);
        panLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(panSlider);
        panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(0.0);
        panSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        
        // Volume
        addAndMakeVisible(volumeLabel);
        volumeLabel.setText("Volume", juce::dontSendNotification);
        volumeLabel.setJustificationType(juce::Justification::centredLeft);
        
        addAndMakeVisible(volumeSlider);
        volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        volumeSlider.setRange(0.0, 1.0, 0.01);
        volumeSlider.setValue(0.8);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        
        // Load/Clear buttons
        addAndMakeVisible(loadSampleButton);
        loadSampleButton.setButtonText("Load Sample");
        loadSampleButton.onClick = [this]() {
            handleSampleAssign(selectedPad);
        };
        
        addAndMakeVisible(clearSampleButton);
        clearSampleButton.setButtonText("Clear Sample");
        clearSampleButton.onClick = [this]() {
            pads[selectedPad]->setSampleName("");
        };
        
        // MIDI learn
        addAndMakeVisible(midiLearnButton);
        midiLearnButton.setButtonText("MIDI Learn");
        midiLearnButton.setClickingTogglesState(true);
        midiLearnButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        midiLearnButton.onClick = [this]() {
            bool learning = midiLearnButton.getToggleState();
            DBG("MIDI Learn " << (learning ? "ON" : "OFF"));
        };
    }

    void handlePadTriggered(int padIndex, uint8_t velocity)
    {
        selectedPad = padIndex;
        selectedPadLabel.setText("Pad " + juce::String(padIndex + 1) + " Controls", juce::dontSendNotification);
        
        DBG("Pad " << padIndex << " triggered with velocity: " << (int)velocity);
        
        // Trigger sound engine
        if (onPadTriggered)
            onPadTriggered(padIndex, velocity);
    }

    void handleSampleAssign(int padIndex)
    {
        fileChooser = std::make_unique<juce::FileChooser>("Select Audio Sample",
                                                          juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                                          "*.wav;*.aiff;*.mp3");
        
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        
        fileChooser->launchAsync(flags, [this, padIndex](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                pads[padIndex]->setSampleName(file.getFileNameWithoutExtension());
                DBG("Assigned sample to pad " << padIndex << ": " << file.getFullPathName());
                
                if (onSampleAssigned)
                    onSampleAssigned(padIndex, file);
            }
        });
    }

    std::vector<std::unique_ptr<DrumPadComponent>> pads;
    
    juce::Label masterVolumeLabel, selectedPadLabel;
    juce::Slider masterVolumeSlider;
    
    juce::Label tuningLabel, panLabel, volumeLabel;
    juce::Slider tuningSlider, panSlider, volumeSlider;
    
    juce::TextButton loadSampleButton, clearSampleButton, midiLearnButton;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
    int selectedPad = 0;

public:
    std::function<void(int padIndex, uint8_t velocity)> onPadTriggered;
    std::function<void(int padIndex, const juce::File& sampleFile)> onSampleAssigned;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumMachineUI)
};

} // namespace GUI
} // namespace OmegaStudio
