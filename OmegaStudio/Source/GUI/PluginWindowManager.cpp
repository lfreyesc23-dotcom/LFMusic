/*
  ==============================================================================
    PluginWindowManager.cpp
  ==============================================================================
*/

#include "PluginWindowManager.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
PluginEditorWindow::PluginEditorWindow(std::shared_ptr<PluginInstance> plugin)
    : DocumentWindow(plugin->getName(), juce::Colour(0xff2a2a2a), DocumentWindow::allButtons),
      plugin_(plugin) {
    
    if (plugin->hasEditor()) {
        setContentOwned(plugin->createEditor(), true);
    }
    
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(800, 600);
    setVisible(true);
}

PluginEditorWindow::~PluginEditorWindow() {
}

void PluginEditorWindow::closeButtonPressed() {
    setVisible(false);
}

//==============================================================================
PluginWindowManager& PluginWindowManager::getInstance() {
    static PluginWindowManager instance;
    return instance;
}

void PluginWindowManager::openPluginEditor(std::shared_ptr<PluginInstance> plugin) {
    if (!plugin || isEditorOpen(plugin)) return;
    
    auto window = std::make_unique<PluginEditorWindow>(plugin);
    windows_[plugin.get()] = std::move(window);
}

void PluginWindowManager::closePluginEditor(std::shared_ptr<PluginInstance> plugin) {
    if (!plugin) return;
    windows_.erase(plugin.get());
}

void PluginWindowManager::closeAllEditors() {
    windows_.clear();
}

bool PluginWindowManager::isEditorOpen(std::shared_ptr<PluginInstance> plugin) const {
    return plugin && windows_.find(plugin.get()) != windows_.end();
}

PluginEditorWindow* PluginWindowManager::getEditorWindow(std::shared_ptr<PluginInstance> plugin) {
    if (!plugin) return nullptr;
    
    auto it = windows_.find(plugin.get());
    return (it != windows_.end()) ? it->second.get() : nullptr;
}

} // namespace GUI
} // namespace OmegaStudio
