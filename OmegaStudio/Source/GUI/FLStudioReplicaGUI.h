//==============================================================================
// FLStudioReplicaGUI.h
// Réplica exacta de la GUI de FL Studio 2025
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// FL Studio Look and Feel - Réplica exacta
//==============================================================================
class FLStudioLookAndFeel : public juce::LookAndFeel_V4 {
public:
    FLStudioLookAndFeel() {
        // Colores exactos de FL Studio 2025
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff393939));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff8c1a));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc8c8c8));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffff8c1a));
        setColour(juce::Slider::trackColourId, juce::Colour(0xff5a5a5a));
        setColour(juce::Label::textColourId, juce::Colour(0xffc8c8c8));
    }
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, 
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, 
                            bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();
        
        auto baseColour = button.getToggleState() 
            ? juce::Colour(0xffff8c1a) 
            : juce::Colour(0xff4a4a4a);
        
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.1f);
        
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 2.0f);
        
        // Borde
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    }
};

//==============================================================================
// Toolbar superior estilo FL Studio
//==============================================================================
class FLStudioToolbar : public juce::Component {
public:
    FLStudioToolbar() {
        // Iconos/botones de la toolbar
        addButton("FILE", 1);
        addButton("EDIT", 2);
        addButton("ADD", 3);
        addButton("PATTERNS", 4);
        addButton("VIEW", 5);
        addButton("OPTIONS", 6);
        addButton("TOOLS", 7);
        addButton("HELP", 8);
    }
    
    void paint(juce::Graphics& g) override {
        // Fondo gris oscuro FL Studio
        g.fillAll(juce::Colour(0xff2d2d2d));
        
        // Línea inferior
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawLine(0, getHeight() - 1, (float)getWidth(), getHeight() - 1, 1.0f);
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(2, 2);
        int buttonWidth = 60;
        
        for (auto* button : buttons) {
            button->setBounds(area.removeFromLeft(buttonWidth).reduced(2));
        }
    }
    
private:
    void addButton(const juce::String& text, int id) {
        auto* btn = new juce::TextButton(text);
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb8b8b8));
        buttons.add(btn);
        addAndMakeVisible(btn);
    }
    
    juce::OwnedArray<juce::TextButton> buttons;
};

//==============================================================================
// Transport Bar estilo FL Studio
//==============================================================================
class FLStudioTransport : public juce::Component {
public:
    FLStudioTransport() {
        // Botones principales con colores FL Studio
        playBtn.setButtonText("▶");
        playBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00c853));
        playBtn.setClickingTogglesState(true);
        addAndMakeVisible(playBtn);
        
        stopBtn.setButtonText("⏹");
        stopBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4a4a));
        addAndMakeVisible(stopBtn);
        
        recBtn.setButtonText("⏺");
        recBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff1744));
        recBtn.setClickingTogglesState(true);
        addAndMakeVisible(recBtn);
        
        // Pattern/Song mode button
        patternBtn.setButtonText("PAT");
        patternBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff8c1a));
        patternBtn.setClickingTogglesState(true);
        patternBtn.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(patternBtn);
        
        songBtn.setButtonText("SONG");
        songBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff8c1a));
        songBtn.setClickingTogglesState(true);
        addAndMakeVisible(songBtn);
        
        // Tempo
        tempoLabel.setText("108.000", juce::dontSendNotification);
        tempoLabel.setJustificationType(juce::Justification::centred);
        tempoLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
        tempoLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
        addAndMakeVisible(tempoLabel);
        
        // CPU meter
        cpuLabel.setText("CPU: 12%", juce::dontSendNotification);
        cpuLabel.setJustificationType(juce::Justification::centredLeft);
        cpuLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff00));
        addAndMakeVisible(cpuLabel);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2d2d2d));
        
        // Separadores verticales
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawVerticalLine(200, 0, (float)getHeight());
        g.drawVerticalLine(400, 0, (float)getHeight());
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(5);
        
        // Grupo de transporte
        auto transportArea = area.removeFromLeft(180);
        playBtn.setBounds(transportArea.removeFromLeft(55).reduced(2));
        stopBtn.setBounds(transportArea.removeFromLeft(55).reduced(2));
        recBtn.setBounds(transportArea.removeFromLeft(55).reduced(2));
        
        area.removeFromLeft(20);
        
        // Pattern/Song
        auto modeArea = area.removeFromLeft(180);
        patternBtn.setBounds(modeArea.removeFromLeft(85).reduced(2));
        songBtn.setBounds(modeArea.removeFromLeft(85).reduced(2));
        
        area.removeFromLeft(20);
        
        // Tempo
        tempoLabel.setBounds(area.removeFromLeft(100).reduced(2));
        
        // CPU (derecha)
        cpuLabel.setBounds(area.removeFromRight(100).reduced(2));
    }
    
