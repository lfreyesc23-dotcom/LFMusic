//==============================================================================
// FunctionalPianoRoll.h
// Editor MIDI funcional estilo FL Studio
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Nota MIDI arrastrable
//==============================================================================
class MIDINote : public juce::Component {
public:
    MIDINote(int n, double start, double dur, float vel = 0.8f)
        : note(n), startTime(start), duration(dur), velocity(vel) {
        
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    
    void paint(juce::Graphics& g) override {
        // Note color based on velocity
        auto color = juce::Colour::fromHSV(0.55f, velocity, 0.9f, 1.0f);
        
        g.setColour(color);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 2.0f);
        
        g.setColour(color.brighter(0.4f));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2.0f, 1.5f);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        dragger.startDraggingComponent(this, e);
        originalBounds = getBounds();
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isShiftDown()) {
            // Resize (change duration)
            auto newWidth = juce::jmax(10, originalBounds.getWidth() + e.getDistanceFromDragStartX());
            setSize(newWidth, getHeight());
            duration = newWidth / pixelsPerBeat;
        } else {
            // Move
            dragger.dragComponent(this, e, nullptr);
        }
    }
    
    int getNote() const { return note; }
    double getStartTime() const { return startTime; }
    double getDuration() const { return duration; }
    float getVelocity() const { return velocity; }
    
    void setNote(int n) { note = n; repaint(); }
    void setStartTime(double t) { startTime = t; }
    void setDuration(double d) { duration = d; }
    void setVelocity(float v) { velocity = juce::jlimit(0.0f, 1.0f, v); repaint(); }
    void setPixelsPerBeat(double ppb) { pixelsPerBeat = ppb; }
    
private:
    int note;
    double startTime;
    double duration;
    float velocity;
    juce::ComponentDragger dragger;
    juce::Rectangle<int> originalBounds;
    double pixelsPerBeat = 100.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDINote)
};

//==============================================================================
// Grid del Piano Roll
//==============================================================================
class PianoRollGrid : public juce::Component {
public:
    std::function<void(int, double, double)> onNoteAdded;
    
    PianoRollGrid() {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
    
    void paint(juce::Graphics& g) override {
        // Background
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Piano keys on left
        int keyHeight = getHeight() / numKeys;
        for (int i = 0; i < numKeys; ++i) {
            int note = (numKeys - 1 - i) + lowestNote;
            bool isBlackKey = isBlackNote(note % 12);
            
            g.setColour(isBlackKey ? juce::Colour(0xff2a2a2a) : juce::Colour(0xff3a3a3a));
            g.fillRect(0, i * keyHeight, 60, keyHeight - 1);
            
            // Highlight C notes
            if (note % 12 == 0) {
                g.setColour(juce::Colour(0xff4a4a4a));
                g.fillRect(0, i * keyHeight, 60, keyHeight - 1);
                
                // Draw note name
                g.setColour(juce::Colours::white);
                g.setFont(10.0f);
                g.drawText("C" + juce::String(note / 12 - 1), 
                          5, i * keyHeight, 50, keyHeight, 
                          juce::Justification::centredLeft, false);
            }
            
            // Border
            g.setColour(juce::Colour(0xff0a0a0a));
            g.drawLine(0, (float)(i * keyHeight), 60.0f, (float)(i * keyHeight));
        }
        
        // Vertical grid lines (beats)
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int x = 60; x < getWidth(); x += (int)pixelsPerBeat) {
            g.drawVerticalLine(x, 0.0f, (float)getHeight());
        }
        
        // Horizontal grid lines (notes)
        g.setColour(juce::Colour(0xff252525));
        for (int i = 0; i < numKeys; ++i) {
            g.drawHorizontalLine(i * keyHeight, 60.0f, (float)getWidth());
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.x < 60) return; // Clicked on piano keys
        
        if (e.mods.isRightButtonDown()) {
            // Delete note at position
            for (int i = notes.size() - 1; i >= 0; --i) {
                if (notes[i]->getBounds().contains(e.getPosition())) {
                    notes.remove(i);
                    break;
                }
            }
        } else {
            // Add new note
            int note = pixelToNote(e.y);
            double startTime = (e.x - 60) / pixelsPerBeat;
            double duration = snapValue / 4.0; // Quarter note by default
            
            addNote(note, startTime, duration, 0.8f);
            
            if (onNoteAdded) {
                onNoteAdded(note, startTime, duration);
            }
        }
    }
    
    void addNote(int note, double startTime, double duration, float velocity) {
        auto* midiNote = new MIDINote(note, startTime, duration, velocity);
        midiNote->setPixelsPerBeat(pixelsPerBeat);
        notes.add(midiNote);
        addAndMakeVisible(midiNote);
        layoutNote(midiNote);
    }
    
    void clearNotes() {
        notes.clear();
    }
    
    int getNumNotes() const { return notes.size(); }
    MIDINote* getNote(int index) { return notes[index]; }
    
    void setPixelsPerBeat(double ppb) {
        pixelsPerBeat = ppb;
        for (auto* note : notes) {
            note->setPixelsPerBeat(ppb);
            layoutNote(note);
        }
        repaint();
    }
    
