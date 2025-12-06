/*
  ==============================================================================
    PlaylistEditor.cpp
  ==============================================================================
*/

#include "PlaylistEditor.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
ClipComponent::ClipComponent(ClipType type, int trackIndex, double startTime, double duration)
    : type_(type), trackIndex_(trackIndex), startTime_(startTime), duration_(duration),
      colour_(juce::Colours::cyan) {
}

void ClipComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    
    g.setColour(colour_.withAlpha(selected_ ? 1.0f : 0.8f));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    if (selected_) {
        g.setColour(juce::Colour(0xffff8c42));
        g.drawRoundedRectangle(bounds, 4.0f, 2.0f);
    }
    
    // Name
    g.setColour(juce::Colours::white);
    g.setFont(11.0f);
    g.drawText(name_, bounds.reduced(5), juce::Justification::topLeft);
}

void ClipComponent::mouseDown(const juce::MouseEvent& e) {
    if (e.x >= getWidth() - 10) resizing_ = true;
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e) {
    if (resizing_ && onResized) {
        onResized(0, duration_ + e.getDistanceFromDragStartX() / 100.0);
    } else if (onMoved) {
        onMoved(0, e.getOffsetFromDragStart());
    }
}

//==============================================================================
PlaylistTrackComponent::PlaylistTrackComponent(int trackIndex)
    : trackIndex_(trackIndex), trackName_("Track " + juce::String(trackIndex + 1)),
      trackColour_(juce::Colour::fromHSV(trackIndex / 32.0f, 0.7f, 0.8f, 1.0f)) {
}

void PlaylistTrackComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    
    // Header
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(bounds.removeFromLeft(150));
    
    g.setColour(trackColour_);
    g.fillRect(0, 0, 4, getHeight());
    
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    g.drawText(trackName_, 10, 0, 140, getHeight(), juce::Justification::centredLeft);
}

void PlaylistTrackComponent::resized() {
    int x = 150;
    for (auto& clip : clips_) {
        // Position clips
    }
}

void PlaylistTrackComponent::setTrackName(const juce::String& name) {
    trackName_ = name;
    repaint();
}

void PlaylistTrackComponent::setTrackColour(juce::Colour colour) {
    trackColour_ = colour;
    repaint();
}

void PlaylistTrackComponent::addClip(std::unique_ptr<ClipComponent> clip) {
    addAndMakeVisible(clip.get());
    clips_.push_back(std::move(clip));
}

void PlaylistTrackComponent::clearClips() {
    clips_.clear();
}

//==============================================================================
PlaylistEditor::PlaylistEditor() {
    contentComponent_ = std::make_unique<juce::Component>();
    
    viewport_ = std::make_unique<juce::Viewport>();
    viewport_->setViewedComponent(contentComponent_.get(), false);
    addAndMakeVisible(viewport_.get());
    
    horizontalScrollBar_ = std::make_unique<juce::ScrollBar>(false);
    addAndMakeVisible(horizontalScrollBar_.get());
    
    setNumTracks(32);
    startTimer(30);
}

PlaylistEditor::~PlaylistEditor() {
    stopTimer();
}

void PlaylistEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1e1e1e));
    paintGrid(g);
    paintPlayhead(g);
}

void PlaylistEditor::resized() {
    auto bounds = getLocalBounds();
    
    horizontalScrollBar_->setBounds(bounds.removeFromBottom(20));
    viewport_->setBounds(bounds);
    
    int totalHeight = (int)tracks_.size() * TRACK_HEIGHT;
    contentComponent_->setSize(getWidth(), totalHeight);
    
    int y = 0;
    for (auto& track : tracks_) {
        track->setBounds(0, y, getWidth(), TRACK_HEIGHT);
        y += TRACK_HEIGHT;
    }
}

void PlaylistEditor::timerCallback() {
    repaint();
}

void PlaylistEditor::setNumTracks(int numTracks) {
    tracks_.clear();
    
    for (int i = 0; i < numTracks; ++i) {
        auto track = std::make_unique<PlaylistTrackComponent>(i);
        contentComponent_->addAndMakeVisible(track.get());
        tracks_.push_back(std::move(track));
    }
    
    resized();
}

void PlaylistEditor::setPlayheadPosition(double beats) {
    playheadPosition_ = beats;
}

void PlaylistEditor::setPixelsPerBeat(float ppb) {
    pixelsPerBeat_ = std::clamp(ppb, 20.0f, 500.0f);
    resized();
}

void PlaylistEditor::setGridSize(double beats) {
    gridSize_ = beats;
}

void PlaylistEditor::paintGrid(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff2a2a2a));
    
    // Vertical lines
    for (int beat = 0; beat < 1000; ++beat) {
        int x = TRACK_HEADER_WIDTH + (int)(beat * pixelsPerBeat_);
        bool isBar = (beat % 4 == 0);
        g.setColour(isBar ? juce::Colour(0xff3a3a3a) : juce::Colour(0xff252525));
        g.drawVerticalLine(x, 0.0f, (float)getHeight());
    }
}

void PlaylistEditor::paintPlayhead(juce::Graphics& g) {
    int x = TRACK_HEADER_WIDTH + (int)(playheadPosition_ * pixelsPerBeat_);
    g.setColour(juce::Colour(0xffff8c42));
    g.drawVerticalLine(x, 0.0f, (float)getHeight());
}

//==============================================================================
PlaylistWindow::PlaylistWindow(const juce::String& name)
    : DocumentWindow(name, juce::Colour(0xff2a2a2a), DocumentWindow::allButtons) {
    editor_ = new PlaylistEditor();
    setContentOwned(editor_, true);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(1400, 800);
    setVisible(true);
}

PlaylistWindow::~PlaylistWindow() {}

void PlaylistWindow::closeButtonPressed() {
    setVisible(false);
}

} // namespace GUI
} // namespace OmegaStudio
