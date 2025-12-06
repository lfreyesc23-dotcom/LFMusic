/*
  ==============================================================================
    PianoRollEditor.cpp
    
    Implementación completa del Piano Roll visual
  ==============================================================================
*/

#include "PianoRollEditor.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// PianoKeyComponent
//==============================================================================

PianoKeyComponent::PianoKeyComponent(int noteNumber)
    : noteNumber_(noteNumber) {
}

void PianoKeyComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Color base
    juce::Colour keyColour = isBlackKey() 
        ? juce::Colour(0xff2a2a2a) 
        : juce::Colour(0xfff0f0f0);
    
    if (!inScale_) {
        keyColour = keyColour.darker(0.3f);
    }
    
    if (pressed_) {
        keyColour = juce::Colour(0xffff8c42); // FL Studio orange
    } else if (highlighted_) {
        keyColour = keyColour.brighter(0.2f);
    }
    
    g.setColour(keyColour);
    g.fillRect(bounds);
    
    // Border
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawRect(bounds, 1.0f);
    
    // Note name
    if (!isBlackKey() && noteNumber_ % 12 == 0) {
        g.setColour(juce::Colours::grey);
        g.setFont(10.0f);
        juce::String noteName = juce::MidiMessage::getMidiNoteName(noteNumber_, true, true, 4);
        g.drawText(noteName, bounds.reduced(2), juce::Justification::centredLeft);
    }
}

void PianoKeyComponent::mouseDown(const juce::MouseEvent& e) {
    pressed_ = true;
    repaint();
    if (onNoteEvent) onNoteEvent(noteNumber_, true);
}

void PianoKeyComponent::mouseUp(const juce::MouseEvent& e) {
    pressed_ = false;
    repaint();
    if (onNoteEvent) onNoteEvent(noteNumber_, false);
}

bool PianoKeyComponent::isBlackKey() const {
    int note = noteNumber_ % 12;
    return note == 1 || note == 3 || note == 6 || note == 8 || note == 10;
}

void PianoKeyComponent::setHighlighted(bool shouldHighlight) {
    highlighted_ = shouldHighlight;
    repaint();
}

void PianoKeyComponent::setInScale(bool inScale) {
    inScale_ = inScale;
    repaint();
}

//==============================================================================
// NoteComponent
//==============================================================================

NoteComponent::NoteComponent(const Sequencer::MIDINote& note, int noteIndex)
    : note_(note), noteIndex_(noteIndex) {
}

void NoteComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    // Color según estado
    juce::Colour noteColour = juce::Colour(0xff00bfff); // Cyan default
    
    if (ghostNote_) {
        noteColour = noteColour.withAlpha(0.3f);
    } else if (selected_) {
        noteColour = juce::Colour(0xffff8c42); // FL Orange
    }
    
    // Velocity brightness
    float velocityBrightness = note_.velocity;
    noteColour = noteColour.withBrightness(velocityBrightness);
    
    // Fill
    g.setColour(noteColour);
    g.fillRoundedRectangle(bounds.reduced(1), 3.0f);
    
    // Border
    g.setColour(noteColour.brighter(0.3f));
    g.drawRoundedRectangle(bounds.reduced(1), 3.0f, 1.5f);
    
    // Resize handle (right edge)
    if (!ghostNote_ && getWidth() > 20) {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillRect(getWidth() - 6.0f, 0.0f, 6.0f, (float)getHeight());
    }
}

void NoteComponent::mouseDown(const juce::MouseEvent& e) {
    dragStartPos_ = e.getPosition();
    
    // Check si está en resize handle (últimos 6px)
    if (e.x >= getWidth() - 6) {
        resizing_ = true;
    } else {
        if (onNoteSelected) onNoteSelected(noteIndex_);
    }
}

void NoteComponent::mouseDrag(const juce::MouseEvent& e) {
    auto delta = e.getPosition() - dragStartPos_;
    
    if (resizing_) {
        if (onNoteResized) {
            double newLength = note_.duration + (delta.x / 100.0); // Assuming 100 ppb
            newLength = std::max(0.0625, newLength); // Min 1/16
            onNoteResized(noteIndex_, newLength);
        }
    } else {
        if (onNoteMoved) onNoteMoved(noteIndex_, delta);
    }
}

void NoteComponent::mouseUp(const juce::MouseEvent& e) {
    resizing_ = false;
}

void NoteComponent::setSelected(bool selected) {
    selected_ = selected;
    repaint();
}

