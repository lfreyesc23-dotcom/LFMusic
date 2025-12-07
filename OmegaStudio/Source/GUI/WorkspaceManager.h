//==============================================================================
// WorkspaceManager.h
// FL Studio 2025 Workspace/Layout System
// Guarda/carga layouts completos con shortcuts F5-F12
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "DockingSystem.h"
#include <map>
#include <vector>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Window State - Estado de una ventana individual
//==============================================================================
struct WindowState {
    juce::String windowId;
    bool visible = true;
    bool detached = false;
    DockablePanel::DockPosition dockPosition = DockablePanel::DockPosition::Right;
    juce::Rectangle<int> bounds;
    bool alwaysOnTop = false;
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("windowId", windowId);
        obj->setProperty("visible", visible);
        obj->setProperty("detached", detached);
        obj->setProperty("dockPosition", (int)dockPosition);
        obj->setProperty("x", bounds.getX());
        obj->setProperty("y", bounds.getY());
        obj->setProperty("width", bounds.getWidth());
        obj->setProperty("height", bounds.getHeight());
        obj->setProperty("alwaysOnTop", alwaysOnTop);
        return juce::var(obj);
    }
    
    static WindowState fromVar(const juce::var& v) {
        WindowState state;
        if (auto* obj = v.getDynamicObject()) {
            state.windowId = obj->getProperty("windowId").toString();
            state.visible = obj->getProperty("visible");
            state.detached = obj->getProperty("detached");
            state.dockPosition = (DockablePanel::DockPosition)(int)obj->getProperty("dockPosition");
            state.bounds = juce::Rectangle<int>(
                obj->getProperty("x"),
                obj->getProperty("y"),
                obj->getProperty("width"),
                obj->getProperty("height")
            );
            state.alwaysOnTop = obj->getProperty("alwaysOnTop");
        }
        return state;
    }
};

//==============================================================================
// Workspace Layout - Layout completo guardable
//==============================================================================
struct WorkspaceLayout {
    juce::String name;
    juce::String description;
    juce::String shortcutKey; // F5-F12
    std::vector<WindowState> windows;
    juce::Colour accentColour = juce::Colour(0xffff8736);
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("description", description);
        obj->setProperty("shortcutKey", shortcutKey);
        obj->setProperty("accentColour", accentColour.toString());
        
        juce::Array<juce::var> windowsArray;
        for (const auto& window : windows) {
            windowsArray.add(window.toVar());
        }
        obj->setProperty("windows", windowsArray);
        
        return juce::var(obj);
    }
    
    static WorkspaceLayout fromVar(const juce::var& v) {
        WorkspaceLayout layout;
        if (auto* obj = v.getDynamicObject()) {
            layout.name = obj->getProperty("name").toString();
            layout.description = obj->getProperty("description").toString();
            layout.shortcutKey = obj->getProperty("shortcutKey").toString();
            layout.accentColour = juce::Colour::fromString(obj->getProperty("accentColour").toString());
            
            if (auto* windowsArray = obj->getProperty("windows").getArray()) {
                for (const auto& windowVar : *windowsArray) {
                    layout.windows.push_back(WindowState::fromVar(windowVar));
                }
            }
        }
        return layout;
    }
};

//==============================================================================
// Workspace Manager - Gestor de layouts/workspaces
//==============================================================================
class WorkspaceManager : public juce::KeyListener {
public:
    WorkspaceManager() {
        // Workspaces predefinidos profesionales
        createDefaultWorkspaces();
    }
    
    // Crear workspace desde estado actual
    WorkspaceLayout captureCurrentWorkspace(const juce::String& name, 
                                           const juce::String& shortcutKey = "") {
        WorkspaceLayout layout;
        layout.name = name;
        layout.description = "Custom workspace";
        layout.shortcutKey = shortcutKey;
        
        // Capturar estado de todos los paneles registrados
        for (const auto& pair : registeredPanels) {
            WindowState state;
            state.windowId = pair.first;
            
            auto* panel = pair.second;
            state.visible = panel->isVisible();
            state.detached = panel->isDetached();
            state.dockPosition = panel->getDockPosition();
            state.bounds = panel->getBounds();
            
            layout.windows.push_back(state);
        }
        
        return layout;
    }
    
