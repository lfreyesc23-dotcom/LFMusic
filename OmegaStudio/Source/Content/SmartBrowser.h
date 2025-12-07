#pragma once
#include <JuceHeader.h>
#include <string>
#include <vector>

namespace OmegaStudio {

/**
 * @brief Smart Content Browser with Fuzzy Search
 * 
 * Features:
 * - Unified browser for samples, presets, plugins
 * - Fuzzy search algorithm
 * - Filters: BPM, Key, Rating, Tags, Categories
 * - Favorites system
 * - Auto-tagging with ML
 * - Waveform preview
 * - Drag & drop to timeline
 */
class SmartBrowser {
public:
    //==============================================================================
    enum class ContentType {
        Sample,
        Preset,
        Plugin,
        MIDI,
        Project
    };
    
    struct ContentItem {
        juce::String name;
        juce::String path;
        ContentType type;
        
        // Metadata
        juce::String category;
        std::vector<juce::String> tags;
        float bpm = 0.0f;
        juce::String key;          // "Am", "C", etc.
        int rating = 0;            // 0-5 stars
        bool isFavorite = false;
        
        // Preview
        juce::Image waveform;
        juce::AudioBuffer<float> previewBuffer;
        
        // Usage stats
        int timesUsed = 0;
        juce::Time lastUsed;
    };
    
    struct SearchQuery {
        juce::String text;
        ContentType typeFilter = ContentType::Sample;
        
        // Filters
        std::vector<juce::String> categories;
        std::vector<juce::String> tags;
        float bpmMin = 0.0f, bpmMax = 999.0f;
        juce::String keyFilter;
        int minRating = 0;
        bool favoritesOnly = false;
        
        // Sort
        enum SortBy { Name, Date, BPM, Rating, TimesUsed } sortBy = Name;
        bool ascending = true;
    };
    
    //==============================================================================
    SmartBrowser();
    ~SmartBrowser();
    
    // Content management
    void scanDirectory(const juce::File& directory, bool recursive = true);
    void addItem(const ContentItem& item);
    void removeItem(const juce::String& path);
    void clearDatabase();
    
    // Search
    std::vector<ContentItem> search(const SearchQuery& query);
    std::vector<ContentItem> fuzzySearch(const juce::String& query, float threshold = 0.6f);
    
    // Favorites
    void addToFavorites(const juce::String& path);
    void removeFromFavorites(const juce::String& path);
    std::vector<ContentItem> getFavorites();
    
    // Categories & Tags
    std::vector<juce::String> getAllCategories();
    std::vector<juce::String> getAllTags();
    void addTag(const juce::String& path, const juce::String& tag);
    
    // Database
    void saveDatabase(const juce::File& file);
    void loadDatabase(const juce::File& file);
    
private:
    std::vector<ContentItem> database;
    
    // Fuzzy matching algorithm (Levenshtein distance)
    float calculateSimilarity(const juce::String& a, const juce::String& b);
    
    // Auto-tagging with basic ML
    std::vector<juce::String> autoGenerateTags(const ContentItem& item);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartBrowser)
};

} // namespace OmegaStudio