    void setSnapValue(double snap) {
        snapValue = snap;
    }
    
private:
    void layoutNote(MIDINote* note) {
        int y = noteToPixel(note->getNote());
        int x = 60 + (int)(note->getStartTime() * pixelsPerBeat);
        int width = (int)(note->getDuration() * pixelsPerBeat);
        int height = getHeight() / numKeys - 1;
        
        note->setBounds(x, y, width, height);
    }
    
    int pixelToNote(int y) const {
        int keyHeight = getHeight() / numKeys;
        int keyIndex = y / keyHeight;
        return (numKeys - 1 - keyIndex) + lowestNote;
    }
    
    int noteToPixel(int note) const {
        int keyHeight = getHeight() / numKeys;
        int keyIndex = (numKeys - 1) - (note - lowestNote);
        return keyIndex * keyHeight;
    }
    
    bool isBlackNote(int noteInOctave) const {
        return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || 
               noteInOctave == 8 || noteInOctave == 10;
    }
    
    juce::OwnedArray<MIDINote> notes;
    int numKeys = 88; // Full piano range
    int lowestNote = 21; // A0
    double pixelsPerBeat = 100.0;
    double snapValue = 1.0; // 1 beat
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollGrid)
};

//==============================================================================
// Piano Roll completo con toolbar
//==============================================================================
class FunctionalPianoRoll : public juce::Component {
public:
    FunctionalPianoRoll() {
        // Toolbar buttons
        pencilButton.setButtonText("✏️ Pencil");
        pencilButton.setClickingTogglesState(true);
        pencilButton.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(pencilButton);
        
        eraserButton.setButtonText("🗑️ Eraser");
        eraserButton.setClickingTogglesState(true);
        addAndMakeVisible(eraserButton);
        
        selectButton.setButtonText("👆 Select");
        selectButton.setClickingTogglesState(true);
        addAndMakeVisible(selectButton);
        
        // Snap selector
        snapSelector.addItem("1/1", 1);
        snapSelector.addItem("1/2", 2);
        snapSelector.addItem("1/4", 3);
        snapSelector.addItem("1/8", 4);
        snapSelector.addItem("1/16", 5);
        snapSelector.addItem("1/32", 6);
        snapSelector.setSelectedId(3, juce::dontSendNotification);
        snapSelector.onChange = [this] {
            double snap = 1.0;
            switch (snapSelector.getSelectedId()) {
                case 1: snap = 4.0; break;
                case 2: snap = 2.0; break;
                case 3: snap = 1.0; break;
                case 4: snap = 0.5; break;
                case 5: snap = 0.25; break;
                case 6: snap = 0.125; break;
            }
            grid.setSnapValue(snap);
        };
        addAndMakeVisible(snapSelector);
        
        snapLabel.setText("Snap:", juce::dontSendNotification);
        addAndMakeVisible(snapLabel);
        
        // Zoom slider
        zoomSlider.setRange(50.0, 300.0, 1.0);
        zoomSlider.setValue(100.0);
        zoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        zoomSlider.onValueChange = [this] {
            grid.setPixelsPerBeat(zoomSlider.getValue());
        };
        addAndMakeVisible(zoomSlider);
        
        zoomLabel.setText("Zoom:", juce::dontSendNotification);
        addAndMakeVisible(zoomLabel);
        
        // Clear button
        clearButton.setButtonText("Clear All");
        clearButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffF44336));
        clearButton.onClick = [this] {
            grid.clearNotes();
        };
        addAndMakeVisible(clearButton);
        
        // Viewport for scrolling
        viewport.setViewedComponent(&grid, false);
        viewport.setScrollBarsShown(true, true);
        addAndMakeVisible(viewport);
        
        // Set grid size
        grid.setBounds(0, 0, 4000, 2200); // 40 beats x 88 keys
        
        // Add some demo notes
        grid.addNote(60, 0.0, 1.0, 0.8f); // C4
        grid.addNote(64, 1.0, 1.0, 0.7f); // E4
        grid.addNote(67, 2.0, 1.0, 0.75f); // G4
        grid.addNote(72, 3.0, 1.0, 0.9f); // C5
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        // Toolbar
        auto toolbar = area.removeFromTop(40);
        pencilButton.setBounds(toolbar.removeFromLeft(90).reduced(5));
        eraserButton.setBounds(toolbar.removeFromLeft(90).reduced(5));
        selectButton.setBounds(toolbar.removeFromLeft(90).reduced(5));
        toolbar.removeFromLeft(10);
        snapLabel.setBounds(toolbar.removeFromLeft(50).reduced(5));
        snapSelector.setBounds(toolbar.removeFromLeft(80).reduced(5));
        toolbar.removeFromLeft(10);
        zoomLabel.setBounds(toolbar.removeFromLeft(50).reduced(5));
        zoomSlider.setBounds(toolbar.removeFromLeft(150).reduced(5));
        toolbar.removeFromLeft(10);
        clearButton.setBounds(toolbar.removeFromLeft(90).reduced(5));
        
        // Grid viewport
        viewport.setBounds(area);
    }
    
private:
    juce::TextButton pencilButton;
    juce::TextButton eraserButton;
    juce::TextButton selectButton;
    juce::ComboBox snapSelector;
    juce::Label snapLabel;
    juce::Slider zoomSlider;
    juce::Label zoomLabel;
    juce::TextButton clearButton;
    
    juce::Viewport viewport;
    PianoRollGrid grid;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionalPianoRoll)
};

} // namespace GUI
} // namespace OmegaStudio