void NoteComponent::setGhostNote(bool isGhost) {
    ghostNote_ = isGhost;
    repaint();
}

//==============================================================================
// VelocityLaneComponent
//==============================================================================

VelocityLaneComponent::VelocityLaneComponent() {
}

void VelocityLaneComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Background
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRect(bounds);
    
    // Grid lines
    g.setColour(juce::Colour(0xff3a3a3a));
    for (int i = 0; i <= 4; ++i) {
        int y = bounds.getHeight() * i / 4;
        g.drawHorizontalLine(y, 0.0f, (float)bounds.getWidth());
    }
    
    // Velocity bars
    for (size_t i = 0; i < notes_.size(); ++i) {
        const auto& note = notes_[i];
        
        float x = note.startTime * pixelsPerBeat_;
        float width = note.duration * pixelsPerBeat_;
        float height = bounds.getHeight() * note.velocity;
        
        bool isSelected = std::find(selectedNotes_.begin(), selectedNotes_.end(), i) 
                         != selectedNotes_.end();
        
        juce::Colour barColour = isSelected 
            ? juce::Colour(0xffff8c42) 
            : juce::Colour(0xff00bfff);
        
        g.setColour(barColour);
        g.fillRect(x, bounds.getHeight() - height, width, height);
        
        g.setColour(barColour.brighter());
        g.drawRect(x, bounds.getHeight() - height, width, height, 1.0f);
    }
}

void VelocityLaneComponent::mouseDown(const juce::MouseEvent& e) {
    // Find note at position
    for (size_t i = 0; i < notes_.size(); ++i) {
        float x = notes_[i].startTime * pixelsPerBeat_;
        float width = notes_[i].duration * pixelsPerBeat_;
        
        if (e.x >= x && e.x <= x + width) {
            float newVelocity = 1.0f - (e.y / (float)getHeight());
            newVelocity = std::clamp(newVelocity, 0.0f, 1.0f);
            if (onVelocityChanged) onVelocityChanged(i, newVelocity);
            break;
        }
    }
}

void VelocityLaneComponent::mouseDrag(const juce::MouseEvent& e) {
    mouseDown(e); // Same behavior
}

void VelocityLaneComponent::setNotes(const std::vector<Sequencer::MIDINote>& notes) {
    notes_ = notes;
    repaint();
}

void VelocityLaneComponent::setSelectedNotes(const std::vector<int>& indices) {
    selectedNotes_ = indices;
    repaint();
}

//==============================================================================
// PianoRollEditor
//==============================================================================

PianoRollEditor::PianoRollEditor() {
    setWantsKeyboardFocus(true);
    
    // Create piano keys
    for (int i = 0; i < NUM_KEYS; ++i) {
        pianoKeys_[i] = std::make_unique<PianoKeyComponent>(i);
        addAndMakeVisible(pianoKeys_[i].get());
    }
    
    // Velocity lane
    velocityLane_ = std::make_unique<VelocityLaneComponent>();
    addAndMakeVisible(velocityLane_.get());
    
    velocityLane_->onVelocityChanged = [this](int index, float velocity) {
        if (clip_ && index < (int)clip_->getNotes().size()) {
            auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
            notes[index].velocity = velocity;
            rebuildNoteComponents();
        }
    };
    
    // Scrollbars
    horizontalScrollBar_ = std::make_unique<juce::ScrollBar>(false);
    verticalScrollBar_ = std::make_unique<juce::ScrollBar>(true);
    addAndMakeVisible(horizontalScrollBar_.get());
    addAndMakeVisible(verticalScrollBar_.get());
    
    horizontalScrollBar_->addListener(this);
    verticalScrollBar_->addListener(this);
    
    // Timer for playhead
    startTimer(30); // 33 FPS
}

PianoRollEditor::~PianoRollEditor() {
    stopTimer();
}

void PianoRollEditor::setClip(std::shared_ptr<Sequencer::MIDIClip> clip) {
    clip_ = clip;
    rebuildNoteComponents();
    
    if (velocityLane_ && clip_) {
        velocityLane_->setNotes(clip_->getNotes());
    }
}

void PianoRollEditor::setScale(int rootNote, Sequencer::Scale scale) {
    scaleRootNote_ = rootNote;
    currentScale_ = scale;
    updateScaleHighlighting();
    updatePianoKeys();
    repaint();
}

void PianoRollEditor::setScaleVisible(bool visible) {
    scaleVisible_ = visible;
    updatePianoKeys();
    repaint();
}

