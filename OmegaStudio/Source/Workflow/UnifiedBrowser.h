#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio {
namespace Workflow {

/**
 * @brief Browser item types
 */
enum class BrowserItemType {
    Sample,
    Preset,
    Plugin,
    Project,
    Pattern,
    MIDI,
    Folder
};

/**
 * @brief Browser item with metadata
 */
struct BrowserItem {
    juce::File file;
    juce::String name;
    BrowserItemType type{BrowserItemType::Sample};
    
    // Metadata
    std::vector<juce::String> tags;
    float bpm{0.0f};              // For audio samples
    juce::String key;             // Musical key
    int rating{0};                // 1-5 stars
    bool isFavorite{false};
    juce::Time dateAdded;
    juce::Time dateModified;
    int64 fileSize{0};
    
    // Preview
    juce::String waveformPath;    // Cached waveform image
    juce::AudioThumbnail* thumbnail{nullptr};
    
    // For plugins
    juce::String pluginId;
    juce::String manufacturer;
    juce::String category;
    
    BrowserItem() : dateAdded(juce::Time::getCurrentTime()), 
                    dateModified(juce::Time::getCurrentTime()) {}
};

/**
 * @brief Smart search engine with fuzzy matching
 */
class SearchEngine {
public:
    struct SearchCriteria {
        juce::String query;
        BrowserItemType typeFilter{BrowserItemType::Sample};
        bool filterByType{false};
        
        float minBPM{0.0f};
        float maxBPM{999.0f};
        bool filterByBPM{false};
        
        juce::String key;
        bool filterByKey{false};
        
        std::vector<juce::String> tags;
        bool filterByTags{false};
        
        bool favoritesOnly{false};
        int minRating{0};
    };
    