private:
    juce::TextButton playBtn, stopBtn, recBtn;
    juce::TextButton patternBtn, songBtn;
    juce::Label tempoLabel, cpuLabel;
};

//==============================================================================
// Channel Rack estilo FL Studio (el famoso step sequencer)
//==============================================================================
class FLStudioChannelRack : public juce::Component {
public:
    FLStudioChannelRack() {
        // Time ruler
        addAndMakeVisible(timeRuler);
        
        addChannel("Kick", juce::Colour(0xffff5252));
        addChannel("Snare", juce::Colour(0xff00e5ff));
        addChannel("HiHat", juce::Colour(0xffffeb3b));
        addChannel("Bass", juce::Colour(0xff69f0ae));
        addChannel("Lead", juce::Colour(0xffb388ff));
        addChannel("Pad", juce::Colour(0xffff80ab));
        addChannel("FX", juce::Colour(0xff40c4ff));
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff393939));
        
        // Grid vertical cada step
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int i = 0; i <= 16; ++i) {
            int x = 150 + i * 40;
            g.drawVerticalLine(x, 30, (float)getHeight());
            
            // Líneas más gruesas cada 4 steps
            if (i % 4 == 0) {
                g.setColour(juce::Colour(0xff1a1a1a));
                g.drawVerticalLine(x, 30, (float)getHeight());
                g.setColour(juce::Colour(0xff2a2a2a));
            }
        }
    }
    
    void resized() override {
        timeRuler.setBounds(150, 0, getWidth() - 150, 30);
        
        int y = 35;
        for (auto* channel : channels) {
            channel->setBounds(0, y, getWidth(), 45);
            y += 50;
        }
    }
    
private:
    class TimeRuler : public juce::Component {
    public:
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colour(0xff2d2d2d));
            
            g.setColour(juce::Colour(0xffb8b8b8));
            g.setFont(10.0f);
            
            // Números de steps
            for (int i = 1; i <= 16; ++i) {
                int x = (i - 1) * 40;
                g.drawText(juce::String(i), x, 0, 40, getHeight(), 
                          juce::Justification::centred);
            }
        }
    };
    
    class ChannelStrip : public juce::Component {
    public:
        ChannelStrip(const juce::String& name, const juce::Colour& col) : colour(col) {
            // Channel name button con color
            nameBtn.setButtonText(name);
            nameBtn.setColour(juce::TextButton::buttonColourId, colour.darker(0.3f));
            nameBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
            addAndMakeVisible(nameBtn);
            
            // Mute button
            muteBtn.setButtonText("M");
            muteBtn.setClickingTogglesState(true);
            muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
            muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6d00));
            addAndMakeVisible(muteBtn);
            
            // Solo button
            soloBtn.setButtonText("S");
            soloBtn.setClickingTogglesState(true);
            soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
            soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e676));
            addAndMakeVisible(soloBtn);
            
            // Volume slider mini
            volSlider.setRange(0.0, 1.0);
            volSlider.setValue(0.8);
            volSlider.setSliderStyle(juce::Slider::LinearHorizontal);
            volSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            volSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff5a5a5a));
            volSlider.setColour(juce::Slider::thumbColourId, colour);
            addAndMakeVisible(volSlider);
            
            // 16 steps con efecto glow
            for (int i = 0; i < 16; ++i) {
                auto* step = new juce::TextButton();
                step->setClickingTogglesState(true);
                step->setColour(juce::TextButton::buttonOnColourId, colour);
                step->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
                addAndMakeVisible(step);
                steps.add(step);
            }
        }
        
        void paint(juce::Graphics& g) override {
            // Fondo de canal alternado
            g.setColour(juce::Colour(0xff353535));
            g.fillRect(getLocalBounds());
        }
        
        void resized() override {
            auto area = getLocalBounds().reduced(2);
            
            // Sección izquierda de controles
            auto leftPanel = area.removeFromLeft(145);
            nameBtn.setBounds(leftPanel.removeFromTop(22).reduced(1));
            
            auto controls = leftPanel.removeFromTop(18);
            muteBtn.setBounds(controls.removeFromLeft(20).reduced(1));
            soloBtn.setBounds(controls.removeFromLeft(20).reduced(1));
            volSlider.setBounds(controls.reduced(1));
            
            area.removeFromLeft(3);
            
            // Steps con espaciado perfecto
            for (auto* step : steps) {
                step->setBounds(area.removeFromLeft(38).reduced(1, 3));
            }
        }
        
    private:
        juce::Colour colour;
        juce::TextButton nameBtn, muteBtn, soloBtn;
        juce::Slider volSlider;
        juce::OwnedArray<juce::TextButton> steps;
    };
    
    void addChannel(const juce::String& name, const juce::Colour& colour) {
        auto* ch = new ChannelStrip(name, colour);
        channels.add(ch);
        addAndMakeVisible(ch);
    }
    
    TimeRuler timeRuler;
    juce::OwnedArray<ChannelStrip> channels;
};

