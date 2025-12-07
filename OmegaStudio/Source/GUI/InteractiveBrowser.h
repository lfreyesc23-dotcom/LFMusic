#pragma once

#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio::GUI {

/**
 * @brief Sample Item - Representa un sample en el browser
 */
struct SampleItem
{
    juce::String name;
    juce::File file;
    juce::String category;
    int rating{0}; // 0-5 stars
    juce::Array<juce::String> tags;
    juce::Colour color;
    
    bool matchesSearch(const juce::String& searchText) const
    {
        if (searchText.isEmpty())
            return true;
        
        if (searchText.isEmpty())
            return true;
        
        juce::String search = searchText.toLowerCase();
        if (name.toLowerCase().containsIgnoreCase(search))
            return true;
        if (category.toLowerCase().containsIgnoreCase(search))
            return true;
        
        for (const auto& tag : tags)
        {
            if (tag.toLowerCase().containsIgnoreCase(search))
                return true;
        }
        
        return false;
    }
};

/**
 * @brief Sample Row - Una fila en el browser list
 */
class SampleRow : public juce::Component
{
public:
    SampleRow(const SampleItem& item) : item_(item)
    {
        setSize(300, 40);
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        
        // Background
        if (isMouseOver_)
            g.setColour(juce::Colour(0xff3a3a3a));
        else
            g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(bounds);
        
        // Color indicator
        g.setColour(item_.color);
        g.fillRect(bounds.removeFromLeft(4));
        
        bounds.removeFromLeft(5); // Padding
        
        // Icon
        g.setColour(juce::Colour(0xff888888));
        g.setFont(juce::FontOptions(20.0f, juce::Font::plain));
        auto iconArea = bounds.removeFromLeft(30);
        g.drawText("🎵", iconArea, juce::Justification::centred);
        
        // Name
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
        auto nameArea = bounds.removeFromLeft(bounds.getWidth() - 80);
        g.drawText(item_.name, nameArea.reduced(5, 0), 
                  juce::Justification::centredLeft, true);
        
        // Rating stars
        auto ratingArea = bounds.removeFromRight(80);
        drawRating(g, ratingArea);
        
        // Border
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(getLocalBounds());
    }
    
    void mouseEnter(const juce::MouseEvent&) override
    {
        isMouseOver_ = true;
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override
    {
        isMouseOver_ = false;
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            showContextMenu();
        }
        else
        {
            // Start drag
            isDragging_ = true;
            
            if (onStartDrag)
                onStartDrag(item_);
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override
    {
        isDragging_ = false;
    }
    
    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (onDoubleClick)
            onDoubleClick(item_);
    }
    
    const SampleItem& getItem() const { return item_; }
    
    std::function<void(const SampleItem&)> onStartDrag;
    std::function<void(const SampleItem&)> onDoubleClick;

private:
    void drawRating(juce::Graphics& g, juce::Rectangle<int> area)
    {
        int starSize = 12;
        int spacing = 2;
        int x = area.getX();
        int y = area.getCentreY() - starSize / 2;
        
        for (int i = 0; i < 5; ++i)
        {
            if (i < item_.rating)
                g.setColour(juce::Colour(0xffffd700)); // Gold
            else
                g.setColour(juce::Colour(0xff444444)); // Grey
            
            g.setFont(juce::FontOptions(starSize, juce::Font::plain));
            g.drawText("★", x, y, starSize, starSize, 
                      juce::Justification::centred);
            x += starSize + spacing;
        }
    }
    
    void showContextMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Load to Channel");
        menu.addItem(2, "Preview");
        menu.addSeparator();
        menu.addItem(3, "Add to Favorites");
        menu.addItem(4, "Edit Tags");
        menu.addSeparator();
        
        juce::PopupMenu ratingMenu;
        for (int i = 1; i <= 5; ++i)
        {
            ratingMenu.addItem(10 + i, juce::String(i) + " Star" + (i > 1 ? "s" : ""));
        }
        menu.addSubMenu("Set Rating", ratingMenu);
        
        menu.showMenuAsync(juce::PopupMenu::Options());
    }
    
    SampleItem item_;
    bool isMouseOver_{false};
    bool isDragging_{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRow)
};

/**
 * @brief Interactive Browser - Navegador de samples con drag & drop
 */