    // Aplicar workspace
    void applyWorkspace(const WorkspaceLayout& layout) {
        currentLayout = layout;
        
        for (const auto& windowState : layout.windows) {
            if (auto* panel = registeredPanels[windowState.windowId]) {
                panel->setVisible(windowState.visible);
                
                if (windowState.detached) {
                    panel->detachPanel();
                } else {
                    panel->reattachPanel();
                    panel->setDockPosition(windowState.dockPosition);
                }
                
                if (!windowState.bounds.isEmpty()) {
                    panel->setBounds(windowState.bounds);
                }
            }
        }
        
        if (onWorkspaceChanged) {
            onWorkspaceChanged(layout);
        }
    }
    
    // Guardar workspace
    void saveWorkspace(const juce::String& name, const WorkspaceLayout& layout) {
        savedWorkspaces[name] = layout;
        saveWorkspacesToFile();
    }
    
    // Cargar workspace por nombre
    bool loadWorkspace(const juce::String& name) {
        auto it = savedWorkspaces.find(name);
        if (it != savedWorkspaces.end()) {
            applyWorkspace(it->second);
            return true;
        }
        return false;
    }
    
    // Guardar workspace actual con nombre
    void saveCurrentWorkspace(const juce::String& name, const juce::String& shortcut = "") {
        auto layout = captureCurrentWorkspace(name, shortcut);
        saveWorkspace(name, layout);
    }
    
    // Obtener todos los workspaces
    juce::StringArray getWorkspaceNames() const {
        juce::StringArray names;
        for (const auto& pair : savedWorkspaces) {
            names.add(pair.first);
        }
        return names;
    }
    
    WorkspaceLayout getWorkspace(const juce::String& name) const {
        auto it = savedWorkspaces.find(name);
        return it != savedWorkspaces.end() ? it->second : WorkspaceLayout();
    }
    
    // Registrar panel para gesión
    void registerPanel(const juce::String& id, DockablePanel* panel) {
        registeredPanels[id] = panel;
    }
    
    // KeyListener para shortcuts F5-F12
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override {
        // F5 = Recording
        if (key.getKeyCode() == juce::KeyPress::F5Key) {
            loadWorkspace("Recording");
            return true;
        }
        // F6 = Production
        else if (key.getKeyCode() == juce::KeyPress::F6Key) {
            loadWorkspace("Production");
            return true;
        }
        // F7 = Mixing
        else if (key.getKeyCode() == juce::KeyPress::F7Key) {
            loadWorkspace("Mixing");
            return true;
        }
        // F8 = Mastering
        else if (key.getKeyCode() == juce::KeyPress::F8Key) {
            loadWorkspace("Mastering");
            return true;
        }
        // F9-F12 para custom workspaces
        else if (key.getKeyCode() >= juce::KeyPress::F9Key && 
                 key.getKeyCode() <= juce::KeyPress::F12Key) {
            int index = key.getKeyCode() - juce::KeyPress::F9Key + 5;
            auto names = getWorkspaceNames();
            if (index < names.size()) {
                loadWorkspace(names[index]);
            }
            return true;
        }
        
        return false;
    }
    
    WorkspaceLayout getCurrentLayout() const { return currentLayout; }
    
    std::function<void(const WorkspaceLayout&)> onWorkspaceChanged;
    
private:
    std::map<juce::String, DockablePanel*> registeredPanels;
    std::map<juce::String, WorkspaceLayout> savedWorkspaces;
    WorkspaceLayout currentLayout;
    
