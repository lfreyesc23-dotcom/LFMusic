/*
  ==============================================================================
    MixerEditor.h
    
    Mixer Visual completo estilo FL Studio
    - Channel strips con peak meters animados
    - 8 plugin slots por canal (pre/post fader)
    - Send knobs visuales con routing
    - EQ curves dibujables
    - Master section profesional
    - Routing visual con líneas
    - Right-click menus
    - Drag & drop de plugins
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Mixer/MixerEngine.h"
#include "../Audio/Plugins/PluginManager.h"
#include <memory>
#include <vector>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/** Peak Meter animado */
class PeakMeterComponent : public juce::Component,
                          public juce::Timer {
public:
    PeakMeterComponent();
    ~PeakMeterComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void setPeakLevel(float leftPeak, float rightPeak);
    void setRMSLevel(float leftRMS, float rightRMS);
    void reset();
    
private:
    void paintMeterChannel(juce::Graphics& g, juce::Rectangle<float> area, 
                          float peak, float rms, float peakHold);
    
    float leftPeak_ = 0.0f;
    float rightPeak_ = 0.0f;
    float leftRMS_ = 0.0f;
    float rightRMS_ = 0.0f;
    float leftPeakHold_ = 0.0f;
    float rightPeakHold_ = 0.0f;
    int peakHoldCounter_ = 0;
    
    static constexpr float PEAK_DECAY = 0.95f;
    static constexpr int PEAK_HOLD_TIME = 60; // frames
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakMeterComponent)
};

//==============================================================================
/** Plugin slot visual */
class PluginSlotComponent : public juce::Component {
public:
    PluginSlotComponent(int slotIndex);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    void setPlugin(std::shared_ptr<PluginInstance> plugin);
    std::shared_ptr<PluginInstance> getPlugin() const { return plugin_; }
    
    bool isEmpty() const { return plugin_ == nullptr; }
    int getSlotIndex() const { return slotIndex_; }
    
    std::function<void(int slotIndex)> onDoubleClick;
    std::function<void(int slotIndex)> onRightClick;
    
private:
    int slotIndex_;
    std::shared_ptr<PluginInstance> plugin_;
    bool hovering_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginSlotComponent)
};

//==============================================================================
/** Knob rotatorio estilo FL Studio */
class MixerKnob : public juce::Component {
public:
    MixerKnob(const juce::String& label);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    void setValue(float value);
    float getValue() const { return value_; }
    
    void setRange(float min, float max);
    void setDefaultValue(float defaultVal) { defaultValue_ = defaultVal; }
    
    std::function<void(float)> onValueChanged;
    
private:
    juce::String label_;
    float value_ = 0.5f;
    float defaultValue_ = 0.5f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    juce::Point<int> dragStart_;
    float dragStartValue_ = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerKnob)
};