class InteractiveBrowser : public juce::Component,
                           public juce::FileDragAndDropTarget
{
public:
    InteractiveBrowser()
    {
        // Search box
        searchBox_.setTextToShowWhenEmpty("Search samples, presets...", juce::Colours::grey);
        searchBox_.onTextChange = [this]() {
            filterSamples();
        };
        addAndMakeVisible(searchBox_);
        
        // Category buttons
        addCategoryButton("All", juce::Colour(0xff888888));
        addCategoryButton("Drums", juce::Colour(0xffff0000));
        addCategoryButton("Bass", juce::Colour(0xff00ff00));
        addCategoryButton("Synth", juce::Colour(0xff0000ff));
        addCategoryButton("FX", juce::Colour(0xffff00ff));
        addCategoryButton("Favorites", juce::Colour(0xffffd700));
        
        // Viewport for scrolling
        viewport_.setViewedComponent(&contentComponent_, false);
        addAndMakeVisible(viewport_);
        
        // Add sample items (examples)
        addSample("Kick_808.wav", "Drums", 5, juce::Colour(0xffff0000));
        addSample("Snare_Tight.wav", "Drums", 4, juce::Colour(0xffff0000));
        addSample("HiHat_Closed.wav", "Drums", 4, juce::Colour(0xffff0000));
        addSample("Bass_Wobble.wav", "Bass", 5, juce::Colour(0xff00ff00));
        addSample("Bass_Sub.wav", "Bass", 4, juce::Colour(0xff00ff00));
        addSample("Synth_Lead.wav", "Synth", 3, juce::Colour(0xff0000ff));
        addSample("Synth_Pad.wav", "Synth", 4, juce::Colour(0xff0000ff));
        addSample("Reverb_Large.wav", "FX", 5, juce::Colour(0xffff00ff));
        addSample("Delay_Echo.wav", "FX", 4, juce::Colour(0xffff00ff));
        addSample("Vocal_Chop.wav", "Synth", 5, juce::Colour(0xff00ffff));
        
        filterSamples();
        
        setSize(300, 600);
    }
    
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Header
        auto header = getLocalBounds().removeFromTop(40);
        g.setColour(juce::Colour(0xff2d2d2d));
        g.fillRect(header);
        
        // Title
        g.setColour(juce::Colour(0xffff8c00));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("📁 BROWSER", header.reduced(10, 0), 
                  juce::Justification::centredLeft);
        
        // Sample count
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        g.drawText(juce::String(filteredSamples_.size()) + " items", 
                  header.removeFromRight(80).reduced(5, 0),
                  juce::Justification::centredRight);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); // Skip header
        
        // Search box
        searchBox_.setBounds(bounds.removeFromTop(30).reduced(5, 3));
        
        // Category buttons
        auto categoryArea = bounds.removeFromTop(35);
        int buttonWidth = categoryArea.getWidth() / categoryButtons_.size();
        for (auto* button : categoryButtons_)
        {
            button->setBounds(categoryArea.removeFromLeft(buttonWidth).reduced(2));
        }
        
        // Viewport
        viewport_.setBounds(bounds);
        
        // Layout sample rows
        layoutSampleRows();
    }
    
    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    
    void filesDropped(const juce::StringArray& files, int, int) override
    {
        for (const auto& filePath : files)
        {
            juce::File file(filePath);
            if (file.hasFileExtension(".wav;.mp3;.aif;.flac"))
            {
                addSample(file.getFileName(), "Imported", 3, 
                         juce::Colour(0xff888888));
            }
        }
        filterSamples();
    }
    
    void addSample(const juce::String& name, const juce::String& category, 
                   int rating, const juce::Colour& color)
    {
        SampleItem item;
        item.name = name;
        item.category = category;
        item.rating = rating;
        item.color = color;
        
        // Add tags based on name
        if (name.toLowerCase().contains("kick"))
            item.tags.add("kick");
        if (name.toLowerCase().contains("snare"))
            item.tags.add("snare");
        if (name.toLowerCase().contains("808"))
            item.tags.add("808");
        
        samples_.push_back(item);
    }
    
    std::function<void(const SampleItem&)> onSampleDragStart;
    std::function<void(const SampleItem&)> onSampleDoubleClick;

private:
    void addCategoryButton(const juce::String& name, const juce::Colour& color)
    {
        auto* button = new juce::TextButton(name);
        button->setColour(juce::TextButton::buttonColourId, color.darker(0.5f));
        button->setColour(juce::TextButton::buttonOnColourId, color);
        button->setClickingTogglesState(true);
        button->onClick = [this, name]() {
            currentCategory_ = name;
            filterSamples();
        };
        
        if (name == "All")
            button->setToggleState(true, juce::dontSendNotification);
        
        categoryButtons_.add(button);
        addAndMakeVisible(button);
    }
    
    void filterSamples()
    {
        filteredSamples_.clear();
        
        juce::String searchText = searchBox_.getText();
        
        for (const auto& sample : samples_)
        {
            // Category filter
            if (currentCategory_ != "All" && sample.category != currentCategory_)
                continue;
            
            // Search filter
            if (!sample.matchesSearch(searchText))
                continue;
            
            filteredSamples_.push_back(sample);
        }
        
        layoutSampleRows();
        repaint();
    }
    
    void layoutSampleRows()
    {
        sampleRows_.clear();
        
        int y = 0;
        for (const auto& sample : filteredSamples_)
        {
            auto* row = new SampleRow(sample);
            row->setBounds(0, y, viewport_.getWidth() - 20, 40);
            
            row->onStartDrag = [this](const SampleItem& item) {
                DBG("🎵 Drag started: " << item.name);
                if (onSampleDragStart)
                    onSampleDragStart(item);
            };
            
            row->onDoubleClick = [this](const SampleItem& item) {
                DBG("🎵 Double click: " << item.name);
                if (onSampleDoubleClick)
                    onSampleDoubleClick(item);
            };
            
            sampleRows_.add(row);
            contentComponent_.addAndMakeVisible(row);
            
            y += 40;
        }
        
        contentComponent_.setSize(viewport_.getWidth() - 20, y);
    }
    
    std::vector<SampleItem> samples_;
    std::vector<SampleItem> filteredSamples_;
    juce::String currentCategory_{"All"};
    
    juce::TextEditor searchBox_;
    juce::OwnedArray<juce::TextButton> categoryButtons_;
    juce::Viewport viewport_;
    juce::Component contentComponent_;
    juce::OwnedArray<SampleRow> sampleRows_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveBrowser)
};

} // namespace OmegaStudio::GUI