//==============================================================================
// Playlist/Arrangement estilo FL Studio
//==============================================================================
class FLStudioPlaylist : public juce::Component {
public:
    FLStudioPlaylist() {
        // Header con números de compases
        addAndMakeVisible(header);
        
        // Tracks con clips demo
        addTrack("Track 1", juce::Colour(0xffff5252), true, 200, 400);
        addTrack("Track 2", juce::Colour(0xff00e5ff), true, 450, 350);
        addTrack("Track 3", juce::Colour(0xffffeb3b), false, 0, 0);
        addTrack("Track 4", juce::Colour(0xff69f0ae), true, 100, 500);
        addTrack("Track 5", juce::Colour(0xffb388ff), true, 600, 300);
        addTrack("Track 6", juce::Colour(0xffff80ab), false, 0, 0);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a2a));
    }
    
    void resized() override {
        auto area = getLocalBounds();
        header.setBounds(area.removeFromTop(30));
        
        int y = 30;
        for (auto* track : tracks) {
            track->setBounds(0, y, getWidth(), 50);
            y += 55;
        }
    }
    
private:
    class PlaylistHeader : public juce::Component {
    public:
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colour(0xff2d2d2d));
            
            g.setColour(juce::Colour(0xffb8b8b8));
            g.setFont(10.0f);
            
            // Números de compases
            for (int i = 1; i <= 20; ++i) {
                int x = 150 + (i - 1) * 80;
                g.drawText(juce::String(i), x, 0, 80, getHeight(), 
                          juce::Justification::centredLeft);
                
                // Grid vertical
                g.setColour(i % 4 == 1 ? juce::Colour(0xff1a1a1a) : juce::Colour(0xff2a2a2a));
                g.drawVerticalLine(x, (float)getHeight(), 1000.0f);
            }
        }
    };
    
    class PlaylistTrack : public juce::Component {
    public:
        PlaylistTrack(const juce::String& name, const juce::Colour& col, 
                     bool hasClipFlag, float clipX, float clipW) 
            : colour(col), hasClip(hasClipFlag) {
            
            trackLabel.setText(name, juce::dontSendNotification);
            trackLabel.setJustificationType(juce::Justification::centred);
            trackLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff3a3a3a));
            trackLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb8b8b8));
            addAndMakeVisible(trackLabel);
            
            if (hasClip) {
                clipBounds = juce::Rectangle<float>(clipX, 8, clipW, 34);
            }
        }
        
        void paint(juce::Graphics& g) override {
            g.setColour(juce::Colour(0xff353535));
            g.fillRect(getLocalBounds());
            
            // Grid horizontal
            g.setColour(juce::Colour(0xff2a2a2a));
            g.drawLine(0, 0, (float)getWidth(), 0, 1.0f);
            
            // Grid vertical cada compás
            for (int i = 0; i <= 20; ++i) {
                int x = 150 + i * 80;
                g.setColour(i % 4 == 0 ? juce::Colour(0xff1a1a1a) : juce::Colour(0xff2a2a2a));
                g.drawVerticalLine(x, 0, (float)getHeight());
            }
            
            // Clip con waveform simulada
            if (hasClip) {
                // Clip background
                g.setColour(colour.withAlpha(0.7f));
                g.fillRoundedRectangle(clipBounds, 2.0f);
                
                // Border brillante
                g.setColour(colour);
                g.drawRoundedRectangle(clipBounds, 2.0f, 2.0f);
                
                // Waveform simulada
                g.setColour(colour.darker(0.3f));
                juce::Random rand;
                for (int x = (int)clipBounds.getX(); x < clipBounds.getRight(); x += 3) {
                    float h = rand.nextFloat() * clipBounds.getHeight() * 0.6f;
                    float y = clipBounds.getCentreY() - h * 0.5f;
                    g.drawLine((float)x, y, (float)x, y + h, 1.5f);
                }
                
                // Clip name
                g.setColour(juce::Colours::white);
                g.setFont(10.0f);
                g.drawText("Pattern", clipBounds.reduced(5, 2), 
                          juce::Justification::topLeft, false);
            }
        }
        
        void resized() override {
            trackLabel.setBounds(0, 0, 140, getHeight());
        }
        
    private:
        juce::Colour colour;
        juce::Label trackLabel;
        bool hasClip = false;
        juce::Rectangle<float> clipBounds;
    };
    
    void addTrack(const juce::String& name, const juce::Colour& colour, 
                 bool hasClip, float clipX, float clipW) {
        auto* track = new PlaylistTrack(name, colour, hasClip, clipX, clipW);
        tracks.add(track);
        addAndMakeVisible(track);
    }
    
    PlaylistHeader header;
    juce::OwnedArray<PlaylistTrack> tracks;
};

