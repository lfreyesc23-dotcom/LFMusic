//==============================================================================
// AdvancedPlaylistFeatures.h
// FL Studio 2025 Advanced Playlist Features
// Time markers, regions, ruler, ghost clips, snap visual
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Time Marker - Marcador temporal con nombre
//==============================================================================
struct TimeMarker {
    double position; // En beats
    juce::String name;
    juce::Colour colour = juce::Colour(0xffff8736);
    
    TimeMarker(double pos = 0.0, const juce::String& markerName = "Marker")
        : position(pos), name(markerName) {}
};

//==============================================================================
// Region - Sección nombrada (Intro, Verse, Chorus, etc.)
//==============================================================================
struct Region {
    double startPosition;
    double endPosition;
    juce::String name;
    juce::Colour colour = juce::Colour(0xff4a90ff);
    bool collapsed = false;
    
    Region(double start = 0.0, double end = 4.0, const juce::String& regionName = "Region")
        : startPosition(start), endPosition(end), name(regionName) {}
    
    double getLength() const { return endPosition - startPosition; }
};

//==============================================================================
// Snap Settings - Configuración de snap con visualización
//==============================================================================
class SnapSettings {
public:
    enum class GridSize {
        Bar,        // 1 bar
        Half,       // 1/2 bar
        Quarter,    // 1/4 bar
        Eighth,     // 1/8
        Sixteenth,  // 1/16
        ThirtyTwo,  // 1/32
        None        // Free
    };
    
    SnapSettings() = default;
    
    void setGridSize(GridSize size) { gridSize = size; }
    GridSize getGridSize() const { return gridSize; }
    
    bool isEnabled() const { return enabled; }
    void setEnabled(bool enable) { enabled = enable; }
    
    double snapPosition(double position, double beatsPerBar = 4.0) const {
        if (!enabled || gridSize == GridSize::None) {
            return position;
        }
        
        double gridInterval = getGridInterval(beatsPerBar);
        return std::round(position / gridInterval) * gridInterval;
    }
    
    double getGridInterval(double beatsPerBar = 4.0) const {
        switch (gridSize) {
            case GridSize::Bar:       return beatsPerBar;
            case GridSize::Half:      return beatsPerBar / 2.0;
            case GridSize::Quarter:   return beatsPerBar / 4.0;
            case GridSize::Eighth:    return 0.5;
            case GridSize::Sixteenth: return 0.25;
            case GridSize::ThirtyTwo: return 0.125;
            default:                  return 0.0;
        }
    }
    
    juce::String getGridSizeString() const {
        switch (gridSize) {
            case GridSize::Bar:       return "1 bar";
            case GridSize::Half:      return "1/2";
            case GridSize::Quarter:   return "1/4";
            case GridSize::Eighth:    return "1/8";
            case GridSize::Sixteenth: return "1/16";
            case GridSize::ThirtyTwo: return "1/32";
            default:                  return "Off";
        }
    }
    
private:
    GridSize gridSize = GridSize::Quarter;
    bool enabled = true;
};

//==============================================================================
// Advanced Ruler - Ruler con markers y regions
//==============================================================================
class AdvancedRuler : public juce::Component {
public:
    AdvancedRuler() {
        setOpaque(true);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();
        
        // Background
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        // Draw regions first (background)
        for (const auto& region : regions) {
            drawRegion(g, region);
        }
        
        // Draw ruler scale
        drawRulerScale(g, bounds);
        
        // Draw time markers
        for (const auto& marker : markers) {
            drawMarker(g, marker);
        }
        
        // Border
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawLine(0, (float)getHeight(), (float)getWidth(), (float)getHeight(), 2.0f);
    }
    
    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isPopupMenu()) {
            showContextMenu(event.x, event.y);
        } else if (event.mods.isCommandDown()) {
            // Cmd+Click = Add marker
            double position = pixelToPosition(event.x);
            addMarker(position, "Marker " + juce::String(markers.size() + 1));
        }
    }
    
    void addMarker(double position, const juce::String& name) {
        markers.push_back(TimeMarker(position, name));
        repaint();
        if (onMarkersChanged) onMarkersChanged();
    }
    
    void addRegion(double start, double end, const juce::String& name) {
        regions.push_back(Region(start, end, name));
        repaint();
        if (onRegionsChanged) onRegionsChanged();
    }
    
    void setPixelsPerBeat(double ppb) {
        pixelsPerBeat = ppb;
        repaint();
    }
    
    double getPixelsPerBeat() const { return pixelsPerBeat; }
    
    std::vector<TimeMarker>& getMarkers() { return markers; }
    std::vector<Region>& getRegions() { return regions; }
    
    std::function<void()> onMarkersChanged;
    std::function<void()> onRegionsChanged;
    