//==============================================================================
/** Channel Strip completo */
class ChannelStripComponent : public juce::Component,
                             public juce::Timer {
public:
    ChannelStripComponent(int channelIndex);
    ~ChannelStripComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void setChannelName(const juce::String& name);
    juce::String getChannelName() const { return channelName_; }
    
    void setColour(juce::Colour colour);
    juce::Colour getColour() const { return channelColour_; }
    
    void setSelected(bool selected);
    bool isSelected() const { return selected_; }
    
    // Audio levels
    void updateMeters(float leftPeak, float rightPeak, float leftRMS, float rightRMS);
    
    // Controls
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    void setPan(float pan);
    float getPan() const { return pan_; }
    
    void setSolo(bool solo);
    bool isSolo() const { return solo_; }
    
    void setMute(bool mute);
    bool isMute() const { return mute_; }
    
    void setArm(bool arm);
    bool isArm() const { return arm_; }
    
    // Sends
    void setSendLevel(int sendIndex, float level);
    float getSendLevel(int sendIndex) const;
    
    // Plugin slots
    void setPlugin(int slotIndex, std::shared_ptr<PluginInstance> plugin);
    std::shared_ptr<PluginInstance> getPlugin(int slotIndex) const;
    
    // Callbacks
    std::function<void(int channel)> onChannelSelected;
    std::function<void(int channel, float volume)> onVolumeChanged;
    std::function<void(int channel, float pan)> onPanChanged;
    std::function<void(int channel, bool solo)> onSoloChanged;
    std::function<void(int channel, bool mute)> onMuteChanged;
    std::function<void(int channel, int slot)> onPluginSlotClicked;
    
private:
    int channelIndex_;
    juce::String channelName_;
    juce::Colour channelColour_;
    bool selected_ = false;
    
    // Controls
    float volume_ = 0.8f; // 0dB
    float pan_ = 0.5f; // Center
    bool solo_ = false;
    bool mute_ = false;
    bool arm_ = false;
    
    // Sends (4 sends)
    std::array<float, 4> sendLevels_;
    
    // Components
    std::unique_ptr<PeakMeterComponent> peakMeter_;
    std::unique_ptr<juce::Slider> volumeSlider_;
    std::unique_ptr<MixerKnob> panKnob_;
    std::array<std::unique_ptr<MixerKnob>, 4> sendKnobs_;
    std::array<std::unique_ptr<PluginSlotComponent>, 8> pluginSlots_;
    
    std::unique_ptr<juce::TextButton> soloButton_;
    std::unique_ptr<juce::TextButton> muteButton_;
    std::unique_ptr<juce::TextButton> armButton_;
    
    std::unique_ptr<juce::Label> nameLabel_;
    
    void setupComponents();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};

//==============================================================================
/** Master Section */
class MasterStripComponent : public juce::Component,
                            public juce::Timer {
public:
    MasterStripComponent();
    ~MasterStripComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void updateMeters(float leftPeak, float rightPeak, float leftRMS, float rightRMS);
    
    void setVolume(float volume);
    float getVolume() const { return volume_; }
    
    std::function<void(float volume)> onVolumeChanged;
    
private:
    float volume_ = 0.8f;
    
    std::unique_ptr<PeakMeterComponent> peakMeter_;
    std::unique_ptr<juce::Slider> volumeSlider_;
    std::unique_ptr<juce::Label> nameLabel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterStripComponent)
};

//==============================================================================
/** Mixer completo con routing visual */
class MixerEditor : public juce::Component,
                   public juce::Timer {
public:
    MixerEditor();
    ~MixerEditor() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    // Channels management
    void setNumChannels(int numChannels);
    int getNumChannels() const { return (int)channelStrips_.size(); }
    
    void selectChannel(int channelIndex);
    int getSelectedChannel() const { return selectedChannel_; }
    
    // Channel strips access
    ChannelStripComponent* getChannelStrip(int index);
    MasterStripComponent* getMasterStrip() { return masterStrip_.get(); }
    
    // Routing visualization
    void setRoutingVisible(bool visible);
    bool isRoutingVisible() const { return routingVisible_; }
    
    // Mixer engine integration
    void setMixerEngine(OmegaStudio::MixerEngine* engine);
    OmegaStudio::MixerEngine* getMixerEngine() const { return mixerEngine_; }
    
    // Update from engine
    void updateFromEngine();
    
private:
    std::vector<std::unique_ptr<ChannelStripComponent>> channelStrips_;
    std::unique_ptr<MasterStripComponent> masterStrip_;
    
    std::unique_ptr<juce::Viewport> channelViewport_;
    std::unique_ptr<juce::Component> channelContainer_;
    
    int selectedChannel_ = -1;
    bool routingVisible_ = false;
    
    OmegaStudio::MixerEngine* mixerEngine_ = nullptr;
    
    static constexpr int CHANNEL_STRIP_WIDTH = 80;
    static constexpr int MASTER_STRIP_WIDTH = 100;
    
    void setupChannelStrips();
    void paintRoutingLines(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerEditor)
};

//==============================================================================
/** Mixer Window */
class MixerWindow : public juce::DocumentWindow {
public:
    MixerWindow(const juce::String& name);
    ~MixerWindow() override;
    
    void closeButtonPressed() override;
    
    MixerEditor* getEditor() { return editor_; }
    
private:
    MixerEditor* editor_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerWindow)
};

} // namespace GUI
} // namespace OmegaStudio
