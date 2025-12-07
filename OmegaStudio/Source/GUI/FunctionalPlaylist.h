//==============================================================================
// FunctionalPlaylist.h
// Vista de arreglo/playlist funcional estilo FL Studio
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Clip de audio/MIDI arrastrable
//==============================================================================
class PlaylistClip : public juce::Component {
public:
    PlaylistClip(const juce::String& name, double start, double duration, const juce::Colour& color)
        : clipName(name), startTime(start), clipDuration(duration), clipColor(color) {
        
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    
    void paint(juce::Graphics& g) override {
        // Clip background
        g.setColour(clipColor.withAlpha(0.8f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);
        
        // Border
        g.setColour(clipColor.brighter(0.3f));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 3.0f, 2.0f);
        
        // Clip name
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(clipName, getLocalBounds().reduced(5), 
                  juce::Justification::centredLeft, true);
        
        // Waveform/pattern lines (decorative)
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        for (int i = 0; i < getWidth(); i += 8) {
            float height = juce::Random::getSystemRandom().nextFloat() * getHeight() * 0.6f;
            g.drawLine((float)i, getHeight() * 0.5f - height * 0.5f,
                      (float)i, getHeight() * 0.5f + height * 0.5f, 1.0f);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        dragger.startDraggingComponent(this, e);
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        dragger.dragComponent(this, e, nullptr);
        
        // Update position based on scale
        startTime = getX() / pixelsPerSecond;
    }
    
    double getStartTime() const { return startTime; }
    double getDuration() const { return clipDuration; }
    void setPixelsPerSecond(double pps) { pixelsPerSecond = pps; }
    
private:
    juce::String clipName;
    double startTime;
    double clipDuration;
    juce::Colour clipColor;
    juce::ComponentDragger dragger;
    double pixelsPerSecond = 50.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistClip)
};

//==============================================================================
// Track individual en la playlist
//==============================================================================
class PlaylistTrack : public juce::Component {
public:
    PlaylistTrack(const juce::String& name, int trackNumber)
        : trackName(name), trackNum(trackNumber) {
        
        // Track header button
        headerButton.setButtonText(name);
        headerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
        addAndMakeVisible(headerButton);
        
        // Mute button
        muteButton.setButtonText("M");
        muteButton.setClickingTogglesState(true);
        muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        addAndMakeVisible(muteButton);
        
        // Solo button
        soloButton.setButtonText("S");
        soloButton.setClickingTogglesState(true);
        soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
        addAndMakeVisible(soloButton);
        
        // Record arm button
        recordButton.setButtonText("●");
        recordButton.setClickingTogglesState(true);
        recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        addAndMakeVisible(recordButton);
    }
    
    void paint(juce::Graphics& g) override {
        // Track background
        g.fillAll(trackNum % 2 == 0 ? juce::Colour(0xff2a2a2a) : juce::Colour(0xff252525));
        
        // Grid lines (every second)
        g.setColour(juce::Colour(0xff1a1a1a));
        for (int x = 150; x < getWidth(); x += 50) {
            g.drawVerticalLine(x, 0.0f, (float)getHeight());
        }
        
        // Border
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void resized() override {
        auto area = getLocalBounds();
        auto header = area.removeFromLeft(150);
        
        headerButton.setBounds(header.removeFromLeft(80).reduced(2));
        muteButton.setBounds(header.removeFromLeft(23).reduced(2));
        soloButton.setBounds(header.removeFromLeft(23).reduced(2));
        recordButton.setBounds(header.removeFromLeft(23).reduced(2));
        
        // Layout clips
        for (auto* clip : clips) {
            int x = 150 + (int)(clip->getStartTime() * pixelsPerSecond);
            int width = (int)(clip->getDuration() * pixelsPerSecond);
            clip->setBounds(x, 5, width, getHeight() - 10);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isRightButtonDown()) {
            showAddClipMenu(e.getPosition());
        }
    }
    
    void addClip(const juce::String& name, double start, double duration) {
        auto color = juce::Colour::fromHSV(juce::Random::getSystemRandom().nextFloat(), 0.7f, 0.9f, 1.0f);
        auto* clip = new PlaylistClip(name, start, duration, color);
        clip->setPixelsPerSecond(pixelsPerSecond);
        clips.add(clip);
        addAndMakeVisible(clip);
        resized();
    }
    
    void setPixelsPerSecond(double pps) {
        pixelsPerSecond = pps;
        for (auto* clip : clips) {
            clip->setPixelsPerSecond(pps);
        }
        resized();
    }
    
private:
    void showAddClipMenu(const juce::Point<int>& position) {
        juce::PopupMenu menu;
        menu.addItem(1, "Add Audio Clip");
        menu.addItem(2, "Add MIDI Pattern");
        menu.addItem(3, "Add Automation");
        menu.addSeparator();
        menu.addItem(10, "Delete Track");
        
        menu.showMenuAsync(juce::PopupMenu::Options(), [this, position](int result) {
            if (result == 1 || result == 2) {
                double time = (position.getX() - 150) / pixelsPerSecond;
                addClip(result == 1 ? "Audio" : "Pattern", time, 4.0);
            }
        });
    }
    
    juce::String trackName;
    int trackNum;
    juce::TextButton headerButton;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton recordButton;
    
    juce::OwnedArray<PlaylistClip> clips;
    double pixelsPerSecond = 50.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistTrack)
};

//==============================================================================
// Playlist completa con múltiples tracks
//==============================================================================
class FunctionalPlaylist : public juce::Component {
public:
    FunctionalPlaylist() {
        // Add track button
        addTrackButton.setButtonText("+ Add Track");
        addTrackButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4CAF50));
        addTrackButton.onClick = [this] {
            addTrack("Track " + juce::String(tracks.size() + 1));
        };
        addAndMakeVisible(addTrackButton);
        
