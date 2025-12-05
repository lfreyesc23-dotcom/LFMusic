/*
  ==============================================================================

    KeyboardShortcuts.cpp

  ==============================================================================
*/

#include "KeyboardShortcuts.h"

namespace omega {

KeyboardShortcutManager::KeyboardShortcutManager() {
    initializeDefaultMappings();
}

void KeyboardShortcutManager::initializeDefaultMappings() {
    // Transport
    mappings_["play"] = KeyMapping("play", "Play/Pause", juce::KeyPress::spaceKey, "Transport");
    mappings_["stop"] = KeyMapping("stop", "Stop", '0', "Transport");
    mappings_["record"] = KeyMapping("record", "Record", '*', "Transport");
    
    // Edit
    mappings_["undo"] = KeyMapping("undo", "Undo", 'z', juce::ModifierKeys::commandModifier, "Edit");
    mappings_["redo"] = KeyMapping("redo", "Redo", 'z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, "Edit");
    mappings_["cut"] = KeyMapping("cut", "Cut", 'x', juce::ModifierKeys::commandModifier, "Edit");
    mappings_["copy"] = KeyMapping("copy", "Copy", 'c', juce::ModifierKeys::commandModifier, "Edit");
    mappings_["paste"] = KeyMapping("paste", "Paste", 'v', juce::ModifierKeys::commandModifier, "Edit");
    
    // Zoom
    mappings_["zoom_in"] = KeyMapping("zoom_in", "Zoom In", '=', juce::ModifierKeys::commandModifier, "View");
    mappings_["zoom_out"] = KeyMapping("zoom_out", "Zoom Out", '-', juce::ModifierKeys::commandModifier, "View");
}

void KeyboardShortcutManager::setKeyMapping(const juce::String& actionId, const juce::KeyPress& key) {
    if (mappings_.find(actionId) != mappings_.end()) {
        mappings_[actionId].keyPress = key;
    }
}

juce::KeyPress KeyboardShortcutManager::getKeyMapping(const juce::String& actionId) const {
    auto it = mappings_.find(actionId);
    return it != mappings_.end() ? it->second.keyPress : juce::KeyPress();
}

bool KeyboardShortcutManager::hasConflict(const juce::KeyPress& key, juce::String& conflictingAction) const {
    for (const auto& pair : mappings_) {
        if (pair.second.keyPress == key) {
            conflictingAction = pair.second.actionName;
            return true;
        }
    }
    return false;
}

bool KeyboardShortcutManager::executeAction(const juce::String& actionId) {
    auto it = callbacks_.find(actionId);
    if (it != callbacks_.end()) {
        it->second();
        return true;
    }
    return false;
}

bool KeyboardShortcutManager::handleKeyPress(const juce::KeyPress& key) {
    for (const auto& pair : mappings_) {
        if (pair.second.keyPress == key) {
            return executeAction(pair.first);
        }
    }
    return false;
}

void KeyboardShortcutManager::loadPreset(const juce::String& presetName) {
    if (presetName == "ProTools") loadProToolsPreset();
    else if (presetName == "Logic") loadLogicPreset();
    else if (presetName == "Ableton") loadAbletonPreset();
    else if (presetName == "FLStudio") loadFLStudioPreset();
}

void KeyboardShortcutManager::loadProToolsPreset() {
    mappings_["play"] = KeyMapping("play", "Play", juce::KeyPress::spaceKey, "Transport");
    mappings_["record"] = KeyMapping("record", "Record", '3', juce::ModifierKeys::commandModifier, "Transport");
    mappings_["split"] = KeyMapping("split", "Split", 'e', juce::ModifierKeys::commandModifier, "Edit");
}

void KeyboardShortcutManager::loadLogicPreset() {
    mappings_["play"] = KeyMapping("play", "Play", juce::KeyPress::spaceKey, "Transport");
    mappings_["record"] = KeyMapping("record", "Record", 'r', "Transport");
    mappings_["cycle"] = KeyMapping("cycle", "Cycle", 'c', "Transport");
}

void KeyboardShortcutManager::loadAbletonPreset() {
    mappings_["play"] = KeyMapping("play", "Play", juce::KeyPress::spaceKey, "Transport");
    mappings_["record"] = KeyMapping("record", "Record", juce::KeyPress::F9Key, "Transport");
    mappings_["tap_tempo"] = KeyMapping("tap_tempo", "Tap Tempo", 't', juce::ModifierKeys::commandModifier, "Transport");
}

void KeyboardShortcutManager::loadFLStudioPreset() {
    mappings_["play"] = KeyMapping("play", "Play", juce::KeyPress::spaceKey, "Transport");
    mappings_["record"] = KeyMapping("record", "Record", juce::KeyPress::F9Key, "Transport");
    mappings_["pattern"] = KeyMapping("pattern", "Pattern Mode", juce::KeyPress::F4Key, "View");
}

void KeyboardShortcutManager::registerAction(const juce::String& id, const juce::String& name, 
                                            const juce::String& category, std::function<void()> callback) {
    if (mappings_.find(id) == mappings_.end()) {
        mappings_[id] = KeyMapping(id, name, juce::KeyPress(), category);
    }
    callbacks_[id] = callback;
}

std::vector<KeyMapping> KeyboardShortcutManager::getAllMappings() const {
    std::vector<KeyMapping> result;
    for (const auto& pair : mappings_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<juce::String> KeyboardShortcutManager::getCategories() const {
    std::vector<juce::String> cats;
    for (const auto& pair : mappings_) {
        if (std::find(cats.begin(), cats.end(), pair.second.category) == cats.end()) {
            cats.push_back(pair.second.category);
        }
    }
    return cats;
}

} // namespace omega
