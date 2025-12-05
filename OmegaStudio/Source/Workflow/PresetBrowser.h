/*
  ==============================================================================

    PresetBrowser.h
    Global preset browser with fuzzy search and tagging

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>

namespace omega {

struct PresetInfo {
    juce::String name;
    juce::String pluginName;
    juce::String category;
    juce::StringArray tags;
    juce::File file;
    bool isFavorite = false;
    
    PresetInfo() = default;
};

class PresetBrowser {
public:
    PresetBrowser();
    
    // Search
    std::vector<PresetInfo> search(const juce::String& query);
    std::vector<PresetInfo> searchByTag(const juce::String& tag);
    std::vector<PresetInfo> searchByCategory(const juce::String& category);
    std::vector<PresetInfo> getFavorites();
    
    // Management
    void addPreset(const PresetInfo& preset);
    void removePreset(const juce::String& name);
    void toggleFavorite(const juce::String& name);
    
    // Scanning
    void scanDirectory(const juce::File& directory);
    void rescan();
    
    int getPresetCount() const { return static_cast<int>(presets_.size()); }
    
private:
    std::vector<PresetInfo> presets_;
    float fuzzyMatch(const juce::String& query, const juce::String& target);
};

} // namespace omega