void PianoRollEditor::addGhostTrack(std::shared_ptr<Sequencer::MIDIClip> ghostClip, 
                                   const juce::String& name, 
                                   juce::Colour colour) {
    ghostTracks_.push_back({ghostClip, name, colour});
    rebuildNoteComponents();
}

void PianoRollEditor::clearGhostTracks() {
    ghostTracks_.clear();
    rebuildNoteComponents();
}

void PianoRollEditor::setTool(PianoRollTool tool) {
    currentTool_ = tool;
    setMouseCursor(tool == PianoRollTool::Pencil ? juce::MouseCursor::CrosshairCursor 
            : tool == PianoRollTool::Delete ? juce::MouseCursor::NormalCursor
            : juce::MouseCursor::NormalCursor);
}

void PianoRollEditor::setGridSize(double beats) {
    gridSize_ = beats;
    repaint();
}

void PianoRollEditor::setSnapEnabled(bool enabled) {
    snapEnabled_ = enabled;
}

void PianoRollEditor::setPixelsPerBeat(float ppb) {
    pixelsPerBeat_ = std::clamp(ppb, 20.0f, 500.0f);
    if (velocityLane_) velocityLane_->setPixelsPerBeat(ppb);
    rebuildNoteComponents();
    resized();
    repaint();
}

void PianoRollEditor::setNoteHeight(float height) {
    noteHeight_ = std::clamp(height, 6.0f, 30.0f);
    resized();
    repaint();
}

void PianoRollEditor::setPlayheadPosition(double beats) {
    playheadPosition_ = beats;
    repaint();
}

std::vector<int> PianoRollEditor::getSelectedNotes() const {
    return selectedNoteIndices_;
}

void PianoRollEditor::selectAll() {
    selectedNoteIndices_.clear();
    for (int i = 0; i < (int)noteComponents_.size(); ++i) {
        selectedNoteIndices_.push_back(i);
        noteComponents_[i]->setSelected(true);
    }
}

void PianoRollEditor::selectNone() {
    for (auto& nc : noteComponents_) {
        nc->setSelected(false);
    }
    selectedNoteIndices_.clear();
}

void PianoRollEditor::deleteSelected() {
    if (!clip_ || selectedNoteIndices_.empty()) return;
    
    auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
    
    // Sort indices descending para remover de atrás hacia adelante
    std::sort(selectedNoteIndices_.begin(), selectedNoteIndices_.end(), std::greater<int>());
    
    for (int idx : selectedNoteIndices_) {
        if (idx < (int)notes.size()) {
            notes.erase(notes.begin() + idx);
        }
    }
    
    selectedNoteIndices_.clear();
    rebuildNoteComponents();
}

void PianoRollEditor::quantizeSelected(double gridSize) {
    if (!clip_ || selectedNoteIndices_.empty()) return;
    
    auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
    
    for (int idx : selectedNoteIndices_) {
        if (idx < (int)notes.size()) {
            double beats = notes[idx].startTime;
            double quantized = std::round(beats / gridSize) * gridSize;
            notes[idx].startTime = quantized;
        }
    }
    
    rebuildNoteComponents();
}

void PianoRollEditor::transposeSelected(int semitones) {
    if (!clip_ || selectedNoteIndices_.empty()) return;
    
    auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
    
    for (int idx : selectedNoteIndices_) {
        if (idx < (int)notes.size()) {
            int newNote = std::clamp(notes[idx].noteNumber + semitones, 0, 127);
            notes[idx].noteNumber = newNote;
        }
    }
    
    rebuildNoteComponents();
}

void PianoRollEditor::setVelocityForSelected(float velocity) {
    if (!clip_ || selectedNoteIndices_.empty()) return;
    
    auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
    
    for (int idx : selectedNoteIndices_) {
        if (idx < (int)notes.size()) {
            notes[idx].velocity = std::clamp(velocity, 0.0f, 1.0f);
        }
    }
    
    rebuildNoteComponents();
}

void PianoRollEditor::humanizeSelected(float timing, float velocity) {
    if (!clip_ || selectedNoteIndices_.empty()) return;
    
    auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
    juce::Random rng;
    
    for (int idx : selectedNoteIndices_) {
        if (idx < (int)notes.size()) {
            // Timing randomization
            float timeOffset = rng.nextFloat() * timing * 2.0f - timing;
            notes[idx].startTime += timeOffset * 0.1; // Max 10% of beat
            
            // Velocity randomization
            float velOffset = rng.nextFloat() * velocity * 2.0f - velocity;
            notes[idx].velocity = std::clamp(notes[idx].velocity + velOffset * 0.2f, 0.0f, 1.0f);
        }
    }
    
    rebuildNoteComponents();
}

