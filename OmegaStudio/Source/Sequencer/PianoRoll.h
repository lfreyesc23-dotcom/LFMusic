#pragma once

#include <JuceHeader.h>
#include <vector>
#include <set>
#include <memory>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @struct MIDINote
 * @brief Representa una nota MIDI con posición, duración y velocity
 */
struct MIDINote {
    int noteNumber { 60 };        // C4
    double startTime { 0.0 };     // En beats
    double duration { 1.0 };      // En beats
    uint8_t velocity { 100 };
    int channel { 1 };
    bool selected { false };
    juce::Uuid id;
    
    MIDINote() : id(juce::Uuid()) {}
    
    double getEndTime() const { return startTime + duration; }
    
    bool overlaps(const MIDINote& other) const {
        return !(getEndTime() <= other.startTime || startTime >= other.getEndTime());
    }
    
    bool contains(double time) const {
        return time >= startTime && time < getEndTime();
    }
    
    juce::Rectangle<float> getBounds(float pixelsPerBeat, float noteHeight, int lowestNote) const {
        float x = startTime * pixelsPerBeat;
        float y = (127 - noteNumber - lowestNote) * noteHeight;
        float w = duration * pixelsPerBeat;
        float h = noteHeight;
        return {x, y, w, h};
    }
};

/**
 * @struct CCEvent
 * @brief Control Change event para automatización MIDI
 */
struct CCEvent {
    int controller { 1 };      // CC number (1 = mod wheel, 7 = volume, etc.)
    double time { 0.0 };       // En beats
    uint8_t value { 64 };      // 0-127
    int channel { 1 };
    bool selected { false };
    
    juce::Point<float> getPosition(float pixelsPerBeat, float height) const {
        float x = time * pixelsPerBeat;
        float y = height * (1.0f - value / 127.0f);
        return {x, y};
    }
};

/**
 * @class MIDIClip
 * @brief Contenedor de notas MIDI y automation
 */
class MIDIClip {
public:
    MIDIClip(const juce::String& name = "MIDI Clip")
        : name_(name) {}
    
    // Note management
    void addNote(const MIDINote& note) {
        notes_.push_back(note);
        sortNotes();
    }
    
    void removeNote(const juce::Uuid& id) {
        notes_.erase(
            std::remove_if(notes_.begin(), notes_.end(),
                [&id](const MIDINote& n) { return n.id == id; }),
            notes_.end()
        );
    }
    
    MIDINote* findNote(const juce::Uuid& id) {
        auto it = std::find_if(notes_.begin(), notes_.end(),
            [&id](const MIDINote& n) { return n.id == id; });
        return it != notes_.end() ? &(*it) : nullptr;
    }
    
    std::vector<MIDINote>& getNotes() { return notes_; }
    const std::vector<MIDINote>& getNotes() const { return notes_; }
    
    // CC Automation
    void addCCEvent(const CCEvent& event) {
        ccEvents_.push_back(event);
        sortCCEvents();
    }
    
    void removeCCEvent(int index) {
        if (index >= 0 && index < ccEvents_.size()) {
            ccEvents_.erase(ccEvents_.begin() + index);
        }
    }
    
    std::vector<CCEvent>& getCCEvents() { return ccEvents_; }
    const std::vector<CCEvent>& getCCEvents() const { return ccEvents_; }
    
    // Selection
    void selectAll() {
        for (auto& note : notes_) note.selected = true;
    }
    
    void deselectAll() {
        for (auto& note : notes_) note.selected = false;
    }
    
    std::vector<MIDINote*> getSelectedNotes() {
        std::vector<MIDINote*> selected;
        for (auto& note : notes_) {
            if (note.selected) selected.push_back(&note);
        }
        return selected;
    }
    
    // Quantization
    void quantize(double gridSize, float strength = 1.0f) {
        for (auto& note : notes_) {
            double quantizedStart = std::round(note.startTime / gridSize) * gridSize;
            note.startTime = note.startTime + (quantizedStart - note.startTime) * strength;
        }
        sortNotes();
    }
    
