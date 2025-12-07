//==============================================================================
// FLStudio2025Interface.h
// INTERFAZ EXACTA DE FL STUDIO 2025 - NIVEL INDUSTRIAL
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "FLStudioLookAndFeel.h"

namespace Omega {
namespace Audio { class AudioEngine; }
namespace GUI {

//==============================================================================
// FL STUDIO 2025 - TOP TOOLBAR (Barra superior completa)
//==============================================================================
class FLStudio2025Toolbar : public juce::Component, private juce::Timer {
public:
    FLStudio2025Toolbar();
    ~FLStudio2025Toolbar() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setPlaying(bool isPlaying);
    void setRecording(bool isRecording);
    bool isAnyTransportButton(juce::Component* c) const { return c == playButton_.get() || c == stopButton_.get() || c == recordButton_.get(); }
    
    // Transport controls callbacks
    std::function<void(bool)> onPlay;
    std::function<void()> onStop;
    std::function<void(bool)> onRecord;
    std::function<void()> onPatternMode;
    std::function<void()> onSongMode;
    
    // AI callbacks
    std::function<void()> onAISeparate;
    std::function<void()> onAILoop;
    std::function<void()> onAIChord;
    std::function<void()> onAIGopher;
    
private:
    // Logo
    std::unique_ptr<juce::Label> logoLabel_;
    
    // Menu bar
    std::unique_ptr<juce::TextButton> fileButton_;
    std::unique_ptr<juce::TextButton> optionsButton_;
    std::unique_ptr<juce::TextButton> toolsButton_;
    std::unique_ptr<juce::TextButton> helpButton_;
    
    // Transport controls
    std::unique_ptr<juce::TextButton> playButton_;
    std::unique_ptr<juce::TextButton> stopButton_;
    std::unique_ptr<juce::TextButton> recordButton_;
    std::unique_ptr<juce::TextButton> patternButton_;
    std::unique_ptr<juce::TextButton> songButton_;
    
    // Tempo display
    std::unique_ptr<juce::Label> tempoLabel_;
    // Time display
    std::unique_ptr<juce::Label> timeLabel_;
    
    // CPU/Memory meters
    std::unique_ptr<juce::Label> cpuLabel_;
    std::unique_ptr<juce::Label> memoryLabel_;
    float cpuUsage_ = 13.0f;
    int memoryUsage_ = 3858;
    
    // Tool buttons (exactos a FL Studio)
    std::unique_ptr<juce::TextButton> cutButton_;
    std::unique_ptr<juce::TextButton> copyButton_;
    std::unique_ptr<juce::TextButton> pasteButton_;
    std::unique_ptr<juce::TextButton> deleteButton_;
    std::unique_ptr<juce::TextButton> undoButton_;
    std::unique_ptr<juce::TextButton> redoButton_;
    
    // Mode indicators
    std::unique_ptr<juce::TextButton> metronomeButton_;
    std::unique_ptr<juce::TextButton> blendButton_;
    std::unique_ptr<juce::TextButton> typingKeyboardButton_;
    
    // AI BUTTONS (NEW!)
    std::unique_ptr<juce::TextButton> aiSeparateButton_;
    std::unique_ptr<juce::TextButton> aiLoopButton_;
    std::unique_ptr<juce::TextButton> aiChordButton_;
    std::unique_ptr<juce::TextButton> aiGopherButton_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025Toolbar)
};

//==============================================================================
// FL STUDIO 2025 - PATTERN PANEL (Panel lateral izquierdo de patterns)
//==============================================================================
class FLStudio2025PatternPanel : public juce::Component {
public:
    FLStudio2025PatternPanel();
    ~FLStudio2025PatternPanel() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    std::function<void(int)> onPatternSelected;
    std::function<void(int, int)> onPatternMoved;
    
    void addPattern(const juce::String& name, juce::Colour colour);
    void selectPattern(int index);
    
private:
    struct Pattern {
        juce::String name;
        juce::Colour colour;
        bool isSelected = false;
    };
    
    juce::Array<Pattern> patterns_;
    int selectedPatternIndex_ = 0;
    int hoveredPatternIndex_ = -1;
    int patternHeight_ = 24;
    
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<juce::Component> contentComponent_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025PatternPanel)
};

//==============================================================================
// FL STUDIO 2025 - PLAYLIST VIEW (Vista central de arranque/playlist)
//==============================================================================
class FLStudio2025PlaylistView : public juce::Component {
public:
    FLStudio2025PlaylistView();
    ~FLStudio2025PlaylistView() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    struct Clip {
        juce::String name;
        juce::Colour colour;
        int trackIndex;
        double startBeat;
        double lengthBeats;
        juce::Rectangle<int> bounds;
    };
    
