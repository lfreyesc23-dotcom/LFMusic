#pragma once
#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

/**
 * @brief FL Studio 2025 Professional Look and Feel
 */
class FLStudio2025LookAndFeel : public juce::LookAndFeel_V4
{
public:
    FLStudio2025LookAndFeel();
    ~FLStudio2025LookAndFeel() override;
    
    // Color scheme
    struct ColorScheme {
        juce::Colour background = juce::Colour(0xff1a1a1a);
        juce::Colour surface = juce::Colour(0xff2a2a2a);
        juce::Colour surfaceHighlight = juce::Colour(0xff3a3a3a);
        juce::Colour primary = juce::Colour(0xffff8800);
        juce::Colour secondary = juce::Colour(0xff00aaff);
        juce::Colour text = juce::Colour(0xffe0e0e0);
        juce::Colour textDim = juce::Colour(0xff808080);
        juce::Colour border = juce::Colour(0xff404040);
        juce::Colour success = juce::Colour(0xff00cc00);
        juce::Colour warning = juce::Colour(0xffffaa00);
        juce::Colour error = juce::Colour(0xffff0000);
        juce::Colour mixer = juce::Colour(0xff008080);
        juce::Colour piano = juce::Colour(0xff800080);
        juce::Colour playlist = juce::Colour(0xff0080ff);
    };
    
    void setColorScheme(const ColorScheme& scheme);
    ColorScheme getColorScheme() const { return colors; }
    
    // Drawing overrides
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
    
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider&) override;
    
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle, juce::Slider&) override;
    
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    
    void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;
    
    void drawTextEditorOutline(juce::Graphics&, int width, int height, juce::TextEditor&) override;

private:
    ColorScheme colors;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudio2025LookAndFeel)
};

/**
 * @brief Dockable Panel System
 */
class DockablePanel : public juce::Component
{
public:
    enum class DockPosition {
        Left,
        Right,
        Top,
        Bottom,
        Center,
        Floating
    };
    
    DockablePanel(const juce::String& name, DockPosition defaultPosition = DockPosition::Center);
    ~DockablePanel() override;
    
    void setTitle(const juce::String& title) { panelTitle = title; repaint(); }
    juce::String getTitle() const { return panelTitle; }
    
    void setDockPosition(DockPosition position);
    DockPosition getDockPosition() const { return dockPosition; }
    
    void setCanClose(bool canClose) { this->canClose = canClose; }
    void setCanFloat(bool canFloat) { this->canFloat = canFloat; }
    void setCanResize(bool canResize) { this->canResize = canResize; }
    
    void setContentComponent(std::unique_ptr<juce::Component> component);
    juce::Component* getContentComponent() { return contentComponent.get(); }
    
    std::function<void()> onCloseClicked;
    std::function<void()> onFloatClicked;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    juce::String panelTitle;
    DockPosition dockPosition;
    bool canClose = true;
    bool canFloat = true;
    bool canResize = true;
    
    std::unique_ptr<juce::Component> contentComponent;
    juce::Rectangle<int> titleBarBounds;
    juce::Rectangle<int> closeButtonBounds;
    juce::Rectangle<int> floatButtonBounds;
    
    bool isDragging = false;
    juce::Point<int> dragOffset;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockablePanel)
};

/**
 * @brief Workspace - Docking Container
 */
class Workspace : public juce::Component
{
public:
    Workspace();
    ~Workspace() override;
    
    void addPanel(std::unique_ptr<DockablePanel> panel);
    void removePanel(DockablePanel* panel);
    void clearPanels();
    
    std::vector<DockablePanel*> getPanels() const;
    DockablePanel* getPanelByName(const juce::String& name);
    
    // Layout presets
    void saveLayout(const juce::String& name);
    void loadLayout(const juce::String& name);
    std::vector<juce::String> getSavedLayouts() const;
    
