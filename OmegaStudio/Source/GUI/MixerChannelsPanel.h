#pragma once

#include <JuceHeader.h>
#include "FLStudioLookAndFeel.h"

namespace OmegaStudio {
namespace GUI {

using Omega::GUI::FLColors;

//==============================================================================
/** 
 * Canal individual del mixer estilo FL Studio
 */
class MixerChannelStrip : public juce::Component {
public:
    MixerChannelStrip(int channelNumber) : channelNum_(channelNumber) {
        // Nombre del canal
        nameLabel_.setText("Channel " + juce::String(channelNumber), juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        nameLabel_.setEditable(true);
        addAndMakeVisible(nameLabel_);
        
        // Botón Mute
        muteButton_.setButtonText("M");
        muteButton_.setClickingTogglesState(true);
        muteButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        muteButton_.setColour(juce::TextButton::buttonOnColourId, FLColors::Orange);
        muteButton_.onClick = [this]() {
            if (onMuteChanged) onMuteChanged(muteButton_.getToggleState());
        };
        addAndMakeVisible(muteButton_);
        
        // Botón Solo
        soloButton_.setButtonText("S");
        soloButton_.setClickingTogglesState(true);
        soloButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        soloButton_.setColour(juce::TextButton::buttonOnColourId, FLColors::Green);
        soloButton_.onClick = [this]() {
            if (onSoloChanged) onSoloChanged(soloButton_.getToggleState());
        };
        addAndMakeVisible(soloButton_);
        
        // Botón Record
        recordButton_.setButtonText("R");
        recordButton_.setClickingTogglesState(true);
        recordButton_.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        recordButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        addAndMakeVisible(recordButton_);
        
        // Fader de volumen
        volumeFader_.setSliderStyle(juce::Slider::LinearVertical);
        volumeFader_.setRange(-60.0, 6.0, 0.1);
        volumeFader_.setValue(0.0);
        volumeFader_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        volumeFader_.setColour(juce::Slider::trackColourId, FLColors::DarkBg);
        volumeFader_.setColour(juce::Slider::thumbColourId, FLColors::Orange);
        volumeFader_.onValueChange = [this]() {
            if (onVolumeChanged) onVolumeChanged((float)volumeFader_.getValue());
        };
        addAndMakeVisible(volumeFader_);
        
        // Knob de Pan
        panKnob_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob_.setRange(-1.0, 1.0, 0.01);
        panKnob_.setValue(0.0);
        panKnob_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        panKnob_.setColour(juce::Slider::rotarySliderFillColourId, FLColors::Cyan);
        panKnob_.onValueChange = [this]() {
            if (onPanChanged) onPanChanged((float)panKnob_.getValue());
        };
        addAndMakeVisible(panKnob_);
        
        // Label para Pan
        panLabel_.setText("PAN", juce::dontSendNotification);
        panLabel_.setJustificationType(juce::Justification::centred);
        panLabel_.setColour(juce::Label::textColourId, juce::Colours::grey);
        panLabel_.setFont(juce::FontOptions(10.0f));
        addAndMakeVisible(panLabel_);
        
        // Medidor de nivel
        addAndMakeVisible(levelMeter_);
        levelMeter_.setChannelNumber(channelNumber);
        
        // Selector de entrada
        inputSelector_.addItem("None", 1);
        inputSelector_.addItem("Mic", 2);
        inputSelector_.addItem("Line", 3);
        inputSelector_.addItem("MIDI", 4);
        inputSelector_.setSelectedId(1);
        inputSelector_.setColour(juce::ComboBox::backgroundColourId, FLColors::DarkBg);
        addAndMakeVisible(inputSelector_);
        
        // Botón de efectos
        fxButton_.setButtonText("FX");
        fxButton_.setColour(juce::TextButton::buttonColourId, FLColors::Blue);
        fxButton_.onClick = [this]() {
            if (onFxClicked) onFxClicked();
        };
        addAndMakeVisible(fxButton_);
    }
    
    void paint(juce::Graphics& g) override {
        // Fondo del canal
        g.fillAll(FLColors::PanelBg);
        
        // Border
        g.setColour(FLColors::DarkBg);
        g.drawRect(getLocalBounds(), 1);
        
        // Indicador de canal activo
        if (isActive_) {
            g.setColour(FLColors::Orange.withAlpha(0.3f));
            g.fillRect(getLocalBounds().reduced(2));
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(5);
        
        // Nombre del canal en la parte superior
        nameLabel_.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        // Selector de entrada
        inputSelector_.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(5);
        
        // Botones M/S/R
        auto buttonArea = bounds.removeFromTop(25);
        muteButton_.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 3).reduced(2));
        soloButton_.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(2));
        recordButton_.setBounds(buttonArea.reduced(2));
        bounds.removeFromTop(5);
        
        // Medidor de nivel (en el lado izquierdo del fader)
        auto meterAndFaderArea = bounds.removeFromTop(bounds.getHeight() - 100);
        levelMeter_.setBounds(meterAndFaderArea.removeFromLeft(20));
        meterAndFaderArea.removeFromLeft(5);
        
        // Fader de volumen (ocupa la mayor parte del espacio)
        volumeFader_.setBounds(meterAndFaderArea);
        
