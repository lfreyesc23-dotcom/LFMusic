/*
  ==============================================================================
    PianoRollEditor.h
    
    Piano Roll Visual Editor completo estilo FL Studio
    - Note grid con colores y transparencia
    - Velocity lanes con barras visuales
    - Scale highlighting (13 escalas)
    - Ghost notes de otras pistas
    - Tools: Pencil, Brush, Delete, Select, Slice
    - Context menus con shortcuts
    - Snap to grid configurable
    - Zoom horizontal/vertical
    - Chord stamps y arpeggiator visual
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Sequencer/PianoRoll.h"
#include "../Sequencer/PianoRollAdvanced.h"
#include <memory>
#include <vector>
#include <map>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/** Tool modes para Piano Roll */
enum class PianoRollTool {
    Select,
    Pencil,
    Brush,
    Delete,
    Slice,
    Mute,
    Paint
};

//==============================================================================
/** Piano Key component (teclado lateral) */
class PianoKeyComponent : public juce::Component {
public:
    PianoKeyComponent(int noteNumber);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    bool isBlackKey() const;
    void setHighlighted(bool shouldHighlight);
    void setInScale(bool inScale);
    
    std::function<void(int note, bool down)> onNoteEvent;
    
private:
    int noteNumber_;
    bool highlighted_ = false;
    bool inScale_ = true;
    bool pressed_ = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyComponent)
};

//==============================================================================
/** Note visual en el grid */
class NoteComponent : public juce::Component {
public:
    NoteComponent(const Sequencer::MIDINote& note, int noteIndex);
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    void setSelected(bool selected);
    bool isSelected() const { return selected_; }
    
    void setGhostNote(bool isGhost);
    bool isGhostNote() const { return ghostNote_; }
    
    int getNoteIndex() const { return noteIndex_; }
    const Sequencer::MIDINote& getNote() const { return note_; }
    
    std::function<void(int index, juce::Point<int> delta)> onNoteMoved;
    std::function<void(int index, double newLength)> onNoteResized;
    std::function<void(int index)> onNoteSelected;
    
private:
    Sequencer::MIDINote note_;
    int noteIndex_;
    bool selected_ = false;
    bool ghostNote_ = false;
    bool resizing_ = false;
    juce::Point<int> dragStartPos_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteComponent)
};

//==============================================================================
/** Velocity lane editor */
class VelocityLaneComponent : public juce::Component {
public:
    VelocityLaneComponent();
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    
    void setNotes(const std::vector<Sequencer::MIDINote>& notes);
    void setPixelsPerBeat(float ppb) { pixelsPerBeat_ = ppb; repaint(); }
    void setSelectedNotes(const std::vector<int>& indices);
    
    std::function<void(int index, float velocity)> onVelocityChanged;
    
private:
    std::vector<Sequencer::MIDINote> notes_;
    std::vector<int> selectedNotes_;
    float pixelsPerBeat_ = 100.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityLaneComponent)
};