void PianoRollEditor::chordizeSelected(Sequencer::ChordGenerator::ChordType type) {
    // Esta función necesita acceso al ChordGenerator
    // Por ahora placeholder
}

void PianoRollEditor::arpeggiateSelected(Sequencer::Arpeggiator::Pattern pattern, double noteLength) {
    // Placeholder
}

void PianoRollEditor::strumSelected(double strumTime) {
    // Placeholder
}

void PianoRollEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Background
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    // Área del grid (sin piano keys)
    auto gridArea = bounds.withLeft(PIANO_KEY_WIDTH)
                         .withBottom(bounds.getBottom() - VELOCITY_LANE_HEIGHT - 20);
    
    // Paint components
    paintGrid(g, gridArea);
    if (scaleVisible_) paintScaleHighlighting(g);
    paintNotes(g);
    paintPlayhead(g);
    
    // Lasso selection
    if (isLassoing_) {
        g.setColour(juce::Colour(0xffff8c42).withAlpha(0.3f));
        g.fillRect(lassoRectangle_);
        g.setColour(juce::Colour(0xffff8c42));
        g.drawRect(lassoRectangle_, 2);
    }
}

void PianoRollEditor::resized() {
    auto bounds = getLocalBounds();
    
    // Piano keys (left side)
    int keyAreaHeight = bounds.getHeight() - VELOCITY_LANE_HEIGHT - 20;
    for (int i = 0; i < NUM_KEYS; ++i) {
        int y = keyAreaHeight - (i + 1) * noteHeight_;
        pianoKeys_[i]->setBounds(0, (int)y, PIANO_KEY_WIDTH, (int)noteHeight_);
    }
    
    // Velocity lane (bottom)
    velocityLane_->setBounds(PIANO_KEY_WIDTH, 
                            bounds.getHeight() - VELOCITY_LANE_HEIGHT - 20,
                            bounds.getWidth() - PIANO_KEY_WIDTH - 20,
                            VELOCITY_LANE_HEIGHT);
    
    // Scrollbars
    horizontalScrollBar_->setBounds(PIANO_KEY_WIDTH, 
                                   bounds.getHeight() - 20,
                                   bounds.getWidth() - PIANO_KEY_WIDTH - 20, 
                                   20);
    
    verticalScrollBar_->setBounds(bounds.getWidth() - 20, 
                                 0, 
                                 20, 
                                 keyAreaHeight);
    
    // Update note components positions
    rebuildNoteComponents();
}

void PianoRollEditor::mouseDown(const juce::MouseEvent& e) {
    if (e.x < PIANO_KEY_WIDTH) return; // Piano keys area
    
    int noteNumber = getNoteNumberAtY(e.y);
    double startBeat = getBeatsAtX(e.x);
    
    if (currentTool_ == PianoRollTool::Pencil) {
        createNote(noteNumber, snapToGrid(startBeat));
    } else if (currentTool_ == PianoRollTool::Select) {
        lassoRectangle_ = juce::Rectangle<int>(e.getPosition(), e.getPosition());
        isLassoing_ = true;
    }
}

void PianoRollEditor::mouseDrag(const juce::MouseEvent& e) {
    if (isLassoing_) {
        lassoRectangle_ = juce::Rectangle<int>(lassoRectangle_.getTopLeft(), e.getPosition());
        repaint();
    }
}

void PianoRollEditor::mouseUp(const juce::MouseEvent& e) {
    if (isLassoing_) {
        // Select notes within lasso
        selectNone();
        for (size_t i = 0; i < noteComponents_.size(); ++i) {
            if (lassoRectangle_.intersects(noteComponents_[i]->getBounds())) {
                selectedNoteIndices_.push_back((int)i);
                noteComponents_[i]->setSelected(true);
            }
        }
        isLassoing_ = false;
        repaint();
    }
    
    if (e.mods.isRightButtonDown()) {
        showContextMenu();
    }
}

void PianoRollEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    if (e.mods.isCommandDown()) {
        // Zoom horizontal
        setPixelsPerBeat(pixelsPerBeat_ + wheel.deltaY * 20.0f);
    } else if (e.mods.isShiftDown()) {
        // Zoom vertical
        setNoteHeight(noteHeight_ + wheel.deltaY * 2.0f);
    } else {
        // Scroll
        verticalScrollBar_->setCurrentRangeStart(
            verticalScrollBar_->getCurrentRangeStart() - wheel.deltaY * 50.0);
    }
}

bool PianoRollEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) {
        deleteSelected();
        return true;
    }
    else if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)) {
        selectAll();
        return true;
    }
    else if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0)) {
        selectNone();
        return true;
    }
    
    return false;
}

void PianoRollEditor::timerCallback() {
    // Update playhead si está reproduciendo
    repaint();
}

void PianoRollEditor::scrollBarMoved(juce::ScrollBar* scrollBar, double newRangeStart) {
    repaint();
}

void PianoRollEditor::rebuildNoteComponents() {
    noteComponents_.clear();
    
    if (!clip_) return;
    
    const auto& notes = clip_->getNotes();
    
    for (size_t i = 0; i < notes.size(); ++i) {
        auto nc = std::make_unique<NoteComponent>(notes[i], (int)i);
        
        nc->onNoteMoved = [this](int index, juce::Point<int> delta) {
            // Update note position
        };
        
        nc->onNoteResized = [this](int index, double newLength) {
            if (clip_ && index < (int)clip_->getNotes().size()) {
                auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
                notes[index].duration = newLength;
                rebuildNoteComponents();
            }
        };
        
        nc->onNoteSelected = [this](int index) {
            selectedNoteIndices_.clear();
            selectedNoteIndices_.push_back(index);
            for (auto& comp : noteComponents_) {
                comp->setSelected(false);
            }
            if (index < (int)noteComponents_.size()) {
                noteComponents_[index]->setSelected(true);
            }
        };
        
        auto rect = getNoteRectangle(notes[i]);
        nc->setBounds(rect);
        
        addAndMakeVisible(nc.get());
        noteComponents_.push_back(std::move(nc));
    }
    
    // Update velocity lane
    if (velocityLane_) {
        velocityLane_->setNotes(notes);
        velocityLane_->setSelectedNotes(selectedNoteIndices_);
    }
}

void PianoRollEditor::updateScaleHighlighting() {
    notesInScale_.resize(12, false);
    
    // Calcular qué notas están en la escala
    // Implementación básica para Major/Minor
    if (currentScale_ == Sequencer::Scale::Major) {
        notesInScale_ = {true, false, true, false, true, true, false, true, false, true, false, true};
    } else if (currentScale_ == Sequencer::Scale::Minor) {
        notesInScale_ = {true, false, true, true, false, true, false, true, true, false, true, false};
    }
}

void PianoRollEditor::updatePianoKeys() {
    for (int i = 0; i < NUM_KEYS; ++i) {
        bool inScale = !scaleVisible_ || notesInScale_[(i - scaleRootNote_) % 12];
        pianoKeys_[i]->setInScale(inScale);
    }
}

juce::Rectangle<int> PianoRollEditor::getNoteRectangle(const Sequencer::MIDINote& note) const {
    int x = getXForBeats(note.startTime);
    int y = getYForNote(note.noteNumber);
    int width = (int)(note.duration * pixelsPerBeat_);
    int height = (int)noteHeight_;
    
    return juce::Rectangle<int>(x, y, width, height);
}

int PianoRollEditor::getNoteNumberAtY(int y) const {
    int keyAreaHeight = getHeight() - VELOCITY_LANE_HEIGHT - 20;
    int noteNum = (int)((keyAreaHeight - y) / noteHeight_);
    return std::clamp(noteNum, 0, 127);
}

double PianoRollEditor::getBeatsAtX(int x) const {
    return (x - PIANO_KEY_WIDTH) / pixelsPerBeat_;
}

int PianoRollEditor::getXForBeats(double beats) const {
    return PIANO_KEY_WIDTH + (int)(beats * pixelsPerBeat_);
}

int PianoRollEditor::getYForNote(int noteNumber) const {
    int keyAreaHeight = getHeight() - VELOCITY_LANE_HEIGHT - 20;
    return keyAreaHeight - (int)((noteNumber + 1) * noteHeight_);
}

double PianoRollEditor::snapToGrid(double beats) const {
    if (!snapEnabled_) return beats;
    return std::round(beats / gridSize_) * gridSize_;
}

