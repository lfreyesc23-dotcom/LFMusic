//==============================================================================
// AdvancedPianoRollFeatures.h
// FL Studio 2025 Advanced Piano Roll
// Scale highlighting, chord stamps, note properties, velocity layers
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Musical Scale - Definición de escalas musicales
//==============================================================================
struct MusicalScale {
    enum class Type {
        Major, Minor, HarmonicMinor, MelodicMinor,
        Dorian, Phrygian, Lydian, Mixolydian,
        Pentatonic, Blues, Chromatic
    };
    
    Type type = Type::Major;
    int rootNote = 0; // 0-11 (C-B)
    
    std::vector<bool> getScaleNotes() const {
        std::vector<bool> notes(12, false);
        
        switch (type) {
            case Type::Major:
                notes = {true, false, true, false, true, true, false, true, false, true, false, true};
                break;
            case Type::Minor:
                notes = {true, false, true, true, false, true, false, true, true, false, true, false};
                break;
            case Type::HarmonicMinor:
                notes = {true, false, true, true, false, true, false, true, true, false, false, true};
                break;
            case Type::Pentatonic:
                notes = {true, false, true, false, true, false, false, true, false, true, false, false};
                break;
            case Type::Blues:
                notes = {true, false, false, true, false, true, true, true, false, false, true, false};
                break;
            case Type::Chromatic:
                notes = {true, true, true, true, true, true, true, true, true, true, true, true};
                break;
            default:
                notes[0] = true; // At least root
        }
        
        // Rotate to root note
        std::rotate(notes.begin(), notes.begin() + rootNote, notes.end());
        
        return notes;
    }
    
    juce::String getName() const {
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        const char* typeNames[] = {"Major", "Minor", "Harmonic Minor", "Melodic Minor",
                                   "Dorian", "Phrygian", "Lydian", "Mixolydian",
                                   "Pentatonic", "Blues", "Chromatic"};
        
        return juce::String(noteNames[rootNote]) + " " + typeNames[(int)type];
    }
};

//==============================================================================
// Chord Stamp - Preset de acordes
//==============================================================================
struct ChordStamp {
    enum class Type {
        Major, Minor, Dim, Aug,
        Maj7, Min7, Dom7, MinMaj7,
        Sus2, Sus4, Add9, Sixth
    };
    
    Type type = Type::Major;
    juce::String name;
    std::vector<int> intervals; // Intervalos desde root
    
    static ChordStamp getMajor() {
        ChordStamp chord;
        chord.type = Type::Major;
        chord.name = "Major";
        chord.intervals = {0, 4, 7}; // Root, Major 3rd, Perfect 5th
        return chord;
    }
    
    static ChordStamp getMinor() {
        ChordStamp chord;
        chord.type = Type::Minor;
        chord.name = "Minor";
        chord.intervals = {0, 3, 7}; // Root, Minor 3rd, Perfect 5th
        return chord;
    }
    
    static ChordStamp getDominant7() {
        ChordStamp chord;
        chord.type = Type::Dom7;
        chord.name = "Dominant 7th";
        chord.intervals = {0, 4, 7, 10};
        return chord;
    }
    
    static ChordStamp getMajor7() {
        ChordStamp chord;
        chord.type = Type::Maj7;
        chord.name = "Major 7th";
        chord.intervals = {0, 4, 7, 11};
        return chord;
    }
    
    static std::vector<ChordStamp> getAllChords() {
        return {
            getMajor(), getMinor(), getDominant7(), getMajor7(),
            // Add more...
        };
    }
};

//==============================================================================
// Note Properties - Propiedades extendidas de nota
//==============================================================================
struct NoteProperties {
    float velocity = 0.8f;
    float pan = 0.0f; // -1.0 to 1.0
    float modulation = 0.0f;
    float pitchBend = 0.0f;
    juce::Colour colour = juce::Colour(0xff4a90ff);
    bool muted = false;
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("velocity", velocity);
        obj->setProperty("pan", pan);
        obj->setProperty("modulation", modulation);
        obj->setProperty("pitchBend", pitchBend);
        obj->setProperty("colour", colour.toString());
        obj->setProperty("muted", muted);
        return juce::var(obj);
    }
};