        // Pan knob
        panLabel_.setBounds(bounds.removeFromTop(15));
        panKnob_.setBounds(bounds.removeFromTop(60));
        bounds.removeFromTop(5);
        
        // Botón FX
        fxButton_.setBounds(bounds.removeFromTop(25));
    }
    
    void setActive(bool active) { 
        isActive_ = active;
        repaint();
    }
    
    void setLevel(float leftLevel, float rightLevel) {
        levelMeter_.setLevels(leftLevel, rightLevel);
    }
    
    void setChannelName(const juce::String& name) {
        nameLabel_.setText(name, juce::dontSendNotification);
    }
    
    // Callbacks
    std::function<void(bool)> onMuteChanged;
    std::function<void(bool)> onSoloChanged;
    std::function<void(float)> onVolumeChanged;
    std::function<void(float)> onPanChanged;
    std::function<void()> onFxClicked;
    
private:
    class LevelMeter : public juce::Component,
                       private juce::Timer {
    public:
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colours::black);
            
            auto bounds = getLocalBounds().toFloat();
            
            // Dibujar nivel izquierdo
            float leftHeight = bounds.getHeight() * leftLevel_;
            g.setColour(getLevelColor(leftLevel_));
            g.fillRect(bounds.removeFromLeft(bounds.getWidth() / 2).removeFromBottom(leftHeight));
            
            // Dibujar nivel derecho
            float rightHeight = bounds.getHeight() * rightLevel_;
            g.setColour(getLevelColor(rightLevel_));
            g.fillRect(bounds.removeFromBottom(rightHeight));
            
            // Border
            g.setColour(juce::Colours::grey);
            g.drawRect(getLocalBounds().toFloat(), 1.0f);
        }
        
        void setLevels(float left, float right) {
            leftLevel_ = juce::jlimit(0.0f, 1.0f, left);
            rightLevel_ = juce::jlimit(0.0f, 1.0f, right);
            repaint();
        }
        
        void setChannelNumber(int num) {
            channelNum_ = num;
        }
        
        void timerCallback() override {
            // Decay de los niveles
            leftLevel_ *= 0.95f;
            rightLevel_ *= 0.95f;
            repaint();
        }
        
    private:
        juce::Colour getLevelColor(float level) {
            if (level > 0.9f) return juce::Colours::red;
            if (level > 0.7f) return juce::Colours::yellow;
            return juce::Colours::green;
        }
        
        float leftLevel_{0.0f};
        float rightLevel_{0.0f};
        int channelNum_{0};
    } levelMeter_;
    
    int channelNum_;
    bool isActive_{false};
    
    juce::Label nameLabel_;
    juce::Label panLabel_;
    juce::TextButton muteButton_;
    juce::TextButton soloButton_;
    juce::TextButton recordButton_;
    juce::TextButton fxButton_;
    juce::Slider volumeFader_;
    juce::Slider panKnob_;
    juce::ComboBox inputSelector_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelStrip)
};

//==============================================================================
/** 
 * Panel de mixer con múltiples canales estilo FL Studio
 */
class MixerChannelsPanel : public juce::Component {
public:
    MixerChannelsPanel(int numChannels = 8) {
        setNumChannels(numChannels);
        
        // Viewport para scroll
        viewport_.setViewedComponent(&channelsContainer_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(FLColors::DarkBg);
        
        // Título
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("MIXER", 10, 5, getWidth() - 20, 25, juce::Justification::centredLeft);
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(30); // Espacio para título
        
        viewport_.setBounds(bounds);
        
        // Calcular tamaño del contenedor de canales
        int channelWidth = 80;
        int totalWidth = channelWidth * channels_.size();
        channelsContainer_.setBounds(0, 0, totalWidth, bounds.getHeight());
        
        // Posicionar canales
        int x = 0;
        for (auto* channel : channels_) {
            channel->setBounds(x, 0, channelWidth, bounds.getHeight());
            x += channelWidth;
        }
    }
    
    void setNumChannels(int num) {
        channels_.clear();
        channelsContainer_.deleteAllChildren();
        
        for (int i = 0; i < num; ++i) {
            auto* channel = new MixerChannelStrip(i + 1);
            channel->setChannelName("Ch " + juce::String(i + 1));
            
            channel->onVolumeChanged = [i](float vol) {
                DBG("Channel " << (i + 1) << " volume: " << vol << " dB");
            };
            
            channel->onPanChanged = [i](float pan) {
                DBG("Channel " << (i + 1) << " pan: " << pan);
            };
            
            channel->onMuteChanged = [i](bool muted) {
                DBG("Channel " << (i + 1) << " muted: " << muted);
            };
            
            channel->onSoloChanged = [i](bool soloed) {
                DBG("Channel " << (i + 1) << " soloed: " << soloed);
            };
            
            channel->onFxClicked = [i]() {
                DBG("Channel " << (i + 1) << " FX clicked");
            };
            
            channelsContainer_.addAndMakeVisible(channel);
            channels_.add(channel);
        }
        
        resized();
    }
    
    MixerChannelStrip* getChannel(int index) {
        if (juce::isPositiveAndBelow(index, channels_.size()))
            return channels_[index];
        return nullptr;
    }
    
private:
    juce::Component channelsContainer_;
    juce::Viewport viewport_;
    juce::OwnedArray<MixerChannelStrip> channels_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelsPanel)
};

} // namespace GUI
} // namespace OmegaStudio
