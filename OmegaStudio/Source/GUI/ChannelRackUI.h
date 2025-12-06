//==============================================================================
// ChannelRackUI.h
// Professional Channel Rack UI like FL Studio
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "../Sequencer/ChannelRack.h"
#include "FLStudioLookAndFeel.h"

namespace Omega {
namespace GUI {

//==============================================================================
// Channel Strip - Individual channel in the rack
//==============================================================================
class ChannelStripComponent : public juce::Component,
                              public juce::DragAndDropContainer {
public:
    ChannelStripComponent(int channelIndex);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    void setChannelName(const juce::String& name);
    void setChannelColor(juce::Colour color);
    void setMuted(bool shouldBeMuted);
    void setSolo(bool shouldBeSolo);
    void setVolume(float volume);
    void setPan(float pan);
    
    std::function<void()> onSoloClicked;
    std::function<void()> onMuteClicked;
    std::function<void(float)> onVolumeChanged;
    std::function<void(float)> onPanChanged;
    std::function<void()> onChannelClicked;
    
private:
    int channelIndex_;
    juce::String channelName_;
    juce::Colour channelColor_;
    bool isMuted_ = false;
    bool isSolo_ = false;
    float volume_ = 0.8f;
    float pan_ = 0.5f;
    bool isSelected_ = false;
    
    std::unique_ptr<juce::TextButton> muteButton_;
    std::unique_ptr<juce::TextButton> soloButton_;
    std::unique_ptr<juce::Slider> volumeKnob_;
    std::unique_ptr<juce::Slider> panKnob_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};

//==============================================================================
// Step Button - Individual step in the pattern grid
//==============================================================================
class StepButton : public juce::Component {
public:
    StepButton(int step, int channel);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    void setActive(bool shouldBeActive);
    void setVelocity(float velocity);
    void setHighlight(bool shouldHighlight); // For current playback position
    
    bool isActive() const { return isActive_; }
    float getVelocity() const { return velocity_; }
    
    std::function<void(int step, int channel, bool active)> onStepToggled;
    std::function<void(int step, int channel, float velocity)> onVelocityChanged;
    
private:
    int step_;
    int channel_;
    bool isActive_ = false;
    float velocity_ = 0.8f;
    bool isHighlighted_ = false;
    bool isHovered_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepButton)
};

//==============================================================================
// Pattern Grid - The step sequencer grid
//==============================================================================
class PatternGrid : public juce::Component {
public:
    PatternGrid(int numChannels, int numSteps);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setNumSteps(int steps);
    void setStep(int channel, int step, bool active, float velocity = 0.8f);
    void clearPattern();
    void randomizePattern();
    void setPlaybackPosition(int step);
    
    std::function<void(int channel, int step, bool active)> onStepChanged;
    
private:
    int numChannels_;
    int numSteps_;
    int currentPlaybackStep_ = -1;
    
    juce::OwnedArray<StepButton> stepButtons_;
    
    void createStepButtons();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternGrid)
};

//==============================================================================
// Channel Rack Window - Main Channel Rack UI
//==============================================================================
class ChannelRackUI : public juce::Component,
                      public juce::Timer {
public:
    ChannelRackUI(OmegaStudio::Sequencer::ChannelRackEngine& channelRack);
    ~ChannelRackUI() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    // Channel Management
    void addChannel(const juce::String& instrumentName);
    void removeChannel(int channelIndex);
    void duplicateChannel(int channelIndex);
    void clearChannel(int channelIndex);
    
    // Pattern Management
    void setCurrentPattern(int patternIndex);
    void newPattern();
    void duplicatePattern();
    void clearPattern();
    void randomizePattern();
    
    // Playback
    void updatePlaybackPosition(int step);
    
private:
    OmegaStudio::Sequencer::ChannelRackEngine& channelRack_;
    
    // UI Components
    std::unique_ptr<PatternGrid> patternGrid_;
    juce::OwnedArray<ChannelStripComponent> channelStrips_;
    
    std::unique_ptr<juce::ComboBox> patternSelector_;
    std::unique_ptr<juce::TextButton> addChannelButton_;
    std::unique_ptr<juce::TextButton> newPatternButton_;
    std::unique_ptr<juce::Label> patternNameLabel_;
    
    std::unique_ptr<juce::Viewport> channelViewport_;
    std::unique_ptr<juce::Component> channelContainer_;
    
    // Settings
    int currentPatternIndex_ = 0;
    int numSteps_ = 16;
    int zoom_ = 1; // 1x, 2x, 4x zoom levels
    
    // Layout
    void layoutChannels();
    void layoutPatternGrid();
    void createToolbar();
    
    // Handlers
    void onChannelStripClicked(int channelIndex);
    void onStepToggled(int channel, int step, bool active);
    void onAddChannel();
    void onNewPattern();
    void onPatternSelected();
    
    FLStudioLookAndFeel flLookAndFeel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackUI)
};

//==============================================================================
// Context Menu for Channel Rack
//==============================================================================
class ChannelRackContextMenu {
public:
    static void showChannelMenu(ChannelStripComponent* channel,
                                std::function<void(const juce::String&)> callback);
    
    static void showStepMenu(StepButton* step,
                            std::function<void(const juce::String&)> callback);
    
    static void showPatternMenu(juce::Component* owner,
                                std::function<void(const juce::String&)> callback);
};

} // namespace GUI
} // namespace Omega