//==============================================================================
// Scale Highlighter - Resalta notas de la escala
//==============================================================================
class ScaleHighlighter : public juce::Component {
public:
    ScaleHighlighter() = default;
    
    void paint(juce::Graphics& g) override {
        if (!scale.has_value()) return;
        
        auto scaleNotes = scale->getScaleNotes();
        float keyHeight = (float)getHeight() / 128.0f;
        
        for (int note = 0; note < 128; ++note) {
            int noteInOctave = note % 12;
            
            if (scaleNotes[noteInOctave]) {
                float y = getHeight() - (note + 1) * keyHeight;
                
                // Highlight root notes stronger
                float alpha = (noteInOctave == scale->rootNote) ? 0.3f : 0.15f;
                
                g.setColour(juce::Colour(0xff4aff90).withAlpha(alpha));
                g.fillRect(0.0f, y, (float)getWidth(), keyHeight);
            }
        }
    }
    
    void setScale(const MusicalScale& newScale) {
        scale = newScale;
        repaint();
    }
    
    void clearScale() {
        scale.reset();
        repaint();
    }
    
    std::optional<MusicalScale> getScale() const { return scale; }
    
private:
    std::optional<MusicalScale> scale;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleHighlighter)
};

//==============================================================================
// Velocity Layer Editor - Editor de capas de velocity
//==============================================================================
class VelocityLayerEditor : public juce::Component {
public:
    VelocityLayerEditor() {
        setOpaque(true);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Grid lines
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int i = 0; i <= 4; ++i) {
            float y = getHeight() * (i / 4.0f);
            g.drawLine(0, y, (float)getWidth(), y, 1.0f);
        }
        
        // Draw velocity bars
        if (notes.empty()) return;
        
        float barWidth = (float)getWidth() / (float)notes.size();
        
        for (size_t i = 0; i < notes.size(); ++i) {
            float x = i * barWidth;
            float height = getHeight() * notes[i].velocity;
            float y = getHeight() - height;
            
            // Color based on velocity
            juce::Colour barColour;
            if (notes[i].velocity < 0.33f)
                barColour = juce::Colour(0xff4aff90); // Green
            else if (notes[i].velocity < 0.66f)
                barColour = juce::Colour(0xfffff036); // Yellow
            else
                barColour = juce::Colour(0xffff3636); // Red
            
            g.setColour(barColour);
            g.fillRect(x + 1, y, barWidth - 2, height);
            
            // Border
            g.setColour(barColour.darker(0.3f));
            g.drawRect(x + 1, y, barWidth - 2, height, 1.0f);
        }
        
        // Draw editing line (if dragging)
        if (isDragging && dragStartNote >= 0 && dragEndNote >= 0) {
            int start = std::min(dragStartNote, dragEndNote);
            int end = std::max(dragStartNote, dragEndNote);
            
            g.setColour(juce::Colour(0xffff8736).withAlpha(0.5f));
            g.drawLine((float)(start * barWidth), (float)dragY,
                      (float)((end + 1) * barWidth), (float)dragY, 2.0f);
        }
    }
    
    void mouseDown(const juce::MouseEvent& event) override {
        isDragging = true;
        dragStartNote = getNoteAtX(event.x);
        dragY = event.y;
        
        if (dragStartNote >= 0 && dragStartNote < (int)notes.size()) {
            float newVelocity = 1.0f - ((float)event.y / (float)getHeight());
            notes[dragStartNote].velocity = juce::jlimit(0.0f, 1.0f, newVelocity);
            repaint();
        }
    }
    
    void mouseDrag(const juce::MouseEvent& event) override {
        dragEndNote = getNoteAtX(event.x);
        dragY = event.y;
        
        // Apply velocity to range
        int start = std::min(dragStartNote, dragEndNote);
        int end = std::max(dragStartNote, dragEndNote);
        
        float newVelocity = 1.0f - ((float)event.y / (float)getHeight());
        newVelocity = juce::jlimit(0.0f, 1.0f, newVelocity);
        
        for (int i = start; i <= end && i < (int)notes.size(); ++i) {
            notes[i].velocity = newVelocity;
        }
        
        repaint();
        
        if (onVelocityChanged) {
            onVelocityChanged();
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        isDragging = false;
        dragStartNote = -1;
        dragEndNote = -1;
        repaint();
    }
    
    void setNotes(const std::vector<NoteProperties>& newNotes) {
        notes = newNotes;
        repaint();
    }
    
    const std::vector<NoteProperties>& getNotes() const { return notes; }
    
    std::function<void()> onVelocityChanged;
    
private:
    std::vector<NoteProperties> notes;
    bool isDragging = false;
    int dragStartNote = -1;
    int dragEndNote = -1;
    int dragY = 0;
    
    int getNoteAtX(int x) const {
        if (notes.empty()) return -1;
        float barWidth = (float)getWidth() / (float)notes.size();
        return juce::jlimit(0, (int)notes.size() - 1, (int)(x / barWidth));
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityLayerEditor)
};