    void addClip(const Clip& clip);
    void clearClips();
    
    std::function<void(const Clip&)> onClipSelected;
    std::function<void(const Clip&, double, int)> onClipMoved;
    
private:
    juce::Array<Clip> clips_;
    int selectedClipIndex_ = -1;
    
    // Grid parameters - más espacioso como FL Studio
    int tracksCount_ = 30;
    int trackHeight_ = 48;  // Aumentado de 40 a 48 para mejor espaciado
    double beatsPerBar_ = 4.0;
    double pixelsPerBeat_ = 20.0;
    double horizontalOffset_ = 0.0;
    double verticalOffset_ = 0.0;
    
    // Ruler
    void drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawTrackHeaders(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawClips(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025PlaylistView)
};

//==============================================================================
// FL STUDIO 2025 - GOPHER AI CHAT (Panel de IA derecho)
//==============================================================================
class FLStudio2025GopherAIChat : public juce::Component {
public:
    FLStudio2025GopherAIChat();
    ~FLStudio2025GopherAIChat() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void addMessage(const juce::String& sender, const juce::String& message, juce::Colour senderColor);
    void processCommand(const juce::String& command);
    
    // Callbacks for AI commands
    std::function<void()> onSeparateStems;
    std::function<void(const juce::String&)> onGenerateLoop;
    std::function<void()> onGenerateChord;
    std::function<void(const juce::String&)> onAskGopher;
    
private:
    std::unique_ptr<juce::Label> titleLabel_;
    std::unique_ptr<juce::TextEditor> chatDisplay_;
    std::unique_ptr<juce::TextEditor> inputField_;
    std::unique_ptr<juce::TextButton> sendButton_;
    
    void sendMessage();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025GopherAIChat)
};

//==============================================================================
// FL STUDIO 2025 - CHANNEL RACK (Panel inferior con step sequencer)
//==============================================================================
class FLStudio2025ChannelRack : public juce::Component {
public:
    FLStudio2025ChannelRack();
    ~FLStudio2025ChannelRack() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    
    struct Channel {
        juce::String name;
        juce::Colour colour;
        float volume = 0.8f;
        float pan = 0.0f;
        bool mute = false;
        bool solo = false;
        std::vector<bool> steps;  // 32 steps
        
        Channel() : colour(juce::Colours::white), steps(32, false) {}  // Default constructor
        Channel(const juce::String& n, juce::Colour c) 
            : name(n), colour(c), steps(32, false) {}
    };
    
    void addChannel(const juce::String& name, juce::Colour colour);
    void toggleStep(int channelIndex, int stepIndex);
    
    std::function<void(int, int)> onStepToggled;
    
private:
    juce::Array<Channel> channels_;
    int visibleSteps_ = 32;
    
    // Tabs
    std::unique_ptr<juce::TabbedComponent> tabComponent_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025ChannelRack)
};

//==============================================================================
// FL STUDIO 2025 - MAIN WINDOW (Ventana principal completa)
//==============================================================================
class FLStudio2025MainWindow : public juce::Component {
public:
    explicit FLStudio2025MainWindow(Audio::AudioEngine* audioEngine);
    ~FLStudio2025MainWindow() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    Audio::AudioEngine* audioEngine_ = nullptr;  // Not owned

    // Layout
    std::unique_ptr<FLStudio2025Toolbar> toolbar_;
    std::unique_ptr<FLStudio2025PatternPanel> patternPanel_;
    std::unique_ptr<FLStudio2025PlaylistView> playlistView_;
    std::unique_ptr<FLStudio2025GopherAIChat> gopherChat_;  // AI ASSISTANT!
    std::unique_ptr<FLStudio2025ChannelRack> channelRack_;
    
    // Resizer bars
    std::unique_ptr<juce::ResizableEdgeComponent> leftResizer_;
    std::unique_ptr<juce::ResizableEdgeComponent> rightResizer_;
    std::unique_ptr<juce::ResizableEdgeComponent> channelRackResizer_;  // NEW!
    
    // Dimensions
    int toolbarHeight_ = 60;
    int patternPanelWidth_ = 200;
    int helpPanelWidth_ = 350;
    int channelRackHeight_ = 150;  // Más compacto como imagen

    bool isRecording_ = false;
    
    void initializeDefaultPatterns();
    void initializeDefaultClips();
    void setupHelpContent();
    void initializeChannelRack();  // NEW!
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025MainWindow)
};

} // namespace GUI
} // namespace Omega