    // Default layouts
    void setMixerLayout();
    void setPianoRollLayout();
    void setPlaylistLayout();
    void setMasteringLayout();
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<DockablePanel>> panels;
    std::map<juce::String, juce::ValueTree> savedLayouts;
    
    void arrangePanels();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Workspace)
};

/**
 * @brief Transport Bar - Play, Stop, Record, Tempo
 */
class TransportBar : public juce::Component
{
public:
    TransportBar();
    ~TransportBar() override;
    
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onRecordClicked;
    std::function<void(double)> onBPMChanged;
    std::function<void()> onLoopClicked;
    std::function<void()> onMetronomeClicked;
    
    void setPlaying(bool playing);
    void setRecording(bool recording);
    void setLoopEnabled(bool enabled);
    void setMetronomeEnabled(bool enabled);
    void setBPM(double bpm);
    void setPosition(double bars, double beats);
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton recordButton;
    juce::TextButton loopButton;
    juce::TextButton metronomeButton;
    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    juce::Label positionLabel;
    
    bool isPlaying = false;
    bool isRecording = false;
    bool loopEnabled = false;
    bool metronomeEnabled = false;
    double currentBPM = 120.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

/**
 * @brief Top Menu Bar
 */
class TopMenuBar : public juce::Component
{
public:
    TopMenuBar();
    ~TopMenuBar() override;
    
    void setMenuBarModel(juce::MenuBarModel* model);
    
    std::function<void(const juce::String&)> onViewChanged;
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    juce::ComboBox viewSelector;
    juce::TextButton settingsButton;
    juce::TextButton helpButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopMenuBar)
};

/**
 * @brief Channel Rack Component (FL Studio)
 */
class ChannelRackComponent : public juce::Component
{
public:
    struct Channel {
        juce::String name;
        bool muted = false;
        bool solo = false;
        float volume = 0.8f;
        float pan = 0.0f;
        int mixerTrack = 0;
        juce::Colour colour;
        juce::String instrumentName;
    };
    
    ChannelRackComponent();
    ~ChannelRackComponent() override;
    
    void addChannel(const Channel& channel);
    void removeChannel(int index);
    void clearChannels();
    
    std::vector<Channel>& getChannels() { return channels; }
    
    std::function<void(int channelIndex)> onChannelSelected;
    std::function<void(int channelIndex, bool state)> onMuteToggled;
    std::function<void(int channelIndex, bool state)> onSoloToggled;
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<Channel> channels;
    int selectedChannel = -1;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackComponent)
};

/**
 * @brief Browser Panel (Plugins, Samples, Presets)
 */
class BrowserPanel : public juce::Component,
                     public juce::TextEditor::Listener
{
public:
    enum class BrowserMode {
        Plugins,
        Samples,
        Presets,
        Projects
    };
    
    BrowserPanel();
    ~BrowserPanel() override;
    
    void setBrowserMode(BrowserMode mode);
    BrowserMode getBrowserMode() const { return currentMode; }
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void textEditorTextChanged(juce::TextEditor&) override;

private:
    BrowserMode currentMode = BrowserMode::Plugins;
    
    juce::TabbedComponent tabs;
    juce::TextEditor searchBox;
    juce::TreeView browserTree;
    juce::Label infoLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserPanel)
};

/**
 * @brief Main Application Window - FL Studio 2025 Style
 */
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name);
    ~MainWindow() override;
    
    void closeButtonPressed() override;
    
    Workspace* getWorkspace() { return workspace.get(); }
    TransportBar* getTransportBar() { return transportBar.get(); }
    TopMenuBar* getTopMenuBar() { return topMenuBar.get(); }
    
    void setFullScreen(bool fullScreen);
    void setAlwaysOnTop(bool onTop);
    
    void saveWindowState();
    void loadWindowState();

private:
    FLStudio2025LookAndFeel lookAndFeel;
    
    std::unique_ptr<TopMenuBar> topMenuBar;
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<Workspace> workspace;
    
    void setupDefaultLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace GUI
} // namespace OmegaStudio