//==============================================================================
// Chord Stamp Browser - Browser de chord stamps
//==============================================================================
class ChordStampBrowser : public juce::Component {
public:
    ChordStampBrowser() {
        chords = ChordStamp::getAllChords();
        
        for (size_t i = 0; i < chords.size(); ++i) {
            auto* button = new juce::TextButton(chords[i].name);
            button->onClick = [this, i] { selectChord(i); };
            chordButtons.add(button);
            addAndMakeVisible(button);
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(4);
        int buttonHeight = 32;
        
        for (auto* button : chordButtons) {
            button->setBounds(bounds.removeFromTop(buttonHeight));
            bounds.removeFromTop(2);
        }
    }
    
    void selectChord(size_t index) {
        if (index < chords.size() && onChordSelected) {
            onChordSelected(chords[index]);
        }
    }
    
    std::function<void(const ChordStamp&)> onChordSelected;
    
private:
    std::vector<ChordStamp> chords;
    juce::OwnedArray<juce::TextButton> chordButtons;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordStampBrowser)
};

//==============================================================================
// Note Properties Panel - Panel de propiedades de nota
//==============================================================================
class NotePropertiesPanel : public juce::Component {
public:
    NotePropertiesPanel() {
        addAndMakeVisible(velocitySlider);
        velocitySlider.setRange(0.0, 1.0);
        velocitySlider.setTextValueSuffix(" Velocity");
        
        addAndMakeVisible(panSlider);
        panSlider.setRange(-1.0, 1.0);
        panSlider.setTextValueSuffix(" Pan");
        
        addAndMakeVisible(modSlider);
        modSlider.setRange(0.0, 1.0);
        modSlider.setTextValueSuffix(" Mod");
        
        addAndMakeVisible(pitchSlider);
        pitchSlider.setRange(-1.0, 1.0);
        pitchSlider.setTextValueSuffix(" Pitch");
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        g.setColour(juce::Colour(0xffdddddd));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("NOTE PROPERTIES", getLocalBounds().removeFromTop(24), 
                  juce::Justification::centred);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(8);
        bounds.removeFromTop(28);
        
        velocitySlider.setBounds(bounds.removeFromTop(60));
        bounds.removeFromTop(4);
        panSlider.setBounds(bounds.removeFromTop(60));
        bounds.removeFromTop(4);
        modSlider.setBounds(bounds.removeFromTop(60));
        bounds.removeFromTop(4);
        pitchSlider.setBounds(bounds.removeFromTop(60));
    }
    
    void setNoteProperties(const NoteProperties& props) {
        velocitySlider.setValue(props.velocity, juce::dontSendNotification);
        panSlider.setValue(props.pan, juce::dontSendNotification);
        modSlider.setValue(props.modulation, juce::dontSendNotification);
        pitchSlider.setValue(props.pitchBend, juce::dontSendNotification);
    }
    
    NoteProperties getNoteProperties() const {
        NoteProperties props;
        props.velocity = (float)velocitySlider.getValue();
        props.pan = (float)panSlider.getValue();
        props.modulation = (float)modSlider.getValue();
        props.pitchBend = (float)pitchSlider.getValue();
        return props;
    }
    
private:
    juce::Slider velocitySlider;
    juce::Slider panSlider;
    juce::Slider modSlider;
    juce::Slider pitchSlider;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotePropertiesPanel)
};

} // namespace GUI
} // namespace OmegaStudio
