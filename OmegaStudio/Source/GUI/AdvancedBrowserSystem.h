//==============================================================================
// AdvancedBrowserSystem.h
// FL Studio 2025 Advanced Browser
// Grid/List/Icon views, Ratings, Tags, Preview player con waveform
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Sample Item - Item con metadata completa
//==============================================================================
struct SampleItem {
    juce::File file;
    juce::String name;
    juce::String category;
    int rating = 0; // 0-5 stars
    std::vector<juce::String> tags;
    juce::Colour colour = juce::Colours::grey;
    bool isFavorite = false;
    double bpm = 0.0;
    juce::String key;
    int64 fileSize = 0;
    juce::String format;
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("path", file.getFullPathName());
        obj->setProperty("name", name);
        obj->setProperty("category", category);
        obj->setProperty("rating", rating);
        obj->setProperty("favorite", isFavorite);
        obj->setProperty("bpm", bpm);
        obj->setProperty("key", key);
        
        juce::Array<juce::var> tagsArray;
        for (const auto& tag : tags) {
            tagsArray.add(tag);
        }
        obj->setProperty("tags", tagsArray);
        
        return juce::var(obj);
    }
};

//==============================================================================
// Preview Player - Reproductor con waveform
//==============================================================================
class PreviewPlayer : public juce::Component,
                     private juce::Timer {
public:
    PreviewPlayer() {
        formatManager.registerBasicFormats();
        startTimer(30); // 30ms para actualización suave
        setOpaque(true);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();
        
        // Background
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        if (thumbnail && thumbnail->getNumChannels() > 0) {
            // Draw waveform
            g.setColour(juce::Colour(0xff2a2a2a));
            thumbnail->drawChannels(g, bounds.reduced(2), 
                                   0.0, thumbnail->getTotalLength(),
                                   1.0f);
            
            // Draw waveform fill
            g.setColour(juce::Colour(0xff4a90ff));
            thumbnail->drawChannels(g, bounds.reduced(2), 
                                   0.0, thumbnail->getTotalLength(),
                                   0.8f);
            
            // Draw playhead
            if (isPlaying) {
                float progress = (float)transportSource.getCurrentPosition() / 
                                (float)thumbnail->getTotalLength();
                int playheadX = (int)(progress * bounds.getWidth());
                
                g.setColour(juce::Colour(0xffff8736));
                g.drawLine((float)playheadX, 0, (float)playheadX, 
                          (float)bounds.getHeight(), 2.0f);
            }
        } else {
            // No file loaded
            g.setColour(juce::Colour(0xff6a6a6a));
            g.setFont(juce::Font(12.0f));
            g.drawText("No preview", bounds, juce::Justification::centred);
        }
        
        // Time display
        if (thumbnail && thumbnail->getTotalLength() > 0) {
            g.setColour(juce::Colour(0xffdddddd));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            
            auto timeArea = bounds.removeFromBottom(16).reduced(4, 2);
            g.drawText(getTimeString(transportSource.getCurrentPosition()), 
                      timeArea.removeFromLeft(50), juce::Justification::centredLeft);
            g.drawText(getTimeString(thumbnail->getTotalLength()), 
                      timeArea.removeFromRight(50), juce::Justification::centredRight);
        }
        
        // Border
        g.setColour(juce::Colour(0xff0a0a0a));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void mouseDown(const juce::MouseEvent& event) override {
        if (thumbnail && thumbnail->getTotalLength() > 0) {
            // Click to seek
            float progress = (float)event.x / (float)getWidth();
            double newPosition = progress * thumbnail->getTotalLength();
            transportSource.setPosition(newPosition);
        }
    }
    
    void mouseDoubleClick(const juce::MouseEvent&) override {
        togglePlayPause();
    }
    
    void loadFile(const juce::File& file) {
        auto* reader = formatManager.createReaderFor(file);
        
        if (reader != nullptr) {
            std::unique_ptr<juce::AudioFormatReaderSource> newSource(
                new juce::AudioFormatReaderSource(reader, true));
            
            transportSource.setSource(newSource.get(), 0, nullptr, 
                                     reader->sampleRate);
            
            readerSource.reset(newSource.release());
            
            // Create thumbnail
            thumbnail = std::make_unique<juce::AudioThumbnail>(512, formatManager, 
                                                              thumbnailCache);
            thumbnail->setSource(new juce::FileInputSource(file));
            
            repaint();
        }
    }
    
    void play() {
        if (!isPlaying) {
            transportSource.start();
            isPlaying = true;
        }
    }
    
    void stop() {
        transportSource.stop();
        transportSource.setPosition(0);
        isPlaying = false;
        repaint();
    }
    
    void togglePlayPause() {
        if (isPlaying) {
            stop();
        } else {
            play();
        }
    }
    
    bool isCurrentlyPlaying() const { return isPlaying; }
    
private:
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache{10};
    std::unique_ptr<juce::AudioThumbnail> thumbnail;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    bool isPlaying = false;
    
    void timerCallback() override {
        if (isPlaying) {
            repaint();
            
            // Auto-stop at end
            if (transportSource.getCurrentPosition() >= transportSource.getLengthInSeconds()) {
                stop();
            }
        }
    }
    
    juce::String getTimeString(double seconds) const {
        int minutes = (int)seconds / 60;
        int secs = (int)seconds % 60;
        return juce::String::formatted("%d:%02d", minutes, secs);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreviewPlayer)
};

//==============================================================================
// Rating Component - Sistema de estrellas 5-star
//==============================================================================
class RatingComponent : public juce::Component {
public:
    RatingComponent() = default;
    
    void paint(juce::Graphics& g) override {
        float starWidth = getWidth() / 5.0f;
        
        for (int i = 0; i < 5; ++i) {
            float x = i * starWidth;
            bool filled = (i < rating);
            
            if (hoverRating >= 0 && i < hoverRating) {
                filled = true;
            }
            
            drawStar(g, x + starWidth / 2, (float)getHeight() / 2, 
                    starWidth * 0.4f, filled);
        }
    }
    
    void mouseMove(const juce::MouseEvent& event) override {
        hoverRating = (event.x * 5) / getWidth();
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        hoverRating = -1;
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& event) override {
        int newRating = juce::jlimit(0, 5, (event.x * 5) / getWidth());
        setRating(newRating);
        
        if (onRatingChanged) {
            onRatingChanged(rating);
        }
    }
    
    void setRating(int newRating) {
        rating = juce::jlimit(0, 5, newRating);
        repaint();
    }
    
    int getRating() const { return rating; }
    
    std::function<void(int)> onRatingChanged;
    
private:
    int rating = 0;
    int hoverRating = -1;
    
    void drawStar(juce::Graphics& g, float cx, float cy, float radius, bool filled) {
        juce::Path star;
        
        // 5-pointed star
        for (int i = 0; i < 10; ++i) {
            float angle = i * juce::MathConstants<float>::pi / 5.0f - juce::MathConstants<float>::halfPi;
            float r = (i % 2 == 0) ? radius : radius * 0.4f;
            float x = cx + r * std::cos(angle);
            float y = cy + r * std::sin(angle);
            
            if (i == 0)
                star.startNewSubPath(x, y);
            else
                star.lineTo(x, y);
        }
        star.closeSubPath();
        
        if (filled) {
            g.setColour(juce::Colour(0xffffd700)); // Gold
            g.fillPath(star);
        } else {
            g.setColour(juce::Colour(0xff4a4a4a));
            g.fillPath(star);
        }
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.strokePath(star, juce::PathStrokeType(1.0f));
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RatingComponent)
};

//==============================================================================
// Browser View Modes
//==============================================================================
enum class BrowserViewMode {
    List,
    Grid,
    Icons
};

//==============================================================================
// Advanced Browser Panel - Browser completo
//==============================================================================
class AdvancedBrowserPanel : public juce::Component {
public:
    AdvancedBrowserPanel() {
        // View mode selector
        viewModeSelector.addItem("☰ List", 1);
        viewModeSelector.addItem("⊞ Grid", 2);
        viewModeSelector.addItem("🖼 Icons", 3);
        viewModeSelector.setSelectedId(1);
        viewModeSelector.onChange = [this] { changeViewMode(); };
        addAndMakeVisible(viewModeSelector);
        
        // Search box
        searchBox.setTextToShowWhenEmpty("🔍 Search samples...", juce::Colour(0xff6a6a6a));
        searchBox.onTextChange = [this] { filterResults(); };
        addAndMakeVisible(searchBox);
        
        // Preview player
        addAndMakeVisible(previewPlayer);
        
        // Rating filter
        addAndMakeVisible(ratingFilter);
        
        // Tags panel
        tagsLabel.setText("TAGS:", juce::dontSendNotification);
        tagsLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9a9a9a));
        addAndMakeVisible(tagsLabel);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2b2b2b));
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(4);
        
        // Top bar
        auto topBar = bounds.removeFromTop(32);
        viewModeSelector.setBounds(topBar.removeFromLeft(100));
        topBar.removeFromLeft(4);
        searchBox.setBounds(topBar);
        
        bounds.removeFromTop(4);
        
        // Preview player (bottom)
        previewPlayer.setBounds(bounds.removeFromBottom(80));
        bounds.removeFromBottom(4);
        
        // Rating filter
        auto ratingArea = bounds.removeFromBottom(24);
        ratingFilter.setBounds(ratingArea.removeFromRight(120));
        
        // Tags
        bounds.removeFromBottom(4);
        auto tagsArea = bounds.removeFromBottom(24);
        tagsLabel.setBounds(tagsArea.removeFromLeft(50));
        
        // Main content area para list/grid/icons
        contentArea = bounds;
    }
    
private:
    juce::ComboBox viewModeSelector;
    juce::TextEditor searchBox;
    PreviewPlayer previewPlayer;
    RatingComponent ratingFilter;
    juce::Label tagsLabel;
    juce::Rectangle<int> contentArea;
    BrowserViewMode currentViewMode = BrowserViewMode::List;
    std::vector<SampleItem> items;
    
    void changeViewMode() {
        currentViewMode = (BrowserViewMode)(viewModeSelector.getSelectedId() - 1);
        repaint();
    }
    
    void filterResults() {
        juce::String searchText = searchBox.getText().toLowerCase();
        // Filter items based on search
        repaint();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedBrowserPanel)
};

} // namespace GUI
} // namespace OmegaStudio