    std::vector<BrowserItem*> search(const std::vector<BrowserItem>& items, 
                                     const SearchCriteria& criteria) {
        std::vector<BrowserItem*> results;
        
        for (const auto& item : items) {
            if (matchesCriteria(const_cast<BrowserItem&>(item), criteria)) {
                results.push_back(const_cast<BrowserItem*>(&item));
            }
        }
        
        // Sort by relevance
        std::sort(results.begin(), results.end(), 
            [&criteria](const BrowserItem* a, const BrowserItem* b) {
                return calculateRelevance(*a, criteria) > calculateRelevance(*b, criteria);
            });
        
        return results;
    }
    
private:
    bool matchesCriteria(BrowserItem& item, const SearchCriteria& criteria) {
        // Type filter
        if (criteria.filterByType && item.type != criteria.typeFilter) {
            return false;
        }
        
        // BPM filter
        if (criteria.filterByBPM) {
            if (item.bpm < criteria.minBPM || item.bpm > criteria.maxBPM) {
                return false;
            }
        }
        
        // Key filter
        if (criteria.filterByKey && !criteria.key.isEmpty()) {
            if (item.key != criteria.key) {
                return false;
            }
        }
        
        // Tags filter
        if (criteria.filterByTags && !criteria.tags.empty()) {
            bool hasAnyTag = false;
            for (const auto& tag : criteria.tags) {
                if (std::find(item.tags.begin(), item.tags.end(), tag) != item.tags.end()) {
                    hasAnyTag = true;
                    break;
                }
            }
            if (!hasAnyTag) return false;
        }
        
        // Favorites filter
        if (criteria.favoritesOnly && !item.isFavorite) {
            return false;
        }
        
        // Rating filter
        if (item.rating < criteria.minRating) {
            return false;
        }
        
        // Text query (fuzzy match)
        if (!criteria.query.isEmpty()) {
            juce::String itemText = item.name.toLowerCase();
            juce::String query = criteria.query.toLowerCase();
            
            if (!itemText.contains(query)) {
                // Try fuzzy matching
                if (fuzzyMatch(itemText, query) < 0.6f) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    float calculateRelevance(const BrowserItem& item, const SearchCriteria& criteria) {
        float score = 0.0f;
        
        // Exact name match
        if (item.name.toLowerCase() == criteria.query.toLowerCase()) {
            score += 100.0f;
        }
        
        // Starts with query
        if (item.name.toLowerCase().startsWith(criteria.query.toLowerCase())) {
            score += 50.0f;
        }
        
        // Contains query
        if (item.name.toLowerCase().contains(criteria.query.toLowerCase())) {
            score += 25.0f;
        }
        
        // Favorite boost
        if (item.isFavorite) {
            score += 10.0f;
        }
        
        // Rating boost
        score += item.rating * 5.0f;
        
        // Recent files boost
        auto age = juce::Time::getCurrentTime() - item.dateModified;
        if (age.inDays() < 7) {
            score += 10.0f;
        }
        
        return score;
    }
    
    float fuzzyMatch(const juce::String& text, const juce::String& query) {
        if (query.isEmpty()) return 1.0f;
        
        int matches = 0;
        int queryPos = 0;
        
        for (int i = 0; i < text.length() && queryPos < query.length(); ++i) {
            if (text[i] == query[queryPos]) {
                matches++;
                queryPos++;
            }
        }
        
        return static_cast<float>(matches) / query.length();
    }
};

/**
 * @brief Unified Browser Component (FL Studio-style)
 */
class UnifiedBrowser : public juce::Component,
                       public juce::FileDragAndDropTarget {
public:
    UnifiedBrowser();
    ~UnifiedBrowser() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Drag & Drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    // Database management
    void scanDirectory(const juce::File& directory, bool recursive = true);
    void addItem(const BrowserItem& item);
    void removeItem(const juce::File& file);
    void refreshDatabase();
    
    // Favorites
    void addToFavorites(BrowserItem* item);
    void removeFromFavorites(BrowserItem* item);
    std::vector<BrowserItem*> getFavorites();
    
    // Recent files
    void addToRecent(const BrowserItem& item);
    std::vector<BrowserItem*> getRecent(int count = 20);
    
    // Search
    void setSearchQuery(const juce::String& query);
    void setSearchFilter(BrowserItemType type);
    void setBPMRange(float min, float max);
    void setKeyFilter(const juce::String& key);
    void setTagFilter(const std::vector<juce::String>& tags);
    
    // Preview
    void startPreview(BrowserItem* item);
    void stopPreview();
    bool isPreviewActive() const { return previewActive_; }
    
    // Callbacks
    std::function<void(BrowserItem*)> onItemDoubleClicked;
    std::function<void(BrowserItem*, int x, int y)> onItemDragStarted;
    std::function<void(BrowserItem*)> onItemSelected;
    
private:
    // Database
    std::vector<BrowserItem> items_;
    std::vector<BrowserItem*> filteredItems_;
    std::vector<BrowserItem*> recentItems_;
    
    // Search
    SearchEngine searchEngine_;
    SearchEngine::SearchCriteria searchCriteria_;
    
    // UI Components
    std::unique_ptr<juce::TextEditor> searchBox_;
    std::unique_ptr<juce::ComboBox> typeFilter_;
    std::unique_ptr<juce::ComboBox> keyFilter_;
    std::unique_ptr<juce::Slider> bpmMinSlider_;
    std::unique_ptr<juce::Slider> bpmMaxSlider_;
    std::unique_ptr<juce::TreeView> folderTree_;
    std::unique_ptr<juce::ListBox> itemList_;
    std::unique_ptr<juce::TabbedComponent> tabs_;
    
    // Preview
    std::unique_ptr<juce::AudioFormatManager> formatManager_;
    std::unique_ptr<juce::AudioTransportSource> transportSource_;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
    bool previewActive_{false};
    
    // Selected item
    BrowserItem* selectedItem_{nullptr};
    
    // Metadata cache
    juce::File metadataFile_;
    
    void initializeUI();
    void performSearch();
    void updateItemList();
    void loadMetadata();
    void saveMetadata();
    
    // Item rendering
    class ItemListModel : public juce::ListBoxModel {
    public:
        ItemListModel(UnifiedBrowser& owner) : owner_(owner) {}
        
        int getNumRows() override {
            return static_cast<int>(owner_.filteredItems_.size());
        }
        
        void paintListBoxItem(int rowNumber, juce::Graphics& g,
                            int width, int height, bool rowIsSelected) override;
        
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
        
    private:
        UnifiedBrowser& owner_;
    };
    
    std::unique_ptr<ItemListModel> listModel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedBrowser)
};

/**
 * @brief Browser Item Component (individual item rendering)
 */
class BrowserItemComponent : public juce::Component {
public:
    BrowserItemComponent(BrowserItem& item) : item_(item) {
        setSize(200, 60);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();
        
        // Background
        if (isSelected_) {
            g.setColour(juce::Colours::blue.withAlpha(0.3f));
            g.fillRect(bounds);
        }
        
        // Icon
        auto iconBounds = bounds.removeFromLeft(50).reduced(5);
        g.setColour(juce::Colours::white);
        drawItemIcon(g, iconBounds);
        
        // Text info
        auto textBounds = bounds.reduced(5);
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(item_.name, textBounds.removeFromTop(20), 
                  juce::Justification::left, true);
        
        // Metadata
        g.setFont(10.0f);
        g.setColour(juce::Colours::grey);
        juce::String info;
        
        if (item_.bpm > 0) {
            info += juce::String(item_.bpm, 0) + " BPM  ";
        }
        if (!item_.key.isEmpty()) {
            info += item_.key + "  ";
        }
        
        g.drawText(info, textBounds.removeFromTop(15), 
                  juce::Justification::left, true);
        
        // Rating stars
        if (item_.rating > 0) {
            auto starBounds = textBounds.removeFromTop(15);
            for (int i = 0; i < item_.rating; ++i) {
                g.setColour(juce::Colours::yellow);
                g.fillEllipse(starBounds.removeFromLeft(12).toFloat());
                starBounds.removeFromLeft(2);
            }
        }
        
        // Favorite indicator
        if (item_.isFavorite) {
            g.setColour(juce::Colours::red);
            g.fillEllipse(getWidth() - 20.0f, 5.0f, 15.0f, 15.0f);
        }
    }
    
    void setSelected(bool selected) {
        isSelected_ = selected;
        repaint();
    }
    
private:
    BrowserItem& item_;
    bool isSelected_{false};
    
    void drawItemIcon(juce::Graphics& g, juce::Rectangle<int> bounds) {
        switch (item_.type) {
            case BrowserItemType::Sample:
                g.drawText("🎵", bounds, juce::Justification::centred);
                break;
            case BrowserItemType::Preset:
                g.drawText("🎛️", bounds, juce::Justification::centred);
                break;
            case BrowserItemType::Plugin:
                g.drawText("🔌", bounds, juce::Justification::centred);
                break;
            case BrowserItemType::MIDI:
                g.drawText("🎹", bounds, juce::Justification::centred);
                break;
            default:
                g.drawText("📁", bounds, juce::Justification::centred);
                break;
        }
    }
};

} // namespace Workflow
} // namespace OmegaStudio
