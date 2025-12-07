#pragma once

#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio::GUI {

/**
 * @brief MIDI Note representation for piano roll
 */
struct MIDINote
{
    int pitch{60};        // MIDI note number (0-127)
    double startTime{0.0};   // In beats
    double duration{1.0};    // In beats
    int velocity{100};    // 0-127
    bool selected{false};
    
    juce::Rectangle<float> bounds; // Screen coordinates
};

/**
 * @brief Interactive Piano Roll - Edición completa de notas MIDI
 */
class InteractivePianoRoll : public juce::Component
{
public:
    InteractivePianoRoll()
    {
        setSize(800, 600);
        
        // Add some example notes
        addNote(60, 0.0, 1.0, 100);  // C4
        addNote(64, 1.0, 1.0, 100);  // E4
        addNote(67, 2.0, 1.0, 100);  // G4
        addNote(72, 3.0, 1.0, 100);  // C5
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Header
        auto header = bounds.removeFromTop(40);
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillRect(header);
        
        // Title
        g.setColour(juce::Colour(0xffff8c00));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("🎹 PIANO ROLL", header.reduced(10, 0), 
                  juce::Justification::centredLeft);
        
        // Scale info
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawText("Scale: C Major | Snap: 1/4", header.removeFromRight(200).reduced(5, 0),
                  juce::Justification::centredRight);
        
        // Piano keys (left side)
        auto pianoKeys = bounds.removeFromLeft(60);
        drawPianoKeys(g, pianoKeys);
        
        // Grid area
        auto gridArea = bounds;
        drawGrid(g, gridArea);
        drawNotes(g, gridArea);
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        auto pos = e.getPosition();
        
        // Check if clicking on existing note
        for (auto& note : notes_)
        {
            if (note.bounds.contains(pos.toFloat()))
            {
                // Select note
                if (!e.mods.isShiftDown())
                {
                    // Deselect all others
                    for (auto& n : notes_)
                        n.selected = false;
                }
                note.selected = true;
                draggingNote_ = &note;
                dragStartPos_ = pos;
                dragStartNotePitch_ = note.pitch;
                dragStartNoteTime_ = note.startTime;
                repaint();
                return;
            }
        }
        
        // Create new note
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Skip header
        auto gridArea = bounds.removeFromLeft(bounds.getWidth() - 60);
        
        if (gridArea.contains(pos))
        {
            int pitch = screenYToPitch(pos.y - 40);
            double time = screenXToTime(pos.x - 60);
            
            // Snap to grid
            time = std::round(time * snapDivision_) / snapDivision_;
            
            addNote(pitch, time, 1.0 / snapDivision_, 100);
            repaint();
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggingNote_)
        {
            auto delta = e.getPosition() - dragStartPos_;
            
            // Update pitch (vertical)
            int pitchDelta = -delta.y / noteHeight_;
            draggingNote_->pitch = juce::jlimit(0, 127, dragStartNotePitch_ + pitchDelta);
            
            // Update time (horizontal)
            double timeDelta = delta.x / pixelsPerBeat_;
            draggingNote_->startTime = juce::jmax(0.0, dragStartNoteTime_ + timeDelta);
            
            // Snap to grid
            draggingNote_->startTime = std::round(draggingNote_->startTime * snapDivision_) / snapDivision_;
            
            repaint();
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        draggingNote_ = nullptr;
    }
    
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        // Delete note on double click
        auto pos = e.getPosition();
        
        for (int i = notes_.size() - 1; i >= 0; --i)
        {
            if (notes_[i].bounds.contains(pos.toFloat()))
            {
                notes_.erase(notes_.begin() + i);
                repaint();
                return;
            }
        }
    }
    
    void addNote(int pitch, double startTime, double duration, int velocity)
    {
        MIDINote note;
        note.pitch = pitch;
        note.startTime = startTime;
        note.duration = duration;
        note.velocity = velocity;
        notes_.push_back(note);
        repaint();
    }
    
    const std::vector<MIDINote>& getNotes() const { return notes_; }
    
