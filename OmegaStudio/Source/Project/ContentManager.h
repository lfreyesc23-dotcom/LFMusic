/*
  ==============================================================================

    ContentManager.h
    Content pack management and marketplace integration

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace omega {

struct ContentPack {
    juce::String name;
    juce::String version;
    juce::String author;
    juce::File installPath;
    bool isInstalled = false;
    
    ContentPack() = default;
};

class ContentManager {
public:
    ContentManager();
    
    bool installPack(const juce::File& packFile);
    bool uninstallPack(const juce::String& packName);
    bool updatePack(const juce::String& packName);
    
    std::vector<ContentPack> getInstalledPacks() const;
    std::vector<ContentPack> getAvailablePacks() const;
    
    bool checkForUpdates();
    
private:
    std::vector<ContentPack> installedPacks_;
};

} // namespace omega
