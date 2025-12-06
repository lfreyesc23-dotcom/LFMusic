#pragma once

#include <JuceHeader.h>
#include "FLStudioLookAndFeel.h"

namespace OmegaStudio {
namespace GUI {

using Omega::GUI::FLColors;

//==============================================================================
/** 
 * Toolbar superior con controles de grabación estilo FL Studio 2025
 */
class RecordToolbar : public juce::Component {
public:
    RecordToolbar() {
        // Botón de grabación
        recordButton_.setButtonText("REC");
        recordButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
        recordButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkred);
        recordButton_.onClick = [this]() { 
            isRecording_ = !isRecording_;
            if (onRecordClicked) onRecordClicked(isRecording_);
            repaint();
        };
        addAndMakeVisible(recordButton_);
        
        // Botón Play
        playButton_.setButtonText("▶");
        playButton_.setColour(juce::TextButton::buttonColourId, FLColors::Green);
        playButton_.onClick = [this]() { 
            isPlaying_ = !isPlaying_;
            if (onPlayClicked) onPlayClicked(isPlaying_);
            repaint();
        };
        addAndMakeVisible(playButton_);
        
        // Botón Stop
        stopButton_.setButtonText("■");
        stopButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
        stopButton_.onClick = [this]() { 
            isPlaying_ = false;
            isRecording_ = false;
            if (onStopClicked) onStopClicked();
            repaint();
        };
        addAndMakeVisible(stopButton_);
        
        // Botón Loop
        loopButton_.setButtonText("🔁");
        loopButton_.setClickingTogglesState(true);
        loopButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        loopButton_.setColour(juce::TextButton::buttonOnColourId, FLColors::Orange);
        addAndMakeVisible(loopButton_);
        
        // Control de Tempo
        tempoLabel_.setText("BPM", juce::dontSendNotification);
        tempoLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(tempoLabel_);
        
        tempoSlider_.setRange(60.0, 200.0, 0.1);
        tempoSlider_.setValue(120.0);
        tempoSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
        tempoSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        tempoSlider_.setColour(juce::Slider::trackColourId, FLColors::Orange);
        tempoSlider_.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        tempoSlider_.onValueChange = [this]() {
            if (onTempoChanged) onTempoChanged(tempoSlider_.getValue());
        };
        addAndMakeVisible(tempoSlider_);
        
        // Botón de Metrónomo
        metronomeButton_.setButtonText("♪ Click");
        metronomeButton_.setClickingTogglesState(true);
        metronomeButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        metronomeButton_.setColour(juce::TextButton::buttonOnColourId, FLColors::Cyan);
        addAndMakeVisible(metronomeButton_);
        
        // Selector de entrada
        inputSelector_.addItem("Mic 1", 1);
        inputSelector_.addItem("Mic 2", 2);
        inputSelector_.addItem("Line In", 3);
        inputSelector_.addItem("MIDI", 4);
        inputSelector_.setSelectedId(1);
        inputSelector_.setColour(juce::ComboBox::backgroundColourId, FLColors::DarkBg);
        inputSelector_.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        addAndMakeVisible(inputSelector_);
        
        // Medidor de entrada
        addAndMakeVisible(inputMeter_);
        
        // Botón de configuración de audio
        audioSettingsButton_.setButtonText("⚙");
        audioSettingsButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        addAndMakeVisible(audioSettingsButton_);
        
        // Botón Snap
        snapButton_.setButtonText("SNAP");
        snapButton_.setClickingTogglesState(true);
        snapButton_.setToggleState(true, juce::dontSendNotification);
        snapButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        snapButton_.setColour(juce::TextButton::buttonOnColourId, FLColors::Blue);
        addAndMakeVisible(snapButton_);
        
        // Selector de cuantización
        quantizeSelector_.addItem("1/1", 1);
        quantizeSelector_.addItem("1/2", 2);
        quantizeSelector_.addItem("1/4", 3);
        quantizeSelector_.addItem("1/8", 4);
        quantizeSelector_.addItem("1/16", 5);
        quantizeSelector_.addItem("1/32", 6);
        quantizeSelector_.setSelectedId(3);
        addAndMakeVisible(quantizeSelector_);
        
