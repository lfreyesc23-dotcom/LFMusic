/*
  ==============================================================================

    KeyboardShortcuts.h
    Customizable keyboard shortcuts system with DAW preset imports

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <map>
#include <vector>

namespace omega {

struct KeyMapping {
    juce::String actionId;
    juce::String actionName;
    juce::KeyPress keyPress;
    juce::String category;
    
    KeyMapping() = default;
    KeyMapping(const juce::String& id, const juce::String& name, 
               int keyCode, const juce::String& cat)
        : actionId(id), actionName(name), keyPress(keyCode), category(cat) {}
    KeyMapping(const juce::String& id, const juce::String& name, 
               int keyCode, juce::ModifierKeys mods, const juce::String& cat)
        : actionId(id), actionName(name), keyPress(keyCode, mods, 0), category(cat) {}
    KeyMapping(const juce::String& id, const juce::String& name, 
               const juce::KeyPress& key, const juce::String& cat)
        : actionId(id), actionName(name), keyPress(key), category(cat) {}
};

class KeyboardShortcutManager {
public:
    KeyboardShortcutManager();
    
    // Mapping management
    void setKeyMapping(const juce::String& actionId, const juce::KeyPress& key);
    juce::KeyPress getKeyMapping(const juce::String& actionId) const;
    bool hasConflict(const juce::KeyPress& key, juce::String& conflictingAction) const;
    
    // Action execution
    bool executeAction(const juce::String& actionId);
    bool handleKeyPress(const juce::KeyPress& key);
    
    // Presets
    void loadPreset(const juce::String& presetName); // "ProTools", "Logic", "Ableton", "FLStudio"
    void saveCustomPreset(const juce::String& name);
    void loadCustomPreset(const juce::File& file);
    
    // Get all mappings
    std::vector<KeyMapping> getAllMappings() const;
    std::vector<juce::String> getCategories() const;
    
    // Action registration
    void registerAction(const juce::String& id, const juce::String& name, 
                       const juce::String& category, std::function<void()> callback);
    
private:
    void initializeDefaultMappings();
    void loadProToolsPreset();
    void loadLogicPreset();
    void loadAbletonPreset();
    void loadFLStudioPreset();
    
    std::map<juce::String, KeyMapping> mappings_;
    std::map<juce::String, std::function<void()>> callbacks_;
};

} // namespace omega