//==============================================================================
// Ventana Principal - Réplica exacta de FL Studio
//==============================================================================
class FLStudioReplicaWindow : public juce::Component {
public:
    FLStudioReplicaWindow() {
        setLookAndFeel(&flLookAndFeel);
        
        addAndMakeVisible(toolbar);
        addAndMakeVisible(transport);
        
        // Tabs estilo FL Studio
        tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
        tabs->setTabBarDepth(30);
        tabs->setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff393939));
        tabs->setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0xff2a2a2a));
        tabs->setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colour(0xffff8c1a));
        
        channelRack = std::make_unique<FLStudioChannelRack>();
        playlist = std::make_unique<FLStudioPlaylist>();
        
        tabs->addTab("CHANNEL RACK", juce::Colour(0xff393939), channelRack.get(), false);
        tabs->addTab("PLAYLIST", juce::Colour(0xff393939), playlist.get(), false);
        tabs->addTab("PIANO ROLL", juce::Colour(0xff393939), new juce::Component(), true);
        tabs->addTab("MIXER", juce::Colour(0xff393939), new juce::Component(), true);
        
        addAndMakeVisible(tabs.get());
        
        setSize(1400, 900);
    }
    
    ~FLStudioReplicaWindow() override {
        setLookAndFeel(nullptr);
        tabs->clearTabs();
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff393939));
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        toolbar.setBounds(area.removeFromTop(32));
        transport.setBounds(area.removeFromTop(50));
        
        tabs->setBounds(area);
    }
    
private:
    FLStudioLookAndFeel flLookAndFeel;
    FLStudioToolbar toolbar;
    FLStudioTransport transport;
    std::unique_ptr<juce::TabbedComponent> tabs;
    std::unique_ptr<FLStudioChannelRack> channelRack;
    std::unique_ptr<FLStudioPlaylist> playlist;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudioReplicaWindow)
};

} // namespace GUI
} // namespace OmegaStudio