    void createDefaultWorkspaces() {
        // RECORDING workspace (F5)
        {
            WorkspaceLayout recording;
            recording.name = "Recording";
            recording.description = "Optimized for audio recording";
            recording.shortcutKey = "F5";
            recording.accentColour = juce::Colour(0xffff3636);
            
            WindowState mixer;
            mixer.windowId = "mixer";
            mixer.visible = true;
            mixer.dockPosition = DockablePanel::DockPosition::Bottom;
            recording.windows.push_back(mixer);
            
            WindowState browser;
            browser.windowId = "browser";
            browser.visible = true;
            browser.dockPosition = DockablePanel::DockPosition::Left;
            recording.windows.push_back(browser);
            
            savedWorkspaces["Recording"] = recording;
        }
        
        // PRODUCTION workspace (F6)
        {
            WorkspaceLayout production;
            production.name = "Production";
            production.description = "Full production layout";
            production.shortcutKey = "F6";
            production.accentColour = juce::Colour(0xffff8736);
            
            WindowState channelRack;
            channelRack.windowId = "channelRack";
            channelRack.visible = true;
            channelRack.dockPosition = DockablePanel::DockPosition::Right;
            production.windows.push_back(channelRack);
            
            WindowState pianoRoll;
            pianoRoll.windowId = "pianoRoll";
            pianoRoll.visible = true;
            pianoRoll.dockPosition = DockablePanel::DockPosition::Bottom;
            production.windows.push_back(pianoRoll);
            
            WindowState browser;
            browser.windowId = "browser";
            browser.visible = true;
            browser.dockPosition = DockablePanel::DockPosition::Left;
            production.windows.push_back(browser);
            
            savedWorkspaces["Production"] = production;
        }
        
        // MIXING workspace (F7)
        {
            WorkspaceLayout mixing;
            mixing.name = "Mixing";
            mixing.description = "Focus on mixing";
            mixing.shortcutKey = "F7";
            mixing.accentColour = juce::Colour(0xff36a9ff);
            
            WindowState mixer;
            mixer.windowId = "mixer";
            mixer.visible = true;
            mixer.detached = true;
            mixer.bounds = juce::Rectangle<int>(100, 100, 1200, 700);
            mixing.windows.push_back(mixer);
            
            WindowState playlist;
            playlist.windowId = "playlist";
            playlist.visible = true;
            playlist.dockPosition = DockablePanel::DockPosition::Bottom;
            mixing.windows.push_back(playlist);
            
            savedWorkspaces["Mixing"] = mixing;
        }
        
        // MASTERING workspace (F8)
        {
            WorkspaceLayout mastering;
            mastering.name = "Mastering";
            mastering.description = "Final mastering layout";
            mastering.shortcutKey = "F8";
            mastering.accentColour = juce::Colour(0xff9d36ff);
            
            WindowState mixer;
            mixer.windowId = "mixer";
            mixer.visible = true;
            mixer.dockPosition = DockablePanel::DockPosition::Right;
            mastering.windows.push_back(mixer);
            
            WindowState visualizers;
            visualizers.windowId = "visualizers";
            visualizers.visible = true;
            visualizers.dockPosition = DockablePanel::DockPosition::Bottom;
            mastering.windows.push_back(visualizers);
            
            savedWorkspaces["Mastering"] = mastering;
        }
    }
    
    void saveWorkspacesToFile() {
        juce::File settingsDir = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("OmegaStudio")
            .getChildFile("Workspaces");
        
        settingsDir.createDirectory();
        
        juce::Array<juce::var> workspacesArray;
        for (const auto& pair : savedWorkspaces) {
            workspacesArray.add(pair.second.toVar());
        }
        
        juce::var data(workspacesArray);
        juce::File file = settingsDir.getChildFile("workspaces.json");
        file.replaceWithText(juce::JSON::toString(data, true));
    }
    
    void loadWorkspacesFromFile() {
        juce::File file = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("OmegaStudio")
            .getChildFile("Workspaces")
            .getChildFile("workspaces.json");
        
        if (file.existsAsFile()) {
            auto json = juce::JSON::parse(file);
            if (auto* array = json.getArray()) {
                for (const auto& item : *array) {
                    auto layout = WorkspaceLayout::fromVar(item);
                    savedWorkspaces[layout.name] = layout;
                }
            }
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceManager)
};

} // namespace GUI
} // namespace OmegaStudio
