/*
  ==============================================================================
    PluginWindowManager.h - Sistema de ventanas para plugins VST/AU
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Audio/Plugins/PluginManager.h"
#include <map>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
class PluginEditorWindow : public juce::DocumentWindow {
public:
    PluginEditorWindow(std::shared_ptr<PluginInstance> plugin);
    ~PluginEditorWindow() override;
    
    void closeButtonPressed() override;
    
    std::shared_ptr<PluginInstance> getPlugin() const { return plugin_; }
    
private:
    std::shared_ptr<PluginInstance> plugin_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

//==============================================================================
class PluginWindowManager {
public:
    static PluginWindowManager& getInstance();
    
    void openPluginEditor(std::shared_ptr<PluginInstance> plugin);
    void closePluginEditor(std::shared_ptr<PluginInstance> plugin);
    void closeAllEditors();
    
    bool isEditorOpen(std::shared_ptr<PluginInstance> plugin) const;
    PluginEditorWindow* getEditorWindow(std::shared_ptr<PluginInstance> plugin);
    
private:
    PluginWindowManager() = default;
    
    std::map<PluginInstance*, std::unique_ptr<PluginEditorWindow>> windows_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindowManager)
};

} // namespace GUI
} // namespace OmegaStudio