private:
    std::vector<TimeMarker> markers;
    std::vector<Region> regions;
    double pixelsPerBeat = 32.0;
    double beatsPerBar = 4.0;
    
    void drawRulerScale(juce::Graphics& g, juce::Rectangle<int> bounds) {
        g.setColour(juce::Colour(0xff9a9a9a));
        g.setFont(juce::Font(9.0f));
        
        double visibleBeats = getWidth() / pixelsPerBeat;
        double interval = getOptimalInterval(visibleBeats);
        
        for (double beat = 0; beat < visibleBeats; beat += interval) {
            int x = (int)(beat * pixelsPerBeat);
            
            // Draw tick
            int tickHeight = (std::fmod(beat, beatsPerBar) == 0.0) ? 12 : 8;
            g.drawLine((float)x, (float)(getHeight() - tickHeight), 
                      (float)x, (float)getHeight(), 1.0f);
            
            // Draw bar number
            if (std::fmod(beat, beatsPerBar) == 0.0) {
                int barNumber = (int)(beat / beatsPerBar) + 1;
                g.drawText(juce::String(barNumber), x + 2, 2, 40, 16, 
                          juce::Justification::centredLeft);
            }
        }
    }
    
    void drawMarker(juce::Graphics& g, const TimeMarker& marker) {
        int x = positionToPixel(marker.position);
        
        // Marker line
        g.setColour(marker.colour);
        g.drawLine((float)x, 0, (float)x, (float)getHeight(), 2.0f);
        
        // Marker flag
        juce::Path flag;
        flag.startNewSubPath((float)x, 0);
        flag.lineTo((float)(x + 10), 5);
        flag.lineTo((float)(x + 10), 15);
        flag.lineTo((float)x, 20);
        flag.closeSubPath();
        g.fillPath(flag);
        
        // Marker name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(marker.name, x + 12, 2, 100, 16, juce::Justification::centredLeft);
    }
    
    void drawRegion(juce::Graphics& g, const Region& region) {
        int startX = positionToPixel(region.startPosition);
        int endX = positionToPixel(region.endPosition);
        
        auto regionBounds = juce::Rectangle<int>(startX, 0, endX - startX, getHeight());
        
        // Region background
        g.setColour(region.colour.withAlpha(0.2f));
        g.fillRect(regionBounds);
        
        // Region borders
        g.setColour(region.colour);
        g.drawRect(regionBounds, 2);
        
        // Region name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(region.name, regionBounds.reduced(4, 2), 
                  juce::Justification::centredTop);
    }
    
    double getOptimalInterval(double visibleBeats) const {
        if (visibleBeats < 16) return 0.25; // 1/16
        if (visibleBeats < 32) return 0.5;  // 1/8
        if (visibleBeats < 64) return 1.0;  // 1/4
        return beatsPerBar;                 // 1 bar
    }
    
    int positionToPixel(double position) const {
        return (int)(position * pixelsPerBeat);
    }
    
    double pixelToPosition(int pixel) const {
        return pixel / pixelsPerBeat;
    }
    
    void showContextMenu(int x, int y) {
        juce::PopupMenu menu;
        menu.addItem(1, "Add Marker Here");
        menu.addItem(2, "Add Region Here");
        menu.addSeparator();
        menu.addItem(3, "Clear All Markers");
        menu.addItem(4, "Clear All Regions");
        
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this, x](int result) {
                double position = pixelToPosition(x);
                if (result == 1) addMarker(position, "Marker");
                else if (result == 2) addRegion(position, position + 4.0, "Region");
                else if (result == 3) { markers.clear(); repaint(); }
                else if (result == 4) { regions.clear(); repaint(); }
            });
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedRuler)
};

