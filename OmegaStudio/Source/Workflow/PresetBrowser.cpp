/*
  ==============================================================================

    PresetBrowser.cpp

  ==============================================================================
*/

#include "PresetBrowser.h"

namespace omega {

PresetBrowser::PresetBrowser() {}

std::vector<PresetInfo> PresetBrowser::search(const juce::String& query) {
    std::vector<PresetInfo> results;
    juce::String lowerQuery = query.toLowerCase();
    
    for (const auto& preset : presets_) {
        float score = fuzzyMatch(lowerQuery, preset.name.toLowerCase());
        if (score > 0.3f) {
            results.push_back(preset);
        }
    }
    
    return results;
}

std::vector<PresetInfo> PresetBrowser::searchByTag(const juce::String& tag) {
    std::vector<PresetInfo> results;
    for (const auto& preset : presets_) {
        if (preset.tags.contains(tag)) {
            results.push_back(preset);
        }
    }
    return results;
}

std::vector<PresetInfo> PresetBrowser::searchByCategory(const juce::String& category) {
    std::vector<PresetInfo> results;
    for (const auto& preset : presets_) {
        if (preset.category == category) {
            results.push_back(preset);
        }
    }
    return results;
}

std::vector<PresetInfo> PresetBrowser::getFavorites() {
    std::vector<PresetInfo> results;
    for (const auto& preset : presets_) {
        if (preset.isFavorite) {
            results.push_back(preset);
        }
    }
    return results;
}

void PresetBrowser::addPreset(const PresetInfo& preset) {
    presets_.push_back(preset);
}

void PresetBrowser::removePreset(const juce::String& name) {
    presets_.erase(std::remove_if(presets_.begin(), presets_.end(),
        [&name](const PresetInfo& p) { return p.name == name; }), presets_.end());
}

void PresetBrowser::toggleFavorite(const juce::String& name) {
    for (auto& preset : presets_) {
        if (preset.name == name) {
            preset.isFavorite = !preset.isFavorite;
            break;
        }
    }
}

float PresetBrowser::fuzzyMatch(const juce::String& query, const juce::String& target) {
    if (target.contains(query)) return 1.0f;
    
    int matches = 0;
    int lastIndex = -1;
    
    for (int i = 0; i < query.length(); ++i) {
        int index = target.indexOfChar(lastIndex + 1, query[i]);
        if (index > lastIndex) {
            matches++;
            lastIndex = index;
        }
    }
    
    return static_cast<float>(matches) / query.length();
}

void PresetBrowser::scanDirectory(const juce::File& directory) {
    auto files = directory.findChildFiles(juce::File::findFiles, true, "*.preset;*.fxp;*.vstpreset");
    for (const auto& file : files) {
        PresetInfo info;
        info.name = file.getFileNameWithoutExtension();
        info.file = file;
        info.category = file.getParentDirectory().getFileName();
        addPreset(info);
    }
}

void PresetBrowser::rescan() {
    presets_.clear();
}

} // namespace omega