        // Zoom slider
        zoomSlider.setRange(10.0, 200.0, 1.0);
        zoomSlider.setValue(50.0);
        zoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        zoomSlider.onValueChange = [this] {
            pixelsPerSecond = zoomSlider.getValue();
            for (auto* track : tracks) {
                track->setPixelsPerSecond(pixelsPerSecond);
            }
        };
        addAndMakeVisible(zoomSlider);
        
        zoomLabel.setText("Zoom:", juce::dontSendNotification);
        addAndMakeVisible(zoomLabel);
        
        // Viewport for scrolling
        viewport.setViewedComponent(&trackContainer, false);
        viewport.setScrollBarsShown(true, true);
        addAndMakeVisible(viewport);
        
        // Add some default tracks
        addTrack("Master");
        addTrack("Drums");
        addTrack("Bass");
        addTrack("Lead");
        addTrack("Pads");
        
        // Add some demo clips
        if (tracks.size() > 1) {
            tracks[1]->addClip("Kick Pattern", 0.0, 4.0);
            tracks[1]->addClip("Snare Pattern", 4.0, 4.0);
            tracks[2]->addClip("Bass Line", 0.0, 8.0);
            tracks[3]->addClip("Lead Melody", 8.0, 8.0);
        }
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        // Top toolbar
        auto toolbar = area.removeFromTop(35);
        addTrackButton.setBounds(toolbar.removeFromLeft(120).reduced(5));
        toolbar.removeFromLeft(10);
        zoomLabel.setBounds(toolbar.removeFromLeft(50).reduced(5));
        zoomSlider.setBounds(toolbar.removeFromLeft(150).reduced(5));
        
        // Viewport
        viewport.setBounds(area);
        
        // Layout tracks in container
        int totalHeight = tracks.size() * 80;
        int totalWidth = juce::jmax(area.getWidth(), (int)(pixelsPerSecond * 60)); // 60 seconds
        
        trackContainer.setBounds(0, 0, totalWidth, totalHeight);
        
        int y = 0;
        for (auto* track : tracks) {
            track->setBounds(0, y, totalWidth, 78);
            y += 80;
        }
    }
    
    void addTrack(const juce::String& name) {
        auto* track = new PlaylistTrack(name, tracks.size());
        track->setPixelsPerSecond(pixelsPerSecond);
        tracks.add(track);
        trackContainer.addAndMakeVisible(track);
        resized();
    }
    
    int getNumTracks() const { return tracks.size(); }
    
private:
    juce::TextButton addTrackButton;
    juce::Slider zoomSlider;
    juce::Label zoomLabel;
    juce::Viewport viewport;
    juce::Component trackContainer;
    juce::OwnedArray<PlaylistTrack> tracks;
    double pixelsPerSecond = 50.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionalPlaylist)
};

} // namespace GUI
} // namespace OmegaStudio