    void quantizeSelected(double gridSize, float strength = 1.0f) {
        for (auto& note : notes_) {
            if (note.selected) {
                double quantizedStart = std::round(note.startTime / gridSize) * gridSize;
                note.startTime = note.startTime + (quantizedStart - note.startTime) * strength;
            }
        }
        sortNotes();
    }
    
    // Transpose
    void transpose(int semitones) {
        for (auto& note : notes_) {
            if (note.selected) {
                note.noteNumber = juce::jlimit(0, 127, note.noteNumber + semitones);
            }
        }
    }
    
    // Velocity scaling
    void scaleVelocity(float scale) {
        for (auto& note : notes_) {
            if (note.selected) {
                note.velocity = juce::jlimit(1, 127, (int)(note.velocity * scale));
            }
        }
    }
    
    // Humanize
    void humanize(float timingAmount, float velocityAmount) {
        auto& rand = juce::Random::getSystemRandom();
        for (auto& note : notes_) {
            if (note.selected) {
                // Timing randomization
                note.startTime += (rand.nextFloat() - 0.5f) * timingAmount;
                
                // Velocity randomization
                int velOffset = (rand.nextFloat() - 0.5f) * velocityAmount * 127;
                note.velocity = juce::jlimit(1, 127, note.velocity + velOffset);
            }
        }
        sortNotes();
    }
    
    // Length
    double getLength() const {
        if (notes_.empty()) return 4.0;  // Default 4 beats
        double maxEnd = 0.0;
        for (const auto& note : notes_) {
            maxEnd = std::max(maxEnd, note.getEndTime());
        }
        return maxEnd;
    }
    
    // To MIDI
    juce::MidiMessageSequence toMidiSequence(double clipStartTime = 0.0) const {
        juce::MidiMessageSequence sequence;
        
        for (const auto& note : notes_) {
            double noteOnTime = (clipStartTime + note.startTime);
            double noteOffTime = noteOnTime + note.duration;
            
            sequence.addEvent(juce::MidiMessage::noteOn(
                note.channel, note.noteNumber, note.velocity
            ), noteOnTime);
            
            sequence.addEvent(juce::MidiMessage::noteOff(
                note.channel, note.noteNumber
            ), noteOffTime);
        }
        
        sequence.updateMatchedPairs();
        return sequence;
    }
    
    const juce::String& getName() const { return name_; }
    void setName(const juce::String& name) { name_ = name; }
    
private:
    void sortNotes() {
        std::sort(notes_.begin(), notes_.end(),
            [](const MIDINote& a, const MIDINote& b) {
                return a.startTime < b.startTime;
            });
    }
    
    void sortCCEvents() {
        std::sort(ccEvents_.begin(), ccEvents_.end(),
            [](const CCEvent& a, const CCEvent& b) {
                return a.time < b.time;
            });
    }
    
    juce::String name_;
    std::vector<MIDINote> notes_;
    std::vector<CCEvent> ccEvents_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIClip)
};

/**
 * @class PianoRollComponent
 * @brief Editor visual de MIDI con velocity lanes y automation
 */
