#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @brief Channel in the Channel Rack (like FL Studio)
 * Can be instrument, audio clip, or layer
 */
struct Channel {
    enum class Type {
        Instrument,  // VST/AU or built-in synth
        AudioClip,   // Sample player
        Layer,       // Multi-instrument layer
        Split        // Keyboard split
    };
    
    int id{0};
    juce::String name{"Channel"};
    Type type{Type::Instrument};
    juce::Colour colour{juce::Colours::orange};
    
    // Instrument data
    juce::String pluginIdentifier;  // For VST/AU
    juce::String instrumentType;     // For built-in: "ProSampler", "ProSynth", etc.
    
    // Audio clip
    juce::File audioFile;
    double sampleStart{0.0};
    double sampleEnd{-1.0};  // -1 = full length
    
    // Step sequencer pattern
    std::vector<bool> steps;  // 16-step pattern
    std::vector<int> velocities;  // Per-step velocity
    
    // Mixer routing
    int mixerTrack{0};
    
    // Settings
    int midiChannel{0};    // 0 = omni
    int rootNote{60};      // C4
    int keyZoneLow{0};     // For split
    int keyZoneHigh{127};
    bool isMuted{false};
    bool isSoloed{false};
    
    // Layer/Split children
    std::vector<int> childChannelIds;
    
    Channel() {
        steps.resize(16, false);
        velocities.resize(16, 100);
    }
};

/**
 * @brief Channel Rack Engine (FL Studio-style)
 */
class ChannelRackEngine {
public:
    ChannelRackEngine();
    ~ChannelRackEngine();
    
    // Channel Management
    int addChannel(const juce::String& name, Channel::Type type);
    void removeChannel(int channelId);
    void moveChannel(int channelId, int newPosition);
    Channel* getChannel(int channelId);
    const std::vector<std::shared_ptr<Channel>>& getChannels() const { return channels_; }
    
    // Layer/Split Management
    int createLayer(const juce::String& name, const std::vector<int>& childIds);
    int createSplit(const juce::String& name);
    void addToLayer(int layerId, int channelId);
    void removeFromLayer(int layerId, int channelId);
    void setSplitZone(int channelId, int lowKey, int highKey);
    
    // Step Sequencer
    void setStep(int channelId, int stepIndex, bool active);
    void setStepVelocity(int channelId, int stepIndex, int velocity);
    void clearSteps(int channelId);
    void fillSteps(int channelId);
    void randomizeSteps(int channelId, float probability = 0.5f);
    
    // Pattern presets
    void loadStepPattern(int channelId, const juce::String& patternName);
    std::vector<juce::String> getAvailablePatterns() const;
    
    // Playback
    void prepareToPlay(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);
    void advanceStep();  // Called on each 16th note
    int getCurrentStep() const { return currentStep_; }
    void setStepLength(int steps) { stepLength_ = steps; }  // 16, 32, 64, etc.
    
    // MIDI routing
    void routeMidiToChannel(int channelId, const juce::MidiMessage& message);
    
    // Cloning & Presets
    int cloneChannel(int sourceId);
    void saveChannelPreset(int channelId, const juce::File& file);
    int loadChannelPreset(const juce::File& file);
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    std::vector<std::shared_ptr<Channel>> channels_;
    int nextChannelId_{1};
    
    // Playback state
    int currentStep_{0};
    int stepLength_{16};
    double sampleRate_{44100.0};
    int blockSize_{512};
    
    // Step patterns library
    std::map<juce::String, std::vector<bool>> stepPatterns_;
    
    void initializePatterns();
    void triggerChannelNote(Channel* channel, int velocity, juce::MidiBuffer& midi, int sampleOffset);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackEngine)
};

/**
 * @brief Channel Rack GUI Component (FL Studio-style grid)
 */
class ChannelRackComponent : public juce::Component,
                              public juce::DragAndDropContainer,
                              public juce::DragAndDropTarget {
public:
    ChannelRackComponent(ChannelRackEngine& engine);
    ~ChannelRackComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    // Drag & Drop
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& details) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& details) override;
    
    // View controls
    void setStepSize(int pixels) { stepSize_ = pixels; repaint(); }
    void setShowVelocity(bool show) { showVelocity_ = show; repaint(); }
    void setStepLength(int steps);
    
private:
    ChannelRackEngine& engine_;
    
    // View settings
    int stepSize_{40};
    int rowHeight_{50};
    bool showVelocity_{true};
    int scrollY_{0};
    
    // Interaction
    int selectedChannelId_{-1};
    int hoveredChannel_{-1};
    int hoveredStep_{-1};
    
    enum class Tool { Draw, Erase, Select };
    Tool currentTool_{Tool::Draw};
    
    // Rendering
    void drawChannelList(juce::Graphics& g);
    void drawStepGrid(juce::Graphics& g);
    void drawSteps(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    
    // Hit testing
    int getChannelAtY(int y);
    int getStepAtX(int x);
    
    // Context menu
    void showChannelMenu(int channelId);
    void showStepMenu(int channelId, int stepIndex);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackComponent)
};

/**
 * @brief Step Sequencer Cell (individual step button)
 */
class StepButton : public juce::Component {
public:
    StepButton(int channelId, int stepIndex);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    
    void setActive(bool active);
    void setVelocity(int velocity);
    void setAccent(bool accent);
    
    std::function<void(int, int, bool)> onStepChanged;
    
private:
    int channelId_;
    int stepIndex_;
    bool isActive_{false};
    int velocity_{100};
    bool isAccent_{false};
    bool isHovered_{false};
};

/**
 * @brief Channel Strip in the rack (mixer routing, effects, etc.)
 */
class ChannelStripComponent : public juce::Component {
public:
    ChannelStripComponent(Channel& channel);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    Channel& channel_;
    
    std::unique_ptr<juce::Slider> volumeSlider_;
    std::unique_ptr<juce::Slider> panSlider_;
    std::unique_ptr<juce::TextButton> muteButton_;
    std::unique_ptr<juce::TextButton> soloButton_;
    std::unique_ptr<juce::ComboBox> mixerTrackSelector_;
    std::unique_ptr<juce::Label> nameLabel_;
};

} // namespace Sequencer
} // namespace OmegaStudio