//==============================================================================
/** Piano Roll Editor completo */
class PianoRollEditor : public juce::Component,
                       public juce::Timer,
                       public juce::ScrollBar::Listener {
public:
    PianoRollEditor();
    ~PianoRollEditor() override;
    
    // Clip management
    void setClip(std::shared_ptr<Sequencer::MIDIClip> clip);
    std::shared_ptr<Sequencer::MIDIClip> getClip() const { return clip_; }
    
    // Scale highlighting
    void setScale(int rootNote, Sequencer::Scale scale);
    void setScaleVisible(bool visible);
    bool isScaleVisible() const { return scaleVisible_; }
    
    // Ghost notes
    void addGhostTrack(std::shared_ptr<Sequencer::MIDIClip> ghostClip, 
                      const juce::String& name, 
                      juce::Colour colour);
    void clearGhostTracks();
    
    // Tools
    void setTool(PianoRollTool tool);
    PianoRollTool getCurrentTool() const { return currentTool_; }
    
    // Grid & snap
    void setGridSize(double beats);
    double getGridSize() const { return gridSize_; }
    void setSnapEnabled(bool enabled);
    bool isSnapEnabled() const { return snapEnabled_; }
    
    // Zoom
    void setPixelsPerBeat(float ppb);
    float getPixelsPerBeat() const { return pixelsPerBeat_; }
    void setNoteHeight(float height);
    float getNoteHeight() const { return noteHeight_; }
    
    // Playhead
    void setPlayheadPosition(double beats);
    double getPlayheadPosition() const { return playheadPosition_; }
    
    // Selection
    std::vector<int> getSelectedNotes() const;
    void selectAll();
    void selectNone();
    void deleteSelected();
    
    // Editing operations
    void quantizeSelected(double gridSize);
    void transposeSelected(int semitones);
    void setVelocityForSelected(float velocity);
    void humanizeSelected(float timing, float velocity);
    
    // Chord & arp tools
    void chordizeSelected(Sequencer::ChordGenerator::ChordType type);
    void arpeggiateSelected(Sequencer::Arpeggiator::Pattern pattern, double noteLength);
    void strumSelected(double strumTime);
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    // Timer for playhead
    void timerCallback() override;
    
    // ScrollBar listener
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override;
    
private:
    // Data
    std::shared_ptr<Sequencer::MIDIClip> clip_;
    std::vector<std::unique_ptr<NoteComponent>> noteComponents_;
    
    // Ghost tracks
    struct GhostTrack {
        std::shared_ptr<Sequencer::MIDIClip> clip;
        juce::String name;
        juce::Colour colour;
    };
    std::vector<GhostTrack> ghostTracks_;
    
    // Scale
    int scaleRootNote_ = 60; // C4
    Sequencer::Scale currentScale_ = Sequencer::Scale::Major;
    bool scaleVisible_ = false;
    std::vector<bool> notesInScale_;
    
    // UI state
    PianoRollTool currentTool_ = PianoRollTool::Select;
    float pixelsPerBeat_ = 100.0f;
    float noteHeight_ = 12.0f;
    double gridSize_ = 0.25; // 16th notes
    bool snapEnabled_ = true;
    double playheadPosition_ = 0.0;
    
    // Piano keys
    static constexpr int NUM_KEYS = 128;
    static constexpr int PIANO_KEY_WIDTH = 60;
    std::array<std::unique_ptr<PianoKeyComponent>, NUM_KEYS> pianoKeys_;
    
    // Velocity lane
    std::unique_ptr<VelocityLaneComponent> velocityLane_;
    static constexpr int VELOCITY_LANE_HEIGHT = 100;
    
    // Scrollbars
    std::unique_ptr<juce::ScrollBar> horizontalScrollBar_;
    std::unique_ptr<juce::ScrollBar> verticalScrollBar_;
    
    // Selection
    std::vector<int> selectedNoteIndices_;
    juce::Rectangle<int> lassoRectangle_;
    bool isLassoing_ = false;
    
    // Context menu
    std::unique_ptr<juce::PopupMenu> contextMenu_;
    
    // Helper methods
    void rebuildNoteComponents();
    void updateScaleHighlighting();
    void updatePianoKeys();
    juce::Rectangle<int> getNoteRectangle(const Sequencer::MIDINote& note) const;
    int getNoteNumberAtY(int y) const;
    double getBeatsAtX(int x) const;
    int getXForBeats(double beats) const;
    int getYForNote(int noteNumber) const;
    double snapToGrid(double beats) const;
    void createNote(int noteNumber, double startBeat);
    void showContextMenu();
    void paintGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void paintNotes(juce::Graphics& g);
    void paintPlayhead(juce::Graphics& g);
    void paintScaleHighlighting(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};

//==============================================================================
/** Piano Roll Window - ventana flotante */
class PianoRollWindow : public juce::DocumentWindow {
public:
    PianoRollWindow(const juce::String& name);
    ~PianoRollWindow() override;
    
    void closeButtonPressed() override;
    
    PianoRollEditor* getEditor() { return editor_; }
    
private:
    PianoRollEditor* editor_; // Owned by DocumentWindow
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWindow)
};

} // namespace GUI
} // namespace OmegaStudio
