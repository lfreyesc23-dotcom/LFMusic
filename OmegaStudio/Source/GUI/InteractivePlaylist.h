#pragma once

#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio::GUI {

/**
 * @brief Clip in playlist
 */
struct PlaylistClip
{
    juce::String name;
    int track{0};
    double startTime{0.0};   // In beats
    double duration{4.0};    // In beats
    juce::Colour color;
    bool selected{false};
    juce::Rectangle<float> bounds;
};

/**
 * @brief Track in playlist
 */
struct PlaylistTrack
{
    juce::String name;
    juce::Colour color;
    bool muted{false};
    bool soloed{false};
    int height{60};
};

/**
 * @brief Interactive Playlist - Arreglo completo con clips arrastrables
 */
class InteractivePlaylist : public juce::Component,
                            private juce::Timer
{
public:
    InteractivePlaylist()
    {
        // Create default tracks
        addTrack("Audio 1", juce::Colour(0xffff0000));
        addTrack("Audio 2", juce::Colour(0xff00ff00));
        addTrack("MIDI 1", juce::Colour(0xff0000ff));
        addTrack("MIDI 2", juce::Colour(0xffff00ff));
        addTrack("Master", juce::Colour(0xffffd700));
        
        // Add some example clips
        addClip("Intro", 0, 0.0, 8.0, juce::Colour(0xffff6666));
        addClip("Verse", 0, 8.0, 16.0, juce::Colour(0xff66ff66));
        addClip("Chorus", 0, 24.0, 16.0, juce::Colour(0xff6666ff));
        
        addClip("Bass Line", 1, 0.0, 32.0, juce::Colour(0xffff8c00));
        
        addClip("Lead Melody", 2, 8.0, 8.0, juce::Colour(0xffff00ff));
        addClip("Lead Melody 2", 2, 24.0, 8.0, juce::Colour(0xffff00ff));
        
        setSize(1200, 400);
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
        g.drawText("📝 PLAYLIST", header.reduced(10, 0), 
                  juce::Justification::centredLeft);
        
        // Zoom controls
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawText("Zoom: " + juce::String(zoomLevel_, 1) + "x", 
                  header.removeFromRight(100).reduced(5, 0),
                  juce::Justification::centredRight);
        
        // Track names area
        auto trackNamesArea = bounds.removeFromLeft(150);
        
        // Timeline/ruler area
        auto rulerArea = bounds.removeFromTop(30);
        drawRuler(g, rulerArea);
        
        // Draw track names
        drawTrackNames(g, trackNamesArea);
        
        // Draw grid
        drawGrid(g, bounds);
        
        // Draw clips
        drawClips(g, bounds);
        
        // Draw playhead
        if (isPlaying_)
        {
            drawPlayhead(g, bounds);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        auto pos = e.getPosition();
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40);  // Skip header
        bounds.removeFromLeft(150); // Skip track names
        bounds.removeFromTop(30);  // Skip ruler
        
        // Check if clicking on clip
        for (auto& clip : clips_)
        {
            if (clip.bounds.contains(pos.toFloat()))
            {
                if (!e.mods.isShiftDown())
                {
                    // Deselect all
                    for (auto& c : clips_)
                        c.selected = false;
                }
                
                clip.selected = true;
                draggingClip_ = &clip;
                dragStartPos_ = pos;
                dragStartClipTime_ = clip.startTime;
                dragStartClipTrack_ = clip.track;
                repaint();
                return;
            }
        }
        
        // Click on empty space - create new clip
        if (bounds.contains(pos))
        {
            int track = screenYToTrack(pos.y);
            double time = screenXToTime(pos.x);
            
            if (track >= 0 && track < tracks_.size())
            {
                addClip("New Clip", track, time, 4.0, tracks_[track].color);
                repaint();
            }
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggingClip_)
        {
            auto delta = e.getPosition() - dragStartPos_;
            
            // Update track (vertical)
            int trackDelta = delta.y / 60; // Track height = 60
            int newTrack = juce::jlimit(0, (int)tracks_.size() - 1, 
                                       dragStartClipTrack_ + trackDelta);
            draggingClip_->track = newTrack;
            
            // Update time (horizontal)
            double timeDelta = delta.x / pixelsPerBeat_;
            draggingClip_->startTime = juce::jmax(0.0, 
                                                  dragStartClipTime_ + timeDelta);
            
            // Snap to grid
            draggingClip_->startTime = std::round(draggingClip_->startTime * snapDivision_) 
                                      / snapDivision_;
            
            repaint();
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        draggingClip_ = nullptr;
    }
    
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        auto pos = e.getPosition();
        
        // Delete clip on double click
        for (int i = clips_.size() - 1; i >= 0; --i)
        {
            if (clips_[i].bounds.contains(pos.toFloat()))
            {
                clips_.erase(clips_.begin() + i);
                repaint();
                return;
            }
        }
    }
    
    void mouseWheelMove(const juce::MouseEvent& e, 
                       const juce::MouseWheelDetails& wheel) override
    {
        // Zoom with mouse wheel
        if (e.mods.isCommandDown())
        {
            zoomLevel_ += wheel.deltaY * 0.5f;
            zoomLevel_ = juce::jlimit(0.5f, 4.0f, zoomLevel_);
            pixelsPerBeat_ = 50.0 * zoomLevel_;
            repaint();
        }
    }
    
    void addTrack(const juce::String& name, const juce::Colour& color)
    {
        PlaylistTrack track;
        track.name = name;
        track.color = color;
        tracks_.push_back(track);
        resized();
    }
    
    void addClip(const juce::String& name, int track, double startTime, 
                 double duration, const juce::Colour& color)
    {
        PlaylistClip clip;
        clip.name = name;
        clip.track = track;
        clip.startTime = startTime;
        clip.duration = duration;
        clip.color = color;
        clips_.push_back(clip);
    }
    
    void play()
    {
        if (!isPlaying_)
        {
            isPlaying_ = true;
            playheadPosition_ = 0.0;
            startTimer(50); // 20 FPS for playhead
        }
    }
    
    void stop()
    {
        isPlaying_ = false;
        stopTimer();
        repaint();
    }

    void timerCallback() override
    {
        playheadPosition_ += 0.1; // Advance playhead
        repaint();
    }

private:
    void drawRuler(juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(area);
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        
        // Draw bar numbers
        for (int bar = 0; bar < 64; ++bar)
        {
            int x = static_cast<int>(bar * 4 * pixelsPerBeat_);
            if (x > area.getWidth())
                break;
            
            g.drawText(juce::String(bar + 1), x, area.getY(), 
                      40, area.getHeight(), juce::Justification::centred);
            
            // Tick mark
            g.setColour(juce::Colour(0xff666666));
            g.drawVerticalLine(x, static_cast<float>(area.getY()), 
                             static_cast<float>(area.getBottom()));
            g.setColour(juce::Colours::white);
        }
    }
    
    void drawTrackNames(juce::Graphics& g, juce::Rectangle<int> area)
    {
        area.removeFromTop(30); // Skip ruler height
        
        for (size_t i = 0; i < tracks_.size(); ++i)
        {
            const auto& track = tracks_[i];
            auto trackArea = area.removeFromTop(track.height);
            
            // Background
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRect(trackArea);
            
            // Color indicator
            g.setColour(track.color);
            g.fillRect(trackArea.removeFromLeft(5));
            
            // Track name
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
            g.drawText(track.name, trackArea.reduced(10, 0), 
                      juce::Justification::centredLeft, true);
            
            // Border
            g.setColour(juce::Colour(0xff1a1a1a));
            g.drawRect(area.getX(), trackArea.getY(), 
                      150, trackArea.getHeight());
        }
    }
    
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        // Horizontal lines (tracks)
        g.setColour(juce::Colour(0xff2a2a2a));
        int y = area.getY();
        for (const auto& track : tracks_)
        {
            y += track.height;
            g.drawHorizontalLine(y, static_cast<float>(area.getX()), 
                               static_cast<float>(area.getRight()));
        }
        
        // Vertical lines (beats)
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int beat = 0; beat < 256; ++beat)
        {
            int x = area.getX() + static_cast<int>(beat * pixelsPerBeat_);
            if (x > area.getRight())
                break;
            
            // Darker lines for bar boundaries
            if (beat % 4 == 0)
                g.setColour(juce::Colour(0xff3a3a3a));
            else
                g.setColour(juce::Colour(0xff2a2a2a));
            
            g.drawVerticalLine(x, static_cast<float>(area.getY()), 
                             static_cast<float>(area.getBottom()));
        }
    }
    
    void drawClips(juce::Graphics& g, juce::Rectangle<int> area)
    {
        for (auto& clip : clips_)
        {
            if (clip.track >= tracks_.size())
                continue;
            
            // Calculate position
            int x = area.getX() + static_cast<int>(clip.startTime * pixelsPerBeat_);
            int width = static_cast<int>(clip.duration * pixelsPerBeat_);
            
            int y = area.getY();
            for (int i = 0; i < clip.track; ++i)
                y += tracks_[i].height;
            
            int height = tracks_[clip.track].height - 4;
            
            clip.bounds = juce::Rectangle<float>(static_cast<float>(x), 
                                                 static_cast<float>(y + 2), 
                                                 static_cast<float>(width), 
                                                 static_cast<float>(height));
            
            // Clip color
            juce::Colour clipColor = clip.color;
            if (clip.selected)
                clipColor = juce::Colour(0xffff8c00);
            
            // Draw clip
            g.setColour(clipColor.withAlpha(0.8f));
            g.fillRoundedRectangle(clip.bounds, 5.0f);
            
            // Border
            g.setColour(clipColor.brighter(0.3f));
            g.drawRoundedRectangle(clip.bounds, 5.0f, 2.0f);
            
            // Clip name
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
            g.drawText(clip.name, clip.bounds.reduced(5), 
                      juce::Justification::centredLeft, true);
        }
    }
    
    void drawPlayhead(juce::Graphics& g, juce::Rectangle<int> area)
    {
        int x = area.getX() + static_cast<int>(playheadPosition_ * pixelsPerBeat_);
        
        g.setColour(juce::Colour(0xffff8c00));
        g.drawLine(static_cast<float>(x), static_cast<float>(area.getY()), 
                  static_cast<float>(x), static_cast<float>(area.getBottom()), 2.0f);
        
        // Playhead triangle
        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(x - 6), static_cast<float>(area.getY()),
                           static_cast<float>(x + 6), static_cast<float>(area.getY()),
                           static_cast<float>(x), static_cast<float>(area.getY() + 10));
        g.fillPath(triangle);
    }
    
    int screenYToTrack(int y) const
    {
        int currentY = 70; // Header + ruler
        for (size_t i = 0; i < tracks_.size(); ++i)
        {
            currentY += tracks_[i].height;
            if (y < currentY)
                return i;
        }
        return -1;
    }
    
    double screenXToTime(int x) const
    {
        return (x - 150) / pixelsPerBeat_;
    }
    
    std::vector<PlaylistTrack> tracks_;
    std::vector<PlaylistClip> clips_;
    
    PlaylistClip* draggingClip_{nullptr};
    juce::Point<int> dragStartPos_;
    double dragStartClipTime_{0.0};
    int dragStartClipTrack_{0};
    
    double pixelsPerBeat_{50.0};
    double snapDivision_{4.0}; // 1/4 notes
    float zoomLevel_{1.0f};
    
    bool isPlaying_{false};
    double playheadPosition_{0.0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractivePlaylist)
};

} // namespace OmegaStudio::GUI
