/*
  ==============================================================================
    PlaylistEditor.h
    Playlist/Arrangement View estilo FL Studio
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Sequencer/PlaylistEngine.h"
#include "../Sequencer/Timeline/Timeline.h"
#include <memory>

namespace OmegaStudio {
namespace GUI {

class WaveformCache; // Forward declaration

//==============================================================================
/** Componente de clip de audio/MIDI */
class ClipComponent : public juce::Component {
public:
    enum class ClipType { Audio, MIDI, Pattern };
    
    ClipComponent(ClipType type, int trackIndex, double startTime, double duration);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    void setColour(juce::Colour colour) { colour_ = colour; repaint(); }
    void setSelected(bool selected) { selected_ = selected; repaint(); }
    void setName(const juce::String& name) { name_ = name; }
    
    ClipType getType() const { return type_; }
    int getTrackIndex() const { return trackIndex_; }
    double getStartTime() const { return startTime_; }
    double getDuration() const { return duration_; }
    
    std::function<void(int clipId, juce::Point<int> delta)> onMoved;
    std::function<void(int clipId, double newDuration)> onResized;
    
private:
    ClipType type_;
    int trackIndex_;
    double startTime_;
    double duration_;
    juce::String name_;
    juce::Colour colour_;
    bool selected_ = false;
    bool resizing_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};

//==============================================================================
/** Pista de playlist */
class PlaylistTrackComponent : public juce::Component {
public:
    PlaylistTrackComponent(int trackIndex);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setTrackName(const juce::String& name);
    void setTrackColour(juce::Colour colour);
    void setHeight(int height);
    
    void addClip(std::unique_ptr<ClipComponent> clip);
    void clearClips();
    
private:
    int trackIndex_;
    juce::String trackName_;
    juce::Colour trackColour_;
    std::vector<std::unique_ptr<ClipComponent>> clips_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistTrackComponent)
};

//==============================================================================
/** Editor de Playlist completo */
class PlaylistEditor : public juce::Component,
                      public juce::Timer {
public:
    PlaylistEditor();
    ~PlaylistEditor() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    // Tracks
    void setNumTracks(int numTracks);
    int getNumTracks() const { return (int)tracks_.size(); }
    
    // Playhead
    void setPlayheadPosition(double beats);
    
    // Grid & zoom
    void setPixelsPerBeat(float ppb);
    void setGridSize(double beats);
    
private:
    std::vector<std::unique_ptr<PlaylistTrackComponent>> tracks_;
    std::unique_ptr<juce::Viewport> viewport_;
    std::unique_ptr<juce::Component> contentComponent_;
    std::unique_ptr<juce::ScrollBar> horizontalScrollBar_;
    
    float pixelsPerBeat_ = 100.0f;
    double gridSize_ = 1.0; // 1 bar
    double playheadPosition_ = 0.0;
    
    static constexpr int TRACK_HEIGHT = 80;
    static constexpr int TRACK_HEADER_WIDTH = 150;
    
    void paintGrid(juce::Graphics& g);
    void paintPlayhead(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistEditor)
};

//==============================================================================
class PlaylistWindow : public juce::DocumentWindow {
public:
    PlaylistWindow(const juce::String& name);
    ~PlaylistWindow() override;
    void closeButtonPressed() override;
    PlaylistEditor* getEditor() { return editor_; }
    
private:
    PlaylistEditor* editor_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistWindow)
};

} // namespace GUI
} // namespace OmegaStudio