    void setSnapDivision(double division) 
    { 
        snapDivision_ = division;
        repaint();
    }

private:
    void drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> area)
    {
        int numKeys = 128;
        int keyHeight = noteHeight_;
        
        for (int i = 0; i < numKeys; ++i)
        {
            int pitch = 127 - i;
            int y = i * keyHeight + 40;
            
            if (y > area.getBottom())
                break;
            
            // Key color (white or black)
            bool isBlackKey = isBlackNote(pitch);
            juce::Colour keyColor = isBlackKey ? 
                juce::Colour(0xff333333) : juce::Colour(0xff555555);
            
            // Highlight C notes
            if (pitch % 12 == 0)
                keyColor = juce::Colour(0xff666666);
            
            g.setColour(keyColor);
            g.fillRect(area.getX(), y, area.getWidth(), keyHeight - 1);
            
            // Key border
            g.setColour(juce::Colour(0xff222222));
            g.drawRect(area.getX(), y, area.getWidth(), keyHeight);
            
            // Note name for C notes
            if (pitch % 12 == 0)
            {
                g.setColour(juce::Colours::white);
                g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
                g.drawText("C" + juce::String(pitch / 12 - 1), 
                          area.getX() + 5, y, area.getWidth() - 10, keyHeight,
                          juce::Justification::centredLeft, false);
            }
        }
    }
    
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        // Horizontal lines (pitch)
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int i = 0; i < 128; ++i)
        {
            int y = i * noteHeight_ + 40;
            if (y > area.getBottom())
                break;
            
            // Darker lines for C notes
            if ((127 - i) % 12 == 0)
                g.setColour(juce::Colour(0xff3a3a3a));
            else
                g.setColour(juce::Colour(0xff2a2a2a));
            
            g.drawHorizontalLine(y, static_cast<float>(area.getX()), 
                               static_cast<float>(area.getRight()));
        }
        
        // Vertical lines (time)
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int i = 0; i < 32; ++i)
        {
            int x = area.getX() + static_cast<int>(i * pixelsPerBeat_);
            if (x > area.getRight())
                break;
            
            // Darker lines for bar boundaries
            if (i % 4 == 0)
                g.setColour(juce::Colour(0xff3a3a3a));
            else
                g.setColour(juce::Colour(0xff2a2a2a));
            
            g.drawVerticalLine(x, static_cast<float>(area.getY()), 
                             static_cast<float>(area.getBottom()));
        }
    }
    
    void drawNotes(juce::Graphics& g, juce::Rectangle<int> area)
    {
        for (auto& note : notes_)
        {
            // Calculate screen position
            int x = area.getX() + static_cast<int>(note.startTime * pixelsPerBeat_);
            int y = 40 + (127 - note.pitch) * noteHeight_;
            int width = static_cast<int>(note.duration * pixelsPerBeat_);
            int height = noteHeight_ - 2;
            
            // Store bounds for hit testing
            note.bounds = juce::Rectangle<float>(static_cast<float>(x), 
                                                 static_cast<float>(y), 
                                                 static_cast<float>(width), 
                                                 static_cast<float>(height));
            
            // Note color based on velocity
            float velocityNorm = note.velocity / 127.0f;
            juce::Colour noteColor = juce::Colour(0xff00ff00)
                .withBrightness(0.4f + velocityNorm * 0.6f);
            
            if (note.selected)
                noteColor = juce::Colour(0xffff8c00);
            
            // Draw note
            g.setColour(noteColor);
            g.fillRoundedRectangle(note.bounds, 3.0f);
            
            // Border
            g.setColour(noteColor.brighter(0.3f));
            g.drawRoundedRectangle(note.bounds, 3.0f, 1.5f);
            
            // Velocity bar
            float velBarWidth = note.bounds.getWidth() * velocityNorm;
            g.setColour(noteColor.darker(0.3f));
            g.fillRect(note.bounds.getX(), note.bounds.getBottom() - 3, 
                      velBarWidth, 3.0f);
        }
    }
    
    bool isBlackNote(int pitch) const
    {
        int note = pitch % 12;
        return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
    }
    
    int screenYToPitch(int y) const
    {
        return 127 - (y / noteHeight_);
    }
    
    double screenXToTime(int x) const
    {
        return x / pixelsPerBeat_;
    }
    
    std::vector<MIDINote> notes_;
    MIDINote* draggingNote_{nullptr};
    juce::Point<int> dragStartPos_;
    int dragStartNotePitch_{0};
    double dragStartNoteTime_{0.0};
    
    // Display settings
    int noteHeight_{15};
    double pixelsPerBeat_{100.0};
    double snapDivision_{4.0}; // 1/4 notes
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractivePianoRoll)
};

} // namespace OmegaStudio::GUI