class PianoRollComponent : public juce::Component,
                           public juce::ScrollBar::Listener {
public:
    PianoRollComponent() {
        clip_ = std::make_shared<MIDIClip>("New Clip");
        
        // Scrollbars
        horizontalScrollbar_.setAutoHide(false);
        horizontalScrollbar_.addListener(this);
        addAndMakeVisible(horizontalScrollbar_);
        
        verticalScrollbar_.setAutoHide(false);
        verticalScrollbar_.setVertical(true);
        verticalScrollbar_.addListener(this);
        addAndMakeVisible(verticalScrollbar_);
        
        // Velocity lane
        velocityLaneHeight_ = 100;
        
        setSize(800, 600);
        updateScrollbarRanges();
    }
    
    ~PianoRollComponent() override {
        horizontalScrollbar_.removeListener(this);
        verticalScrollbar_.removeListener(this);
    }
    
    void setClip(std::shared_ptr<MIDIClip> clip) {
        clip_ = clip;
        repaint();
    }
    
    void setPixelsPerBeat(float ppb) {
        pixelsPerBeat_ = juce::jlimit(20.0f, 200.0f, ppb);
        updateScrollbarRanges();
        repaint();
    }
    
    void setGridSize(double beats) {
        gridSize_ = beats;
        snapEnabled_ = (beats > 0.0);
        repaint();
    }
    
    void setSnapEnabled(bool enabled) {
        snapEnabled_ = enabled;
    }
    
    void paint(juce::Graphics& g) override {
        auto pianoRollArea = getPianoRollArea();
        auto velocityArea = getVelocityLaneArea();
        
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Draw piano keys
        drawPianoKeys(g, pianoRollArea);
        
        // Draw grid
        drawGrid(g, pianoRollArea);
        
        // Draw notes
        drawNotes(g, pianoRollArea);
        
        // Draw selection rectangle
        if (isSelecting_) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.fillRect(selectionRect_);
            g.setColour(juce::Colours::white);
            g.drawRect(selectionRect_, 1);
        }
        
        // Draw velocity lane
        drawVelocityLane(g, velocityArea);
        
        // Draw playhead
        if (isPlaying_) {
            float x = pianoRollArea.getX() + (playheadPosition_ * pixelsPerBeat_) - viewX_;
            g.setColour(juce::Colours::white);
            g.drawLine(x, 0, x, getHeight(), 2.0f);
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        
        // Scrollbars
        horizontalScrollbar_.setBounds(bounds.removeFromBottom(15));
        verticalScrollbar_.setBounds(bounds.removeFromRight(15));
        
        updateScrollbarRanges();
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        auto pianoRollArea = getPianoRollArea();
        auto velocityArea = getVelocityLaneArea();
        
        if (pianoRollArea.contains(e.getPosition())) {
            handlePianoRollMouseDown(e);
        } else if (velocityArea.contains(e.getPosition())) {
            handleVelocityLaneMouseDown(e);
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        if (isDrawing_) {
            handleDrawing(e);
        } else if (isMoving_) {
            handleMoving(e);
        } else if (isResizing_) {
            handleResizing(e);
        } else if (isSelecting_) {
            handleSelecting(e);
        }
    }
    
    void mouseUp(const juce::MouseEvent& e) override {
        if (isSelecting_) {
            finalizeSelection();
        }
        
        isDrawing_ = false;
        isMoving_ = false;
        isResizing_ = false;
        isSelecting_ = false;
        currentNote_ = juce::Uuid();
    }
    
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override {
        if (e.mods.isCommandDown()) {
            // Zoom
            float newPPB = pixelsPerBeat_ + wheel.deltaY * 10.0f;
            setPixelsPerBeat(newPPB);
        } else if (e.mods.isShiftDown()) {
            // Horizontal scroll
            viewX_ += wheel.deltaY * 50.0f;
            viewX_ = juce::jlimit(0.0f, maxViewX_, viewX_);
            horizontalScrollbar_.setCurrentRange(viewX_, viewportWidth_);
            repaint();
        } else {
            // Vertical scroll
            viewY_ += wheel.deltaY * 50.0f;
            viewY_ = juce::jlimit(0.0f, maxViewY_, viewY_);
            verticalScrollbar_.setCurrentRange(viewY_, viewportHeight_);
            repaint();
        }
    }
    
    void scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) override {
        if (scrollBar == &horizontalScrollbar_) {
            viewX_ = newRangeStart;
        } else if (scrollBar == &verticalScrollbar_) {
            viewY_ = newRangeStart;
        }
        repaint();
    }
    
    // Playback
    void setPlayheadPosition(double beats) {
        playheadPosition_ = beats;
        repaint();
    }
    
    void setPlaying(bool playing) {
        isPlaying_ = playing;
        repaint();
    }
    
private:
    juce::Rectangle<int> getPianoRollArea() const {
        return getLocalBounds()
            .withTrimmedBottom(velocityLaneHeight_ + 15)
            .withTrimmedRight(15);
    }
    
    juce::Rectangle<int> getVelocityLaneArea() const {
        return juce::Rectangle<int>(
            0, 
            getHeight() - velocityLaneHeight_ - 15,
            getWidth() - 15,
            velocityLaneHeight_
        );
    }
    
    void drawPianoKeys(juce::Graphics& g, juce::Rectangle<int> area) {
        // White/black key pattern
        static const bool blackKeys[] = {false, true, false, true, false, false, true, false, true, false, true, false};
        
        for (int note = lowestNote_; note <= highestNote_; ++note) {
            float y = area.getY() + (highestNote_ - note) * noteHeight_ - viewY_;
            
            bool isBlack = blackKeys[note % 12];
            g.setColour(isBlack ? juce::Colour(0xff2a2a2a) : juce::Colour(0xff3a3a3a));
            g.fillRect(0.0f, y, 50.0f, noteHeight_);
            
            // Note name
            if (note % 12 == 0) {  // C notes
                g.setColour(juce::Colours::white.withAlpha(0.5f));
                g.drawText("C" + juce::String(note / 12 - 1), 
                          2, y, 46, noteHeight_, 
                          juce::Justification::centredLeft);
            }
        }
    }
    
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area) {
        g.setColour(juce::Colour(0xff2a2a2a));
        
        // Horizontal lines (notes)
        for (int note = lowestNote_; note <= highestNote_; ++note) {
            float y = area.getY() + (highestNote_ - note) * noteHeight_ - viewY_;
            g.drawHorizontalLine(y, area.getX() + 50, area.getRight());
        }
        
        // Vertical lines (beats)
        if (gridSize_ > 0.0) {
            float totalBeats = clip_->getLength();
            for (double beat = 0.0; beat <= totalBeats; beat += gridSize_) {
                float x = area.getX() + 50 + beat * pixelsPerBeat_ - viewX_;
                float thickness = (std::fmod(beat, 1.0) < 0.001) ? 2.0f : 1.0f;
                g.setOpacity(thickness / 2.0f);
                g.drawVerticalLine(x, area.getY(), area.getBottom());
            }
        }
    }
    
    void drawNotes(juce::Graphics& g, juce::Rectangle<int> area) {
        auto& notes = clip_->getNotes();
        
        for (const auto& note : notes) {
            auto bounds = note.getBounds(pixelsPerBeat_, noteHeight_, lowestNote_);
            bounds.translate(area.getX() + 50 - viewX_, area.getY() - viewY_);
            
            // Only draw if visible
            if (bounds.getRight() < area.getX() || bounds.getX() > area.getRight()) continue;
            if (bounds.getBottom() < area.getY() || bounds.getY() > area.getBottom()) continue;
            
            // Color based on velocity
            float brightness = note.velocity / 127.0f * 0.6f + 0.4f;
            juce::Colour col = note.selected 
                ? juce::Colours::orange 
                : juce::Colour::fromHSV(0.55f, 0.7f, brightness, 1.0f);
            
            g.setColour(col);
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(col.darker(0.3f));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        }
    }
    
    void drawVelocityLane(juce::Graphics& g, juce::Rectangle<int> area) {
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRect(area);
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRect(area);
        
        auto& notes = clip_->getNotes();
        
        for (const auto& note : notes) {
            if (note.selected) {
                float x = area.getX() + 50 + note.startTime * pixelsPerBeat_ - viewX_;
                float w = note.duration * pixelsPerBeat_;
                float h = (note.velocity / 127.0f) * area.getHeight();
                float y = area.getBottom() - h;
                
                g.setColour(juce::Colours::orange);
                g.fillRect(x, y, w, h);
            }
        }
    }
    
    void handlePianoRollMouseDown(const juce::MouseEvent& e) {
        auto pianoRollArea = getPianoRollArea();
        auto pos = e.getPosition() - pianoRollArea.getPosition();
        
        double beat = (pos.x - 50 + viewX_) / pixelsPerBeat_;
        int note = highestNote_ - (pos.y + viewY_) / noteHeight_;
        
        if (snapEnabled_ && gridSize_ > 0.0) {
            beat = std::round(beat / gridSize_) * gridSize_;
        }
        
        // Check if clicking on existing note
        MIDINote* clickedNote = findNoteAt(beat, note);
        
        if (clickedNote) {
            if (!e.mods.isShiftDown()) {
                clip_->deselectAll();
            }
            clickedNote->selected = true;
            currentNote_ = clickedNote->id;
            
            // Check if near edge (resize) or center (move)
            float noteX = clickedNote->startTime * pixelsPerBeat_ - viewX_ + 50;
            float noteW = clickedNote->duration * pixelsPerBeat_;
            if (pos.x > noteX + noteW - 10) {
                isResizing_ = true;
            } else {
                isMoving_ = true;
                dragStartBeat_ = beat;
                dragStartNote_ = note;
            }
        } else {
            // Start drawing new note
            if (!e.mods.isShiftDown()) {
                clip_->deselectAll();
            }
            
            if (e.mods.isAltDown()) {
                // Start selection rectangle
                isSelecting_ = true;
                selectionStart_ = e.getPosition();
                selectionRect_ = juce::Rectangle<int>(selectionStart_, selectionStart_);
            } else {
                // Draw new note
                isDrawing_ = true;
                MIDINote newNote;
                newNote.noteNumber = juce::jlimit(0, 127, note);
                newNote.startTime = std::max(0.0, beat);
                newNote.duration = gridSize_ > 0.0 ? gridSize_ : 0.25;
                newNote.velocity = 100;
                newNote.selected = true;
                clip_->addNote(newNote);
                currentNote_ = newNote.id;
            }
        }
        
        repaint();
    }
    
    void handleVelocityLaneMouseDown(const juce::MouseEvent& e) {
        // Edit velocity of selected notes
        auto selectedNotes = clip_->getSelectedNotes();
        if (!selectedNotes.empty()) {
            auto area = getVelocityLaneArea();
            float normalizedY = 1.0f - (e.y - area.getY()) / (float)area.getHeight();
            uint8_t newVelocity = juce::jlimit(1, 127, (int)(normalizedY * 127));
            
            for (auto* note : selectedNotes) {
                note->velocity = newVelocity;
            }
            repaint();
        }
    }
    
    void handleDrawing(const juce::MouseEvent& e) {
        if (auto* note = clip_->findNote(currentNote_)) {
            auto pianoRollArea = getPianoRollArea();
            auto pos = e.getPosition() - pianoRollArea.getPosition();
            double beat = (pos.x - 50 + viewX_) / pixelsPerBeat_;
            
            if (snapEnabled_ && gridSize_ > 0.0) {
                beat = std::round(beat / gridSize_) * gridSize_;
            }
            
            note->duration = std::max(gridSize_ > 0.0 ? gridSize_ : 0.125, beat - note->startTime);
            repaint();
        }
    }
    
    void handleMoving(const juce::MouseEvent& e) {
        auto pianoRollArea = getPianoRollArea();
        auto pos = e.getPosition() - pianoRollArea.getPosition();
        
        double beat = (pos.x - 50 + viewX_) / pixelsPerBeat_;
        int note = highestNote_ - (pos.y + viewY_) / noteHeight_;
        
        if (snapEnabled_ && gridSize_ > 0.0) {
            beat = std::round(beat / gridSize_) * gridSize_;
        }
        
        double deltaBeat = beat - dragStartBeat_;
        int deltaNote = note - dragStartNote_;
        
        auto selectedNotes = clip_->getSelectedNotes();
        for (auto* n : selectedNotes) {
            n->startTime = std::max(0.0, n->startTime + deltaBeat);
            n->noteNumber = juce::jlimit(0, 127, n->noteNumber + deltaNote);
        }
        
        dragStartBeat_ = beat;
        dragStartNote_ = note;
        repaint();
    }
    
    void handleResizing(const juce::MouseEvent& e) {
        if (auto* note = clip_->findNote(currentNote_)) {
            auto pianoRollArea = getPianoRollArea();
            auto pos = e.getPosition() - pianoRollArea.getPosition();
            double beat = (pos.x - 50 + viewX_) / pixelsPerBeat_;
            
            if (snapEnabled_ && gridSize_ > 0.0) {
                beat = std::round(beat / gridSize_) * gridSize_;
            }
            
            note->duration = std::max(gridSize_ > 0.0 ? gridSize_ : 0.125, beat - note->startTime);
            repaint();
        }
    }
    
    void handleSelecting(const juce::MouseEvent& e) {
        selectionRect_ = juce::Rectangle<int>(selectionStart_, e.getPosition());
        repaint();
    }
    
    void finalizeSelection() {
        auto pianoRollArea = getPianoRollArea();
        
        for (auto& note : clip_->getNotes()) {
            auto bounds = note.getBounds(pixelsPerBeat_, noteHeight_, lowestNote_);
            bounds.translate(pianoRollArea.getX() + 50 - viewX_, pianoRollArea.getY() - viewY_);
            
            if (selectionRect_.intersects(bounds.toNearestInt())) {
                note.selected = true;
            }
        }
    }
    
    MIDINote* findNoteAt(double beat, int note) {
        auto& notes = clip_->getNotes();
        for (auto& n : notes) {
            if (n.noteNumber == note && n.contains(beat)) {
                return &n;
            }
        }
        return nullptr;
    }
    
    void updateScrollbarRanges() {
        float totalWidth = clip_->getLength() * pixelsPerBeat_ + 100;
        float totalHeight = (highestNote_ - lowestNote_ + 1) * noteHeight_;
        
        viewportWidth_ = getWidth() - 15;
        viewportHeight_ = getHeight() - velocityLaneHeight_ - 30;
        
        maxViewX_ = std::max(0.0f, totalWidth - viewportWidth_);
        maxViewY_ = std::max(0.0f, totalHeight - viewportHeight_);
        
        horizontalScrollbar_.setRangeLimits(0.0, totalWidth);
        horizontalScrollbar_.setCurrentRange(viewX_, viewportWidth_);
        
        verticalScrollbar_.setRangeLimits(0.0, totalHeight);
        verticalScrollbar_.setCurrentRange(viewY_, viewportHeight_);
    }
    
    std::shared_ptr<MIDIClip> clip_;
    
    // View state
    float pixelsPerBeat_ { 40.0f };
    float noteHeight_ { 15.0f };
    int lowestNote_ { 0 };
    int highestNote_ { 127 };
    double gridSize_ { 0.25 };
    bool snapEnabled_ { true };
    
    float viewX_ { 0.0f };
    float viewY_ { 0.0f };
    float maxViewX_ { 0.0f };
    float maxViewY_ { 0.0f };
    float viewportWidth_ { 800.0f };
    float viewportHeight_ { 600.0f };
    
    int velocityLaneHeight_;
    
    // Interaction state
    bool isDrawing_ { false };
    bool isMoving_ { false };
    bool isResizing_ { false };
    bool isSelecting_ { false };
    juce::Uuid currentNote_;
    double dragStartBeat_ { 0.0 };
    int dragStartNote_ { 0 };
    juce::Point<int> selectionStart_;
    juce::Rectangle<int> selectionRect_;
    
    // Playback
    bool isPlaying_ { false };
    double playheadPosition_ { 0.0 };
    
    // Scrollbars
    juce::ScrollBar horizontalScrollbar_ { false };
    juce::ScrollBar verticalScrollbar_ { true };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};

} // namespace Sequencer
} // namespace OmegaStudio