        // Configurar el owner del input meter
        inputMeter_.setOwner(this);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(FLColors::PanelBg);
        
        // Línea separadora inferior
        g.setColour(FLColors::DarkBg);
        g.drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1, 2.0f);
        
        // Indicador de grabación parpadeante
        if (isRecording_) {
            auto alpha = (sin(juce::Time::getMillisecondCounter() * 0.005f) + 1.0f) * 0.5f;
            g.setColour(juce::Colours::red.withAlpha((float)alpha));
            g.fillEllipse(5, 5, 10, 10);
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(5);
        
        // Fila superior: controles de transporte
        auto transportArea = bounds.removeFromTop(30);
        
        recordButton_.setBounds(transportArea.removeFromLeft(60));
        transportArea.removeFromLeft(5);
        playButton_.setBounds(transportArea.removeFromLeft(50));
        transportArea.removeFromLeft(5);
        stopButton_.setBounds(transportArea.removeFromLeft(50));
        transportArea.removeFromLeft(5);
        loopButton_.setBounds(transportArea.removeFromLeft(50));
        transportArea.removeFromLeft(20);
        
        // Tempo
        tempoLabel_.setBounds(transportArea.removeFromLeft(40));
        tempoSlider_.setBounds(transportArea.removeFromLeft(150));
        transportArea.removeFromLeft(20);
        
        // Metrónomo
        metronomeButton_.setBounds(transportArea.removeFromLeft(80));
        transportArea.removeFromLeft(20);
        
        // Snap y cuantización
        snapButton_.setBounds(transportArea.removeFromLeft(60));
        transportArea.removeFromLeft(5);
        quantizeSelector_.setBounds(transportArea.removeFromLeft(70));
        
        // Resto en el lado derecho
        audioSettingsButton_.setBounds(transportArea.removeFromRight(40));
        transportArea.removeFromRight(5);
        
        // Fila inferior: entrada y medidores
        bounds.removeFromTop(5);
        auto inputArea = bounds.removeFromTop(25);
        
        juce::Label inputLabel;
        inputLabel.setText("Input:", juce::dontSendNotification);
        
        inputSelector_.setBounds(inputArea.removeFromLeft(120));
        inputArea.removeFromLeft(10);
        inputMeter_.setBounds(inputArea.removeFromLeft(200));
    }
    
    // Callbacks
    std::function<void(bool)> onRecordClicked;
    std::function<void(bool)> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void(double)> onTempoChanged;
    
    void setInputLevel(float level) {
        inputMeterLevel_ = level;
        inputMeter_.repaint();
    }
    
private:
    // Medidor de entrada simple
    class InputMeter : public juce::Component {
    public:
        InputMeter() = default;
        
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colours::black);
            
            if (!owner_) return;
            
            // Dibujar nivel
            float level = owner_->inputMeterLevel_;
            int width = (int)(getWidth() * level);
            
            juce::ColourGradient gradient(
                juce::Colours::green, 0.0f, 0.0f,
                juce::Colours::red, (float)getWidth(), 0.0f,
                false
            );
            g.setGradientFill(gradient);
            g.fillRect(0, 0, width, getHeight());
            
            // Border
            g.setColour(juce::Colours::grey);
            g.drawRect(getLocalBounds(), 1);
        }
        
        void setOwner(RecordToolbar* owner) { 
            owner_ = owner;
            repaint();
        }
    private:
        RecordToolbar* owner_ = nullptr;
    } inputMeter_;
    
    juce::TextButton recordButton_;
    juce::TextButton playButton_;
    juce::TextButton stopButton_;
    juce::TextButton loopButton_;
    juce::TextButton metronomeButton_;
    juce::TextButton audioSettingsButton_;
    juce::TextButton snapButton_;
    
    juce::Label tempoLabel_;
    juce::Slider tempoSlider_;
    
    juce::ComboBox inputSelector_;
    juce::ComboBox quantizeSelector_;
    
    bool isRecording_{false};
    bool isPlaying_{false};
    float inputMeterLevel_{0.0f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordToolbar)
};

} // namespace GUI
} // namespace OmegaStudio
