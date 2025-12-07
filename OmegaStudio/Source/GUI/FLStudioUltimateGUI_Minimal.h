#pragma once

#include <JuceHeader.h>
#include "InteractiveChannelRack.h"
#include "InteractivePianoRoll.h"
#include "InteractiveMixer.h"
#include "InteractiveBrowser.h"
#include "InteractivePlaylist.h"

namespace OmegaStudio::GUI {

/**
 * @brief FL STUDIO 2025 ULTIMATE - COMPLETAMENTE FUNCIONAL
 * 
 * Todos los componentes son INTERACTIVOS y USABLES:
 * ✅ Channel Rack - Steps clickeables, secuenciador funcional
 * ✅ Piano Roll - Edita notas MIDI, drag & drop
 * ✅ Mixer - Faders deslizables, mute/solo/pan
 * ✅ Browser - Drag & drop samples, ratings, búsqueda
 * ✅ Playlist - Clips arrastrables, zoom, edición
 * ✅ Workspaces - F5=Recording, F6=Production, F7=Mixing, F8=Mastering
 */
class FLStudioUltimateGUI : public juce::Component,
                             public juce::KeyListener,
                             private juce::Timer
{
public:
    FLStudioUltimateGUI()
    {
        setSize(1920, 1080);
        
        // Create all interactive components
        channelRack_ = std::make_unique<InteractiveChannelRack>();
        addChildComponent(channelRack_.get());
        
        pianoRoll_ = std::make_unique<InteractivePianoRoll>();
        addChildComponent(pianoRoll_.get());
        
        mixer_ = std::make_unique<InteractiveMixer>();
        addChildComponent(mixer_.get());
        
        browser_ = std::make_unique<InteractiveBrowser>();
        addAndMakeVisible(browser_.get());
        
        playlist_ = std::make_unique<InteractivePlaylist>();
        addChildComponent(playlist_.get());
        
        // Play/Stop buttons
        playButton_.setButtonText("▶ PLAY");
        playButton_.onClick = [this]() { handlePlay(); };
        addAndMakeVisible(playButton_);
        
        stopButton_.setButtonText("■ STOP");
        stopButton_.onClick = [this]() { handleStop(); };
        addAndMakeVisible(stopButton_);
        
        // BPM slider
        bpmSlider_.setRange(60, 200, 1);
        bpmSlider_.setValue(120);
        bpmSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        bpmSlider_.onValueChange = [this]() {
            channelRack_->setBPM(bpmSlider_.getValue());
        };
        addAndMakeVisible(bpmSlider_);
        
        // Setup key listeners for F5-F12 workspaces
        addKeyListener(this);
        setWantsKeyboardFocus(true);
        
        // Default workspace: Production (F6)
        switchToWorkspace(1, "Production");
        
        // Start CPU/RAM monitoring
        startTimerHz(2); // 2 times per second
        
        DBG("╔═══════════════════════════════════════════════════════════╗");
        DBG("║   FL STUDIO 2025 ULTIMATE - FULLY FUNCTIONAL              ║");
        DBG("║   ✅ Interactive Channel Rack with steps                   ║");
        DBG("║   ✅ Interactive Piano Roll with note editing              ║");
        DBG("║   ✅ Interactive Mixer with faders                         ║");
        DBG("║   ✅ Interactive Browser with drag & drop                  ║");
        DBG("║   ✅ Interactive Playlist with clips                       ║");
        DBG("╚═══════════════════════════════════════════════════════════╝");
    }
    
    ~FLStudioUltimateGUI() override
    {
        removeKeyListener(this);
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override
    {
        // FL Studio 2025 dark theme
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Top bar with gradient
        auto topBar = getLocalBounds().removeFromTop(50);
        juce::ColourGradient gradient(juce::Colour(0xff3d3d3d), 0, 0,
                                     juce::Colour(0xff2d2d2d), 0, 50.0f, false);
        g.setGradientFill(gradient);
        g.fillRect(topBar);
        
        // Title
        g.setColour(juce::Colour(0xffff8c00));
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText("🎹 FL STUDIO 2025 ULTIMATE", 
                  topBar.reduced(15, 0), 
                  juce::Justification::centredLeft);
        
        // Workspace indicator
        juce::String workspaceName;
        switch (currentWorkspace_)
        {
            case 0: workspaceName = "RECORDING (F5)"; break;
            case 1: workspaceName = "PRODUCTION (F6)"; break;
            case 2: workspaceName = "MIXING (F7)"; break;
            case 3: workspaceName = "MASTERING (F8)"; break;
            default: workspaceName = "PRODUCTION"; break;
        }
        
        g.setColour(juce::Colour(0xff00ff00));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(workspaceName, topBar.removeFromRight(200).reduced(5, 0),
                  juce::Justification::centredRight);
        
        // Bottom status bar
        auto bottomBar = getLocalBounds().removeFromBottom(30);
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillRect(bottomBar);
        
        // CPU/RAM meters
        g.setColour(juce::Colour(0xffaaaaaa));
        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        
        auto cpuText = juce::String::formatted("CPU: %.1f%%", cpuLoad_ * 100.0);
        g.drawText(cpuText, bottomBar.removeFromLeft(100).reduced(10, 0),
                  juce::Justification::centredLeft);
        
        auto ramText = juce::String::formatted("RAM: %.1f%%", ramUsage_ * 100.0);
        g.drawText(ramText, bottomBar.removeFromLeft(100).reduced(10, 0),
                  juce::Justification::centredLeft);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Top bar
        auto topBar = bounds.removeFromTop(50);
        
        // Transport controls in top bar
        auto transportArea = topBar.removeFromRight(400).reduced(5, 10);
        playButton_.setBounds(transportArea.removeFromLeft(80));
        stopButton_.setBounds(transportArea.removeFromLeft(80));
        transportArea.removeFromLeft(10); // Spacing
        bpmSlider_.setBounds(transportArea.removeFromLeft(150));
        
        // Bottom status bar
        bounds.removeFromBottom(30);
        
        // Browser on left (always visible)
        browser_->setBounds(bounds.removeFromLeft(300));
        
        // Layout based on workspace
        layoutWorkspace(bounds);
    }
    
    // ===== WORKSPACE SHORTCUTS (F5-F12) =====
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override
    {
        if (key == juce::KeyPress::F5Key)
        {
            switchToWorkspace(0, "Recording");
            return true;
        }
        else if (key == juce::KeyPress::F6Key)
        {
            switchToWorkspace(1, "Production");
            return true;
        }
        else if (key == juce::KeyPress::F7Key)
        {
            switchToWorkspace(2, "Mixing");
            return true;
        }
        else if (key == juce::KeyPress::F8Key)
        {
            switchToWorkspace(3, "Mastering");
            return true;
        }
        
        return false;
    }
    
    void switchToWorkspace(int id, const juce::String& name)
    {
        currentWorkspace_ = id;
        DBG("🚀 Switched to workspace " << id << ": " << name);
        resized();
        repaint();
    }
    
    void updatePerformance(double cpuLoad, double ramUsage)
    {
        cpuLoad_ = cpuLoad;
        ramUsage_ = ramUsage;
    }

private:
    void layoutWorkspace(juce::Rectangle<int> bounds)
    {
        switch (currentWorkspace_)
        {
            case 0: layoutRecordingWorkspace(bounds); break;
            case 1: layoutProductionWorkspace(bounds); break;
            case 2: layoutMixingWorkspace(bounds); break;
            case 3: layoutMasteringWorkspace(bounds); break;
        }
    }
    
    void layoutRecordingWorkspace(juce::Rectangle<int> bounds)
    {
        // Recording: Playlist top + Mixer bottom
        playlist_->setVisible(true);
        mixer_->setVisible(true);
        channelRack_->setVisible(false);
        pianoRoll_->setVisible(false);
        
        auto topHalf = bounds.removeFromTop(bounds.getHeight() / 2);
        playlist_->setBounds(topHalf);
        mixer_->setBounds(bounds);
    }
    
    void layoutProductionWorkspace(juce::Rectangle<int> bounds)
    {
        // Production: Channel Rack top + Piano Roll bottom
        channelRack_->setVisible(true);
        pianoRoll_->setVisible(true);
        playlist_->setVisible(false);
        mixer_->setVisible(false);
        
        auto topHalf = bounds.removeFromTop(bounds.getHeight() / 2);
        channelRack_->setBounds(topHalf);
        pianoRoll_->setBounds(bounds);
    }
    
    void layoutMixingWorkspace(juce::Rectangle<int> bounds)
    {
        // Mixing: Full mixer
        mixer_->setVisible(true);
        channelRack_->setVisible(false);
        pianoRoll_->setVisible(false);
        playlist_->setVisible(false);
        
        mixer_->setBounds(bounds);
    }
    
    void layoutMasteringWorkspace(juce::Rectangle<int> bounds)
    {
        // Mastering: Mixer + Playlist
        mixer_->setVisible(true);
        playlist_->setVisible(true);
        channelRack_->setVisible(false);
        pianoRoll_->setVisible(false);
        
        auto mixerArea = bounds.removeFromRight(bounds.getWidth() / 2);
        mixer_->setBounds(mixerArea);
        playlist_->setBounds(bounds);
    }
    
    void handlePlay()
    {
        DBG("▶ PLAY");
        channelRack_->play();
        playlist_->play();
    }
    
    void handleStop()
    {
        DBG("■ STOP");
        channelRack_->stop();
        playlist_->stop();
    }
    
    void timerCallback() override
    {
        // Simulate CPU/RAM (in real app, get from audio engine)
        cpuLoad_ = juce::Random::getSystemRandom().nextFloat() * 0.3f;
        ramUsage_ = 0.4f + juce::Random::getSystemRandom().nextFloat() * 0.2f;
        repaint();
    }
    
    // Interactive components
    std::unique_ptr<InteractiveChannelRack> channelRack_;
    std::unique_ptr<InteractivePianoRoll> pianoRoll_;
    std::unique_ptr<InteractiveMixer> mixer_;
    std::unique_ptr<InteractiveBrowser> browser_;
    std::unique_ptr<InteractivePlaylist> playlist_;
    
    // Transport controls
    juce::TextButton playButton_;
    juce::TextButton stopButton_;
    juce::Slider bpmSlider_;
    
    // State
    int currentWorkspace_{1};
    double cpuLoad_{0.0};
    double ramUsage_{0.0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudioUltimateGUI)
};

} // namespace OmegaStudio::GUI
