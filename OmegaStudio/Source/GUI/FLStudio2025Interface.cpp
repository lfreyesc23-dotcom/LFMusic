//==============================================================================
// FLStudio2025Interface.cpp
// IMPLEMENTACIÓN COMPLETA - INTERFAZ EXACTA FL STUDIO 2025
//==============================================================================

#include "FLStudio2025Interface.h"
#include "../Audio/Engine/AudioEngine.h"

namespace Omega {
namespace GUI {

//==============================================================================
// FLStudio2025Toolbar Implementation
//==============================================================================
FLStudio2025Toolbar::FLStudio2025Toolbar() {
    setInterceptsMouseClicks(true, true);

    // Logo FL STUDIO (grande y naranja)
    logoLabel_ = std::make_unique<juce::Label>();
    logoLabel_->setText("FL STUDIO 2025", juce::dontSendNotification);
    logoLabel_->setFont(juce::Font(15.0f, juce::Font::bold));
    logoLabel_->setColour(juce::Label::textColourId, FLColors::Orange);
    addAndMakeVisible(*logoLabel_);
    
    // Menu Bar
    fileButton_ = std::make_unique<juce::TextButton>("FILE");
    optionsButton_ = std::make_unique<juce::TextButton>("OPTIONS");
    toolsButton_ = std::make_unique<juce::TextButton>("TOOLS");
    helpButton_ = std::make_unique<juce::TextButton>("HELP");
    
    addAndMakeVisible(*fileButton_);
    addAndMakeVisible(*optionsButton_);
    addAndMakeVisible(*toolsButton_);
    addAndMakeVisible(*helpButton_);
    
    // Transport Controls
    playButton_ = std::make_unique<juce::TextButton>("▶");
    playButton_->setTooltip("Play/Pause (Space)");
    playButton_->setClickingTogglesState(true);
    playButton_->onClick = [this]() {
        const bool isNowPlaying = playButton_->getToggleState();
        setPlaying(isNowPlaying); // update UI immediately
        if (onPlay) onPlay(isNowPlaying);
        juce::Logger::writeToLog("UI: Play clicked -> " + juce::String(isNowPlaying ? "true" : "false"));
    };
    addAndMakeVisible(*playButton_);
    
    stopButton_ = std::make_unique<juce::TextButton>("■");
    stopButton_->setTooltip("Stop");
    stopButton_->onClick = [this]() {
        if (onStop) onStop();
        setPlaying(false);
        juce::Logger::writeToLog("UI: Stop clicked");
    };
    addAndMakeVisible(*stopButton_);
    
    recordButton_ = std::make_unique<juce::TextButton>("●");
    recordButton_->setTooltip("Record (Ctrl+R)");
    recordButton_->setColour(juce::TextButton::buttonColourId, FLColors::Danger);
    recordButton_->setClickingTogglesState(true);
    recordButton_->onClick = [this]() {
        const bool nowRecording = recordButton_->getToggleState();
        setRecording(nowRecording); // update UI immediately
        if (onRecord) onRecord(nowRecording);
        juce::Logger::writeToLog("UI: Record clicked -> " + juce::String(nowRecording ? "true" : "false"));
    };
    addAndMakeVisible(*recordButton_);
    
    patternButton_ = std::make_unique<juce::TextButton>("PAT");
    patternButton_->setTooltip("Pattern Mode");
    patternButton_->setClickingTogglesState(true);
    patternButton_->setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(*patternButton_);
    
    songButton_ = std::make_unique<juce::TextButton>("SONG");
    songButton_->setTooltip("Song Mode");
    songButton_->setClickingTogglesState(true);
    addAndMakeVisible(*songButton_);
    
    // Tempo Display
    tempoLabel_ = std::make_unique<juce::Label>();
    tempoLabel_->setText("108.000", juce::dontSendNotification);
    tempoLabel_->setFont(juce::Font(18.0f, juce::Font::bold));
    tempoLabel_->setJustificationType(juce::Justification::centred);
    tempoLabel_->setColour(juce::Label::backgroundColourId, FLColors::DarkBg);
    tempoLabel_->setColour(juce::Label::textColourId, FLColors::TextPrimary);
    addAndMakeVisible(*tempoLabel_);
    
    // Time Display
    timeLabel_ = std::make_unique<juce::Label>();
    timeLabel_->setText("0:00:00", juce::dontSendNotification);
    timeLabel_->setFont(juce::Font(14.0f, juce::Font::plain));
    timeLabel_->setJustificationType(juce::Justification::centred);
    timeLabel_->setColour(juce::Label::textColourId, FLColors::TextSecondary);
    addAndMakeVisible(*timeLabel_);
    
    // CPU/Memory Meters
    cpuLabel_ = std::make_unique<juce::Label>();
    cpuLabel_->setText("CPU: 13%", juce::dontSendNotification);
    cpuLabel_->setFont(juce::Font(11.0f, juce::Font::plain));
    cpuLabel_->setColour(juce::Label::textColourId, FLColors::TextPrimary);
    addAndMakeVisible(*cpuLabel_);
    
    memoryLabel_ = std::make_unique<juce::Label>();
    memoryLabel_->setText("3858 MB / 0", juce::dontSendNotification);
    memoryLabel_->setFont(juce::Font(11.0f, juce::Font::plain));
    memoryLabel_->setColour(juce::Label::textColourId, FLColors::TextPrimary);
    addAndMakeVisible(*memoryLabel_);
    
    // Tool Buttons
    cutButton_ = std::make_unique<juce::TextButton>("✂");
    copyButton_ = std::make_unique<juce::TextButton>("📋");
    pasteButton_ = std::make_unique<juce::TextButton>("📄");
    deleteButton_ = std::make_unique<juce::TextButton>("🗑");
    undoButton_ = std::make_unique<juce::TextButton>("↶");
    redoButton_ = std::make_unique<juce::TextButton>("↷");
    
    addAndMakeVisible(*cutButton_);
    addAndMakeVisible(*copyButton_);
    addAndMakeVisible(*pasteButton_);
    addAndMakeVisible(*deleteButton_);
    addAndMakeVisible(*undoButton_);
    addAndMakeVisible(*redoButton_);
    
    // Mode indicators
    metronomeButton_ = std::make_unique<juce::TextButton>("⏱");
    metronomeButton_->setTooltip("Metronome");
    metronomeButton_->setClickingTogglesState(true);
    addAndMakeVisible(*metronomeButton_);
    
    blendButton_ = std::make_unique<juce::TextButton>("BLEND");
    blendButton_->setTooltip("Blend Mode");
    blendButton_->setClickingTogglesState(true);
    addAndMakeVisible(*blendButton_);
    
    typingKeyboardButton_ = std::make_unique<juce::TextButton>("⌨");
    typingKeyboardButton_->setTooltip("Typing Keyboard");
    typingKeyboardButton_->setClickingTogglesState(true);
    addAndMakeVisible(*typingKeyboardButton_);
    
    // Start timer for updates
    startTimerHz(30);
}

void FLStudio2025Toolbar::paint(juce::Graphics& g) {
    // Background con gradient profesional
    juce::ColourGradient bgGradient(
        juce::Colour(0xFF2D2D2D), 0, 0,
        juce::Colour(0xFF282828), 0, getHeight(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Top accent stripe (FL Studio signature)
    g.setColour(FLColors::Orange.withAlpha(0.4f));
    g.fillRect(0, 0, getWidth(), 2);
    
    // Bottom shadow
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1, 2.0f);
    
    // Subtle separators (muy delgados)
    g.setColour(juce::Colour(0xFF1E1E1E));
    g.drawLine(250, 12, 250, getHeight() - 12, 1.0f); // After menu
    g.drawLine(580, 12, 580, getHeight() - 12, 1.0f); // After transport
    g.drawLine(getWidth() - 220, 12, getWidth() - 220, getHeight() - 12, 1.0f); // Before meters
}

void FLStudio2025Toolbar::resized() {
    auto bounds = getLocalBounds().reduced(5);
    const int buttonHeight = 30;
    const int spacing = 5;
    const int smallButtonWidth = 35;
    const int mediumButtonWidth = 50;
    
    // Logo (left)
    logoLabel_->setBounds(bounds.removeFromLeft(180).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing * 2);
    
    // Menu Bar
    fileButton_->setBounds(bounds.removeFromLeft(60).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    optionsButton_->setBounds(bounds.removeFromLeft(75).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    toolsButton_->setBounds(bounds.removeFromLeft(60).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    helpButton_->setBounds(bounds.removeFromLeft(55).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing * 4);
    
    // Transport Controls
    playButton_->setBounds(bounds.removeFromLeft(mediumButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    stopButton_->setBounds(bounds.removeFromLeft(mediumButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    recordButton_->setBounds(bounds.removeFromLeft(mediumButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing * 2);
    
    // Pattern/Song buttons
    patternButton_->setBounds(bounds.removeFromLeft(50).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    songButton_->setBounds(bounds.removeFromLeft(60).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing * 3);
    
    // Tempo Display (center-ish)
    tempoLabel_->setBounds(bounds.removeFromLeft(100).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing * 2);
    
    // Time Display
    timeLabel_->setBounds(bounds.removeFromLeft(80).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing * 2);
    
    // Tool buttons
    undoButton_->setBounds(bounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    redoButton_->setBounds(bounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    cutButton_->setBounds(bounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    copyButton_->setBounds(bounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    pasteButton_->setBounds(bounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    bounds.removeFromLeft(spacing);
    deleteButton_->setBounds(bounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    
    // Mode indicators and CPU/Memory (right side)
    auto rightBounds = bounds.removeFromRight(180);
    cpuLabel_->setBounds(rightBounds.removeFromTop(20).withX(rightBounds.getX()).withWidth(80));
    memoryLabel_->setBounds(rightBounds.withX(rightBounds.getX()).withWidth(100).withHeight(20));
    
    // Mode buttons (bottom right)
    auto modeBounds = bounds.removeFromRight(150);
    metronomeButton_->setBounds(modeBounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    modeBounds.removeFromLeft(spacing);
    blendButton_->setBounds(modeBounds.removeFromLeft(60).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
    modeBounds.removeFromLeft(spacing);
    typingKeyboardButton_->setBounds(modeBounds.removeFromLeft(smallButtonWidth).withHeight(buttonHeight).withY(bounds.getCentreY() - buttonHeight / 2));
}

void FLStudio2025Toolbar::timerCallback() {
    // Update CPU/Memory (simulated)
    cpuUsage_ = 10.0f + (rand() % 10);
    memoryUsage_ = 3800 + (rand() % 100);
    
    cpuLabel_->setText(juce::String::formatted("CPU: %.0f%%", cpuUsage_), juce::dontSendNotification);
    memoryLabel_->setText(juce::String::formatted("%d MB / 0", memoryUsage_), juce::dontSendNotification);
}

void FLStudio2025Toolbar::setPlaying(bool isPlaying) {
    playButton_->setToggleState(isPlaying, juce::sendNotificationSync);
    playButton_->setButtonText(isPlaying ? "❚❚" : "▶");
}

void FLStudio2025Toolbar::setRecording(bool isRecording) {
    recordButton_->setToggleState(isRecording, juce::sendNotificationSync);
    recordButton_->setButtonText(isRecording ? "●" : "●");
    recordButton_->setColour(juce::TextButton::buttonColourId,
                             isRecording ? FLColors::Danger.brighter(0.2f) : FLColors::Danger);
}

//==============================================================================
// FLStudio2025PatternPanel Implementation
//==============================================================================
FLStudio2025PatternPanel::FLStudio2025PatternPanel() {
    // Create viewport for scrolling
    viewport_ = std::make_unique<juce::Viewport>();
    contentComponent_ = std::make_unique<juce::Component>();
    viewport_->setViewedComponent(contentComponent_.get(), false);
    viewport_->setScrollBarsShown(true, false);
    addAndMakeVisible(*viewport_);
    
    // Add default patterns (EXACTOS a la imagen)
    addPattern("🎹 DEDZZIT TERROR", FLColors::Blue);
    addPattern("🎵 Pattern 2", FLColors::Blue);
    addPattern("🎹 DEDZZIT TERROR", FLColors::Blue);
    addPattern("🥁 DEDZZIT TERROR #2", FLColors::Blue);
    addPattern("🎵 Pattern 5", FLColors::Blue);
    addPattern("🎵 Pattern 6", FLColors::Blue);
    addPattern("🎹 DEDZZIT TERROR", FLColors::Blue);
    addPattern("🎵 Sub #2", FLColors::Blue);
    addPattern("🥁 Hat", juce::Colour(0xFFD4AF37)); // Yellow/Gold
    addPattern("🎵 Pattern 11", FLColors::Blue);
    addPattern("🎵 Pattern 12", FLColors::Blue);
    addPattern("🎵 Pattern 13", FLColors::Blue);
    addPattern("🌿 Croak", FLColors::Green);
    addPattern("🌿 DEDZZIT CROAK", FLColors::Green);
    
    selectPattern(8); // Hat selected by default (like image)
}

void FLStudio2025PatternPanel::paint(juce::Graphics& g) {
    // Background con gradient sutil
    juce::ColourGradient bgGradient(
        juce::Colour(0xFF2A2A2A), 0, 0,
        juce::Colour(0xFF2E2E2E), 0, getHeight(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Right border
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.drawLine(getWidth() - 1, 0, getWidth() - 1, getHeight(), 1.5f);
    
    // Header profesional
    g.setColour(juce::Colour(0xFF242424));
    g.fillRect(0, 0, getWidth(), 35);
    
    g.setColour(FLColors::Orange);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("📋 PATTERNS", 12, 0, getWidth() - 24, 35, juce::Justification::centredLeft);
    
    // Draw patterns list (profesional)
    int y = 40;
    for (int i = 0; i < patterns_.size(); i++) {
        const auto& pattern = patterns_[i];
        juce::Rectangle<int> patternBounds(2, y, getWidth() - 4, patternHeight_);
        
        // Background hover effect
        if (i == hoveredPatternIndex_) {
            g.setColour(juce::Colour(0xFF353535));
            g.fillRoundedRectangle(patternBounds.toFloat(), 3.0f);
        }
        
        // Selected background
        if (pattern.isSelected) {
            g.setColour(pattern.colour.withAlpha(0.25f));
            g.fillRoundedRectangle(patternBounds.toFloat(), 3.0f);
            
            // Selection border
            g.setColour(pattern.colour);
            g.drawRoundedRectangle(patternBounds.toFloat(), 3.0f, 2.0f);
        }
        
        // Color indicator (circle, más moderno)
        g.setColour(pattern.colour);
        g.fillEllipse(12, y + 6, 12, 12);
        
        // Pattern icon
        g.setColour(FLColors::TextPrimary);
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.drawText(pattern.name.substring(0, 2), 30, y, 20, patternHeight_, 
                  juce::Justification::centredLeft);
        
        // Pattern name (truncate if needed)
        g.setColour(pattern.isSelected ? FLColors::TextPrimary : juce::Colour(0xFFB0B0B0));
        g.setFont(juce::Font(11.5f, pattern.isSelected ? juce::Font::bold : juce::Font::plain));
        juce::String displayName = pattern.name.substring(pattern.name.indexOfChar(' ') + 1);
        g.drawText(displayName, 50, y, getWidth() - 60, patternHeight_, 
                  juce::Justification::centredLeft, true);
        
        y += patternHeight_;
    }
}

void FLStudio2025PatternPanel::resized() {
    viewport_->setBounds(getLocalBounds().withTrimmedTop(30));
    contentComponent_->setBounds(0, 0, getWidth(), patterns_.size() * patternHeight_);
}

void FLStudio2025PatternPanel::mouseDown(const juce::MouseEvent& event) {
    int index = (event.y - 30) / patternHeight_;
    if (index >= 0 && index < patterns_.size()) {
        selectPattern(index);
        if (onPatternSelected) onPatternSelected(index);
    }
}

void FLStudio2025PatternPanel::mouseDrag(const juce::MouseEvent& event) {
    // Handle pattern reordering
    hoveredPatternIndex_ = (event.y - 30) / patternHeight_;
    repaint();
}

void FLStudio2025PatternPanel::addPattern(const juce::String& name, juce::Colour colour) {
    Pattern pattern;
    pattern.name = name;
    pattern.colour = colour;
    patterns_.add(pattern);
    resized();
    repaint();
}

void FLStudio2025PatternPanel::selectPattern(int index) {
    if (index >= 0 && index < patterns_.size()) {
        for (auto& p : patterns_) p.isSelected = false;
        patterns_.getReference(index).isSelected = true;
        selectedPatternIndex_ = index;
        repaint();
    }
}

//==============================================================================
// FLStudio2025PlaylistView Implementation
//==============================================================================
FLStudio2025PlaylistView::FLStudio2025PlaylistView() {
    // Add example clips (EXACTOS a la imagen)
    Clip clip1;
    clip1.name = "Sede_flat 02 ø";
    clip1.colour = juce::Colour(0xFF8B6F8B);
    clip1.trackIndex = 1;
    clip1.startBeat = 0;
    clip1.lengthBeats = 8;
    addClip(clip1);
    
    Clip clip2;
    clip2.name = "Mar_se Hall ø";
    clip2.colour = juce::Colour(0xFF8B6F8B);
    clip2.trackIndex = 2;
    clip2.startBeat = 0;
    clip2.lengthBeats = 4;
    addClip(clip2);
    
    Clip clip3;
    clip3.name = "► Hook ▐ ▐en OHat 02";
    clip3.colour = juce::Colour(0xFFE67E22);
    clip3.trackIndex = 3;
    clip3.startBeat = 4;
    clip3.lengthBeats = 4;
    addClip(clip3);
    
    // Add more clips for realistic look
    for (int i = 0; i < 15; i++) {
        Clip c;
        c.name = juce::String::formatted("Sample_%d", i);
        c.colour = FLColors::getPatternColor(i);
        c.trackIndex = 4 + (i % 10);
        c.startBeat = (i * 4) % 64;
        c.lengthBeats = 2 + (i % 6);
        addClip(c);
    }
}

void FLStudio2025PlaylistView::paint(juce::Graphics& g) {
    // Background PROFESIONAL
    g.fillAll(juce::Colour(0xFF404040));
    
    auto bounds = getLocalBounds();
    auto rulerBounds = bounds.removeFromTop(35);
    auto headerBounds = bounds.removeFromLeft(150);
    
    // Draw components
    drawTimeRuler(g, rulerBounds);
    drawTrackHeaders(g, headerBounds);
    drawGrid(g, bounds);
    drawClips(g);
}

void FLStudio2025PlaylistView::resized() {
    // Update clip bounds based on current view
    for (auto& clip : clips_) {
        int x = 150 + static_cast<int>((clip.startBeat * pixelsPerBeat_) - horizontalOffset_);
        int y = 35 + (clip.trackIndex * trackHeight_) - static_cast<int>(verticalOffset_);
        int width = static_cast<int>(clip.lengthBeats * pixelsPerBeat_);
        int height = trackHeight_ - 4;
        
        clip.bounds = juce::Rectangle<int>(x, y, width, height);
    }
}

void FLStudio2025PlaylistView::mouseDown(const juce::MouseEvent& event) {
    // Handle clip selection
    for (int i = 0; i < clips_.size(); i++) {
        if (clips_[i].bounds.contains(event.getPosition())) {
            selectedClipIndex_ = i;
            if (onClipSelected) onClipSelected(clips_[i]);
            repaint();
            return;
        }
    }
    selectedClipIndex_ = -1;
    repaint();
}

void FLStudio2025PlaylistView::mouseDrag(const juce::MouseEvent& event) {
    // Handle clip dragging
    if (selectedClipIndex_ >= 0) {
        auto& clip = clips_.getReference(selectedClipIndex_);
        
        double newStartBeat = (event.x - 150 + horizontalOffset_) / pixelsPerBeat_;
        int newTrackIndex = (event.y - 30 + verticalOffset_) / trackHeight_;
        
        // Snap to grid
        newStartBeat = std::round(newStartBeat / 0.25) * 0.25;
        newTrackIndex = juce::jlimit(0, tracksCount_ - 1, newTrackIndex);
        
        if (onClipMoved) onClipMoved(clip, newStartBeat, newTrackIndex);
        
        clip.startBeat = newStartBeat;
        clip.trackIndex = newTrackIndex;
        
        resized();
        repaint();
    }
}

void FLStudio2025PlaylistView::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (event.mods.isCommandDown()) {
        // Zoom
        pixelsPerBeat_ *= (1.0 + wheel.deltaY * 0.5);
        pixelsPerBeat_ = juce::jlimit(5.0, 100.0, pixelsPerBeat_);
    } else if (event.mods.isShiftDown()) {
        // Horizontal scroll
        horizontalOffset_ -= wheel.deltaY * 100.0;
        horizontalOffset_ = juce::jmax(0.0, horizontalOffset_);
    } else {
        // Vertical scroll
        verticalOffset_ -= wheel.deltaY * 100.0;
        verticalOffset_ = juce::jmax(0.0, verticalOffset_);
    }
    
    resized();
    repaint();
}

void FLStudio2025PlaylistView::drawTimeRuler(juce::Graphics& g, juce::Rectangle<int> bounds) {
    g.setColour(FLColors::PanelBg);
    g.fillRect(bounds);
    
    g.setColour(FLColors::TextSecondary);
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    
    // Draw bar numbers
    int startBar = static_cast<int>(horizontalOffset_ / (pixelsPerBeat_ * beatsPerBar_));
    int visibleBars = static_cast<int>(bounds.getWidth() / (pixelsPerBeat_ * beatsPerBar_)) + 2;
    
    for (int i = 0; i < visibleBars; i++) {
        int bar = startBar + i;
        int x = 150 + static_cast<int>((bar * beatsPerBar_ * pixelsPerBeat_) - horizontalOffset_);
        
        if (x >= 150 && x < bounds.getRight()) {
            g.drawText(juce::String(bar + 1), x, bounds.getY(), 40, bounds.getHeight(),
                      juce::Justification::centredLeft);
        }
    }
    
    // Border
    g.setColour(FLColors::Border);
    g.drawLine(bounds.getX(), bounds.getBottom() - 1, bounds.getRight(), bounds.getBottom() - 1, 1.0f);
}

void FLStudio2025PlaylistView::drawTrackHeaders(juce::Graphics& g, juce::Rectangle<int> bounds) {
    g.setColour(FLColors::PanelBg);
    g.fillRect(bounds);
    
    g.setColour(FLColors::TextSecondary);
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    
    // Draw track names
    int startTrack = static_cast<int>(verticalOffset_ / trackHeight_);
    int visibleTracks = (bounds.getHeight() / trackHeight_) + 2;
    
    for (int i = 0; i < visibleTracks; i++) {
        int track = startTrack + i;
        if (track >= tracksCount_) break;
        
        int y = bounds.getY() + (track * trackHeight_) - static_cast<int>(verticalOffset_);
        
        juce::Rectangle<int> trackBounds(bounds.getX(), y, bounds.getWidth(), trackHeight_);
        
        // Alternating background
        if (track % 2 == 0) {
            g.setColour(FLColors::DarkBg.withAlpha(0.3f));
            g.fillRect(trackBounds);
        }
        
        g.setColour(FLColors::TextSecondary);
        g.drawText(juce::String::formatted("Track %d", track + 1), 
                  trackBounds.reduced(5, 0), juce::Justification::centredLeft);
        
        // Border
        g.setColour(FLColors::Border);
        g.drawLine(trackBounds.getX(), trackBounds.getBottom() - 1, 
                  trackBounds.getRight(), trackBounds.getBottom() - 1, 1.0f);
    }
    
    // Right border
    g.setColour(FLColors::Border);
    g.drawLine(bounds.getRight() - 1, bounds.getY(), bounds.getRight() - 1, bounds.getBottom(), 1.0f);
}

void FLStudio2025PlaylistView::drawGrid(juce::Graphics& g, juce::Rectangle<int> bounds) {
    // Subtle gradient background
    juce::ColourGradient bgGradient(
        juce::Colour(0xFF3E3E3E), bounds.getX(), bounds.getY(),
        juce::Colour(0xFF424242), bounds.getX(), bounds.getBottom(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillRect(bounds);
    
    // Very subtle vertical grid lines (beats)
    int startBeat = static_cast<int>(horizontalOffset_ / pixelsPerBeat_);
    int visibleBeats = static_cast<int>(bounds.getWidth() / pixelsPerBeat_) + 2;
    
    for (int i = 0; i < visibleBeats; i++) {
        int beat = startBeat + i;
        int x = static_cast<int>((beat * pixelsPerBeat_) - horizontalOffset_);
        
        if (beat % static_cast<int>(beatsPerBar_) == 0) {
            // Bar lines (more visible)
            g.setColour(juce::Colour(0xFF4A4A4A));
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
        } else {
            // Beat lines (barely visible)
            g.setColour(juce::Colour(0xFF383838));
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 0.5f);
        }
    }
    
    // Horizontal track lines (very subtle)
    int startTrack = static_cast<int>(verticalOffset_ / trackHeight_);
    int visibleTracks = (bounds.getHeight() / trackHeight_) + 2;
    
    for (int i = 0; i < visibleTracks; i++) {
        int track = startTrack + i;
        if (track >= tracksCount_) break;
        
        int y = (track * trackHeight_) - static_cast<int>(verticalOffset_);
        
        g.setColour(juce::Colour(0xFF353535));
        g.drawLine(bounds.getX(), y, bounds.getRight(), y, 0.5f);
    }
}

void FLStudio2025PlaylistView::drawClips(juce::Graphics& g) {
    for (int i = 0; i < clips_.size(); i++) {
        const auto& clip = clips_[i];
        
        // Skip if not visible
        if (!clip.bounds.intersects(getLocalBounds())) continue;
        
        auto clipRect = clip.bounds.toFloat().reduced(1.0f);
        
        // Shadow for depth
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(clipRect.translated(0, 2), 4.0f);
        
        // Main clip background (vibrant colors)
        g.setColour(clip.colour);
        g.fillRoundedRectangle(clipRect, 4.0f);
        
        // Highlight gradient for depth
        juce::ColourGradient gradient(
            clip.colour.brighter(0.3f), clipRect.getX(), clipRect.getY(),
            clip.colour.darker(0.1f), clipRect.getX(), clipRect.getBottom(),
            false
        );
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(clipRect, 4.0f);
        
        // Selection highlight
        if (i == selectedClipIndex_) {
            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.fillRoundedRectangle(clipRect, 4.0f);
            
            // Selection border
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(clipRect, 4.0f, 2.0f);
        }
        
        // Subtle border
        g.setColour(clip.colour.brighter(0.5f).withAlpha(0.5f));
        g.drawRoundedRectangle(clipRect, 4.0f, 1.0f);
        
        // Clip name (clear and readable)
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        auto textBounds = clipRect.reduced(6, 4);
        g.drawText(clip.name, textBounds.toNearestInt(), 
                  juce::Justification::centredLeft, true);
        
        // Waveform visualization (professional)
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        for (int x = 0; x < clipRect.getWidth(); x += 2) {
            int h = 3 + (rand() % 8);
            float waveX = clipRect.getX() + x;
            float waveY = clipRect.getCentreY();
            g.drawLine(waveX, waveY - h/2.0f, waveX, waveY + h/2.0f, 1.2f);
        }
    }
}

void FLStudio2025PlaylistView::addClip(const Clip& clip) {
    clips_.add(clip);
    resized();
}

void FLStudio2025PlaylistView::clearClips() {
    clips_.clear();
    repaint();
}

//==============================================================================
// FLStudio2025HelpPanel Implementation
//==============================================================================
FLStudio2025HelpPanel::FLStudio2025HelpPanel() {
    // Title
    titleLabel_ = std::make_unique<juce::Label>();
    titleLabel_->setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel_->setColour(juce::Label::textColourId, FLColors::TextPrimary);
    titleLabel_->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*titleLabel_);
    
    // Content editor
    contentEditor_ = std::make_unique<juce::TextEditor>();
    contentEditor_->setMultiLine(true);
    contentEditor_->setReadOnly(true);
    contentEditor_->setScrollbarsShown(true);
    contentEditor_->setColour(juce::TextEditor::backgroundColourId, FLColors::DarkBg);
    contentEditor_->setColour(juce::TextEditor::textColourId, FLColors::TextPrimary);
    contentEditor_->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    contentEditor_->setFont(juce::Font(13.0f, juce::Font::plain));
    addAndMakeVisible(*contentEditor_);
    
    // Set default help text (EXACTO a la imagen)
    setHelpText("How do I reverse a sample?", {
        "To reverse a sample in FL Studio, you need to use the Channel Settings window for the sample. Try the following:",
        "",
        "1. Load your sample into the Channel Rack (by dragging it from the Browser onto the Channel Rack or Playlist).",
        "",
        "2. Click on the sample's name in the Channel Rack to open its Channel Settings window.",
        "",
        "3. In the Channel Settings window, look for the SMP (Sample) tab.",
        "",
        "4. In this tab, you will find a \"Reverse\" option. Click this button (listen to play) to reverse the sample playback.",
        "",
        "Now, when you play the sample, it will play in reverse.",
        "",
        "If your sample is in the Playlist as an Audio Clip, you can also open its Channel Settings window by clicking on the Audio Clip in the Channel Rack and then use the same \"Reverse\" option."
    });
}

void FLStudio2025HelpPanel::paint(juce::Graphics& g) {
    // Background exacto FL Studio
    g.fillAll(FLColors::DarkBg);
    
    // Left border
    g.setColour(FLColors::Border);
    g.drawLine(0, 0, 0, getHeight(), 1.0f);
    
    // Header background
    g.setColour(FLColors::PanelBg);
    g.fillRect(0, 0, getWidth(), 50);
    
    // Icons in header
    g.setColour(FLColors::TextSecondary);
    g.setFont(juce::Font(18.0f, juce::Font::plain));
    g.drawText("✏", getWidth() - 70, 15, 25, 25, juce::Justification::centred);
    g.drawText("🌙", getWidth() - 40, 15, 25, 25, juce::Justification::centred);
}

void FLStudio2025HelpPanel::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50); // Header space
    
    titleLabel_->setBounds(bounds.removeFromTop(40).reduced(15, 5));
    contentEditor_->setBounds(bounds.reduced(15));
}

void FLStudio2025HelpPanel::setHelpText(const juce::String& title, const juce::StringArray& steps) {
    currentTitle_ = title;
    currentSteps_ = steps;
    
    titleLabel_->setText(title, juce::dontSendNotification);
    
    juce::String fullText;
    for (int i = 0; i < steps.size(); i++) {
        fullText += steps[i] + "\n\n";
    }
    
    contentEditor_->setText(fullText, false);
}

//==============================================================================
// FLStudio2025ChannelRack Implementation
//==============================================================================
FLStudio2025ChannelRack::FLStudio2025ChannelRack() {
    // Add default channels (EXACTOS a la imagen)
    addChannel("FPC 7 Kicks", juce::Colour(0xFFE67E22));  // Orange
    addChannel("Kick 1", juce::Colour(0xFF4A7BA7));       // Blue
    addChannel("Snare 1", juce::Colour(0xFF7FA563));      // Green
    addChannel("HiHat Closed", juce::Colour(0xFFE74C3C)); // Red
    addChannel("Clap", juce::Colour(0xFFD4AF37));         // Gold
    
    // Set some default steps for visual appeal
    if (!channels_.isEmpty()) {
        // Kick pattern (every 4 steps)
        channels_[0].steps[0] = true;
        channels_[0].steps[4] = true;
        channels_[0].steps[8] = true;
        channels_[0].steps[12] = true;
        
        // Snare pattern
        if (channels_.size() > 2) {
            channels_[2].steps[4] = true;
            channels_[2].steps[12] = true;
        }
        
        // HiHat pattern (every 2 steps)
        if (channels_.size() > 3) {
            for (int i = 0; i < 16; i += 2) {
                channels_[3].steps[i] = true;
            }
        }
    }
}

void FLStudio2025ChannelRack::paint(juce::Graphics& g) {
    // Background exacto imagen
    g.fillAll(FLColors::DarkBg);
    
    // Top border
    g.setColour(FLColors::Border);
    g.drawLine(0, 0, getWidth(), 0, 2.0f);
    
    // Header area (más compacto)
    int headerHeight = 22;
    g.setColour(juce::Colour(0xFF323232)); // Fondo header exacto
    g.fillRect(0, 0, getWidth(), headerHeight);
    
    // Selector desplegable izquierda (EXACTO imagen)
    int selectorX = 8;
    int selectorY = 3;
    int selectorWidth = 85;
    int selectorHeight = 16;
    
    // Dropdown background
    g.setColour(juce::Colour(0xFF2B2B2B));
    g.fillRoundedRectangle(selectorX, selectorY, selectorWidth, selectorHeight, 2.0f);
    
    // Border
    g.setColour(FLColors::Border);
    g.drawRoundedRectangle(selectorX, selectorY, selectorWidth, selectorHeight, 2.0f, 0.8f);
    
    // Icon + Text "Percussion"
    g.setColour(FLColors::TextPrimary);
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText("⌨", selectorX + 4, selectorY, 15, selectorHeight, juce::Justification::centred);
    g.drawText("Percussion", selectorX + 18, selectorY, 50, selectorHeight, juce::Justification::centredLeft);
    
    // Dropdown arrow
    g.setColour(FLColors::TextSecondary);
    juce::Path arrow;
    float arrowX = selectorX + selectorWidth - 10;
    float arrowY = selectorY + selectorHeight / 2;
    arrow.addTriangle(arrowX, arrowY - 1.5f, arrowX + 5, arrowY - 1.5f, arrowX + 2.5f, arrowY + 2);
    g.fillPath(arrow);
    
    // Tab "Channel rack" (texto simple, no tab button)
    g.setColour(FLColors::TextSecondary);
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.drawText("Channel rack", selectorX + selectorWidth + 15, 0, 100, headerHeight, juce::Justification::centredLeft);
    
    // Channel list area
    int channelListWidth = 140;  // Más estrecho como imagen
    int stepGridStartX = channelListWidth;
    int yOffset = headerHeight + 5;
    int rowHeight = 20;  // Más compacto
    int stepWidth = 18;   // Más estrecho
    
    // Draw channel names (simplificado como imagen)
    for (int i = 0; i < channels_.size() && i < 5; i++) {  // Solo mostrar primeros 5
        int y = yOffset + (i * rowHeight);
        
        // Channel color indicator (más fino)
        g.setColour(channels_[i].colour);
        g.fillRect(3, y + 2, 2, rowHeight - 4);
        
        // Channel name (más pequeño)
        g.setColour(FLColors::TextPrimary);
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        
        // Nombre simplificado
        juce::String displayName = (i == 0) ? "FPC 7 Kicks" : channels_[i].name;
        g.drawText(displayName, 10, y, 130, rowHeight, juce::Justification::centredLeft);
    }
    
    // Vertical separator (más sutil)
    g.setColour(FLColors::Border);
    g.drawLine(channelListWidth, headerHeight, channelListWidth, getHeight(), 1.0f);
    
    // Step grid ruler (números: 22, 23, 24... EXACTO imagen)
    int rulerHeight = 18;
    g.setColour(juce::Colour(0xFF323232));
    g.fillRect(stepGridStartX, headerHeight, getWidth() - stepGridStartX, rulerHeight);
    
    g.setColour(FLColors::TextSecondary);
    g.setFont(juce::Font(9.0f, juce::Font::plain));
    int startBar = 22;
    for (int i = 0; i < 10; i++) {  // 22-31
        int x = stepGridStartX + (i * stepWidth * 4) + 8;
        g.drawText(juce::String(startBar + i), x, headerHeight, 25, rulerHeight, juce::Justification::centredLeft);
    }
    
    int gridStartY = headerHeight + rulerHeight;
    
    // Draw step grid (EXACTO imagen - cuadrados naranjas)
    for (int ch = 0; ch < channels_.size() && ch < 5; ch++) {
        int y = yOffset + (ch * rowHeight);
        
        for (int step = 0; step < 32 && step < channels_[ch].steps.size(); step++) {
            int x = stepGridStartX + (step * stepWidth);
            
            juce::Rectangle<float> stepRect(x + 1, y + 1, stepWidth - 2, rowHeight - 2);
            
            // Background oscuro
            g.setColour(juce::Colour(0xFF2B2B2B));
            g.fillRect(stepRect);
            
            // Draw step if active (naranja brillante como imagen)
            if (channels_[ch].steps[step]) {
                g.setColour(juce::Colour(0xFFFF6B00));  // Naranja exacto imagen
                g.fillRect(stepRect);
            }
            
            // Grid lines sutiles
            if (step % 4 == 0) {
                g.setColour(FLColors::Border);
                g.drawLine(x, gridStartY, x, gridStartY + (rowHeight * 5), 0.5f);
            }
        }
    }
}

void FLStudio2025ChannelRack::resized() {
    // Layout will be handled by parent
}

void FLStudio2025ChannelRack::mouseDown(const juce::MouseEvent& event) {
    int channelListWidth = 140;  // Actualizado
    int headerHeight = 22;        // Actualizado
    
    if (event.x < channelListWidth) {
        // Clicked in channel list area
        return;
    }
    
    // Calculate which step was clicked
    int yOffset = headerHeight + 23;  // Header + ruler
    int rowHeight = 20;   // Actualizado
    int stepWidth = 18;    // Actualizado
    
    int channelIndex = (event.y - yOffset) / rowHeight;
    int stepIndex = (event.x - channelListWidth) / stepWidth;
    
    if (channelIndex >= 0 && channelIndex < channels_.size() &&
        stepIndex >= 0 && stepIndex < visibleSteps_) {
        toggleStep(channelIndex, stepIndex);
        if (onStepToggled) onStepToggled(channelIndex, stepIndex);
    }
}

void FLStudio2025ChannelRack::addChannel(const juce::String& name, juce::Colour colour) {
    channels_.add(Channel(name, colour));
    repaint();
}

void FLStudio2025ChannelRack::toggleStep(int channelIndex, int stepIndex) {
    if (channelIndex >= 0 && channelIndex < channels_.size() &&
        stepIndex >= 0 && stepIndex < channels_[channelIndex].steps.size()) {
        channels_.getReference(channelIndex).steps[stepIndex] = 
            !channels_[channelIndex].steps[stepIndex];
        repaint();
    }
}

//==============================================================================
// FLStudio2025MainWindow Implementation (UPDATED)
//==============================================================================
FLStudio2025MainWindow::FLStudio2025MainWindow(Audio::AudioEngine* audioEngine)
    : audioEngine_(audioEngine) {
    // Create all components
    toolbar_ = std::make_unique<FLStudio2025Toolbar>();
    addAndMakeVisible(*toolbar_);
    
    patternPanel_ = std::make_unique<FLStudio2025PatternPanel>();
    addAndMakeVisible(*patternPanel_);
    
    playlistView_ = std::make_unique<FLStudio2025PlaylistView>();
    addAndMakeVisible(*playlistView_);
    
    helpPanel_ = std::make_unique<FLStudio2025HelpPanel>();
    addAndMakeVisible(*helpPanel_);
    
    // NEW: Channel Rack
    channelRack_ = std::make_unique<FLStudio2025ChannelRack>();
    addAndMakeVisible(*channelRack_);
    
    // Setup callbacks
    toolbar_->onPlay = [this](bool shouldPlay) {
        if (!audioEngine_) return;
        if (shouldPlay && !audioEngine_->isRunning()) {
            audioEngine_->start();
        } else if (!shouldPlay && audioEngine_->isRunning()) {
            audioEngine_->stop();
        }
        toolbar_->setPlaying(audioEngine_->isRunning());
        juce::Logger::writeToLog("Engine: play state -> " + juce::String(audioEngine_->isRunning() ? "true" : "false"));
    };

    toolbar_->onStop = [this]() {
        if (!audioEngine_) return;
        audioEngine_->stop();
        audioEngine_->reset();
        toolbar_->setPlaying(false);
        juce::Logger::writeToLog("Engine: stop");
    };

    toolbar_->onRecord = [this](bool isRecord) {
        isRecording_ = isRecord;
        if (audioEngine_) {
            if (isRecording_) {
                audioEngine_->armTrack(0);
                audioEngine_->startRecording();
            } else {
                audioEngine_->stopRecording();
            }
            toolbar_->setRecording(audioEngine_->isRecording());
            juce::Logger::writeToLog("Engine: record state -> " + juce::String(audioEngine_->isRecording() ? "true" : "false"));
        } else {
            toolbar_->setRecording(isRecording_);
        }
    };
    
    patternPanel_->onPatternSelected = [this](int index) {
        DBG("Pattern selected: " + juce::String(index));
    };
    
    playlistView_->onClipSelected = [this](const FLStudio2025PlaylistView::Clip& clip) {
        DBG("Clip selected: " + clip.name);
    };
    
    channelRack_->onStepToggled = [this](int channel, int step) {
        DBG("Step toggled: Channel " + juce::String(channel) + ", Step " + juce::String(step));
    };
    
    // Set initial size (Full HD)
    setSize(1920, 1080);
}

void FLStudio2025MainWindow::paint(juce::Graphics& g) {
    // Main background
    g.fillAll(FLColors::DarkBg);
}

void FLStudio2025MainWindow::resized() {
    auto bounds = getLocalBounds();
    
    // Toolbar (top)
    toolbar_->setBounds(bounds.removeFromTop(toolbarHeight_));
    
    // Channel rack en la parte inferior
    channelRack_->setBounds(bounds.removeFromBottom(channelRackHeight_));
    
    // Pattern panel (left)
    patternPanel_->setBounds(bounds.removeFromLeft(patternPanelWidth_));
    
    // Help panel (right)
    helpPanel_->setBounds(bounds.removeFromRight(helpPanelWidth_));
    
    // Playlist view (center)
    playlistView_->setBounds(bounds);
}

} // namespace GUI
} // namespace Omega