void PianoRollEditor::createNote(int noteNumber, double startBeat) {
    if (!clip_) return;
    
    Sequencer::MIDINote newNote;
    newNote.noteNumber = noteNumber;
    newNote.startTime = startBeat;
    newNote.duration = gridSize_; // Default to grid size
    newNote.velocity = 0.8f;
    
    auto& notes = const_cast<std::vector<Sequencer::MIDINote>&>(clip_->getNotes());
    notes.push_back(newNote);
    
    rebuildNoteComponents();
}

void PianoRollEditor::showContextMenu() {
    juce::PopupMenu menu;
    
    menu.addItem(1, "Cut", !selectedNoteIndices_.empty());
    menu.addItem(2, "Copy", !selectedNoteIndices_.empty());
    menu.addItem(3, "Paste");
    menu.addSeparator();
    menu.addItem(4, "Delete", !selectedNoteIndices_.empty());
    menu.addItem(5, "Select All");
    menu.addSeparator();
    menu.addItem(6, "Quantize", !selectedNoteIndices_.empty());
    menu.addItem(7, "Humanize", !selectedNoteIndices_.empty());
    menu.addSeparator();
    
    juce::PopupMenu toolsMenu;
    toolsMenu.addItem(10, "Chordize", !selectedNoteIndices_.empty());
    toolsMenu.addItem(11, "Arpeggiate", !selectedNoteIndices_.empty());
    toolsMenu.addItem(12, "Strum", !selectedNoteIndices_.empty());
    menu.addSubMenu("Tools", toolsMenu);
    
    // Show menu async (JUCE 8 style)
    menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
        switch (result) {
            case 4: deleteSelected(); break;
            case 5: selectAll(); break;
            case 6: quantizeSelected(gridSize_); break;
            case 7: humanizeSelected(0.5f, 0.3f); break;
        }
    });
}

void PianoRollEditor::paintGrid(juce::Graphics& g, juce::Rectangle<int> area) {
    g.setColour(juce::Colour(0xff2a2a2a));
    
    // Vertical lines (beat grid)
    double beat = 0.0;
    while (true) {
        int x = getXForBeats(beat);
        if (x > area.getRight()) break;
        
        if (x >= area.getX()) {
            bool isMajor = (std::fmod(beat, 1.0) < 0.001);
            g.setColour(isMajor ? juce::Colour(0xff3a3a3a) : juce::Colour(0xff2a2a2a));
            g.drawVerticalLine(x, (float)area.getY(), (float)area.getBottom());
        }
        
        beat += gridSize_;
    }
    
    // Horizontal lines (notes)
    for (int i = 0; i < 128; ++i) {
        int y = getYForNote(i);
        if (y < area.getY() || y > area.getBottom()) continue;
        
        bool isC = (i % 12 == 0);
        g.setColour(isC ? juce::Colour(0xff3a3a3a) : juce::Colour(0xff252525));
        g.drawHorizontalLine(y, (float)area.getX(), (float)area.getRight());
    }
}

void PianoRollEditor::paintNotes(juce::Graphics& g) {
    // Notes are painted by NoteComponents
}

void PianoRollEditor::paintPlayhead(juce::Graphics& g) {
    int x = getXForBeats(playheadPosition_);
    
    g.setColour(juce::Colour(0xffff8c42)); // FL Orange
    g.drawVerticalLine(x, 0.0f, (float)(getHeight() - VELOCITY_LANE_HEIGHT - 20));
}

void PianoRollEditor::paintScaleHighlighting(juce::Graphics& g) {
    for (int i = 0; i < 128; ++i) {
        bool inScale = notesInScale_[(i - scaleRootNote_) % 12];
        if (inScale) {
            int y = getYForNote(i);
            g.setColour(juce::Colour(0xff00ff00).withAlpha(0.1f));
            g.fillRect(PIANO_KEY_WIDTH, y, getWidth() - PIANO_KEY_WIDTH - 20, (int)noteHeight_);
        }
    }
}

//==============================================================================
// PianoRollWindow
//==============================================================================

PianoRollWindow::PianoRollWindow(const juce::String& name)
    : DocumentWindow(name, juce::Colour(0xff2a2a2a), DocumentWindow::allButtons) {
    
    editor_ = new PianoRollEditor();
    setContentOwned(editor_, true);
    
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(1200, 700);
    
    setVisible(true);
}

PianoRollWindow::~PianoRollWindow() {
}

void PianoRollWindow::closeButtonPressed() {
    setVisible(false);
}

} // namespace GUI
} // namespace OmegaStudio