//==============================================================================
// Snap Indicator - Visualización del snap activo
//==============================================================================
class SnapIndicator : public juce::Component {
public:
    SnapIndicator(SnapSettings& settings) : snapSettings(settings) {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(2);
        
        // Background
        auto bgColour = snapSettings.isEnabled() 
            ? juce::Colour(0xffff8736) 
            : juce::Colour(0xff3a3a3a);
        
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Icon
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f));
        g.drawText("🧲", bounds.removeFromLeft(20), juce::Justification::centred);
        
        // Grid size
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(snapSettings.getGridSizeString(), bounds, juce::Justification::centredLeft);
    }
    
    void mouseDown(const juce:MouseEvent& event) override {
        if (event.mods.isPopupMenu()) {
            showGridSizeMenu();
        } else {
            snapSettings.setEnabled(!snapSettings.isEnabled());
            repaint();
        }
    }
    
private:
    SnapSettings& snapSettings;
    
    void showGridSizeMenu() {
        juce::PopupMenu menu;
        menu.addItem(1, "1 bar");
        menu.addItem(2, "1/2");
        menu.addItem(3, "1/4");
        menu.addItem(4, "1/8");
        menu.addItem(5, "1/16");
        menu.addItem(6, "1/32");
        
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result) {
                if (result > 0) {
                    snapSettings.setGridSize((SnapSettings::GridSize)(result - 1));
                    repaint();
                }
            });
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SnapIndicator)
};

//==============================================================================
// Ghost Clips System - Clips semi-transparentes de otros patterns
//==============================================================================
class GhostClipsManager {
public:
    struct GhostClip {
        int sourceTrackIndex;
        double startPosition;
        double length;
        juce::Colour colour;
        juce::String name;
    };
    
    void setShowGhostClips(bool show) { enabled = show; }
    bool isShowingGhostClips() const { return enabled; }
    
    void addGhostClip(const GhostClip& clip) {
        ghostClips.push_back(clip);
    }
    
    void clearGhostClips() {
        ghostClips.clear();
    }
    
    const std::vector<GhostClip>& getGhostClips() const {
        return ghostClips;
    }
    
    void drawGhostClips(juce::Graphics& g, double pixelsPerBeat, int trackHeight) {
        if (!enabled) return;
        
        for (const auto& clip : ghostClips) {
            int x = (int)(clip.startPosition * pixelsPerBeat);
            int width = (int)(clip.length * pixelsPerBeat);
            int y = clip.sourceTrackIndex * trackHeight;
            
            // Draw ghost clip with transparency
            g.setColour(clip.colour.withAlpha(0.3f));
            g.fillRect(x, y, width, trackHeight - 2);
            
            // Draw ghost clip border
            g.setColour(clip.colour.withAlpha(0.5f));
            g.drawRect(x, y, width, trackHeight - 2, 1);
            
            // Draw ghost clip name (faded)
            g.setColour(clip.colour.brighter(0.5f).withAlpha(0.6f));
            g.setFont(juce::Font(9.0f));
            g.drawText(clip.name, x + 4, y + 2, width - 8, trackHeight - 4,
                      juce::Justification::centredLeft, true);
        }
    }
    
private:
    bool enabled = false;
    std::vector<GhostClip> ghostClips;
};

} // namespace GUI
} // namespace OmegaStudio
