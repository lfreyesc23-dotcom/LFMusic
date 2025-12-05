/*
  ==============================================================================

    ContentManager.cpp

  ==============================================================================
*/

#include "ContentManager.h"

namespace omega {

ContentManager::ContentManager() {}

bool ContentManager::installPack(const juce::File& packFile) {
    if (!packFile.existsAsFile()) return false;
    
    ContentPack pack;
    pack.name = packFile.getFileNameWithoutExtension();
    pack.version = "1.0.0";
    pack.installPath = packFile;
    pack.isInstalled = true;
    
    installedPacks_.push_back(pack);
    return true;
}

bool ContentManager::uninstallPack(const juce::String& packName) {
    auto it = std::remove_if(installedPacks_.begin(), installedPacks_.end(),
        [&packName](const ContentPack& p) { return p.name == packName; });
    
    if (it != installedPacks_.end()) {
        installedPacks_.erase(it, installedPacks_.end());
        return true;
    }
    
    return false;
}

bool ContentManager::updatePack(const juce::String& /*packName*/) {
    return true; // Placeholder
}

std::vector<ContentPack> ContentManager::getInstalledPacks() const {
    return installedPacks_;
}

std::vector<ContentPack> ContentManager::getAvailablePacks() const {
    return std::vector<ContentPack>(); // Placeholder
}

bool ContentManager::checkForUpdates() {
    return false; // Placeholder
}

} // namespace omega
