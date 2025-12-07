//==============================================================================
// DockingSystem.h
// FL Studio 2025 Professional Docking & Multi-Window System
// Ventanas desprendibles, multi-monitor, always-on-top
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <map>
#include <memory>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Detachable Window - Ventana flotante desprendible
//==============================================================================
class DetachableWindow : public juce::DocumentWindow {
public:
    DetachableWindow(const juce::String& name, juce::Component* content, bool deleteOnClose = false)
        : juce::DocumentWindow(name, juce::Colour(0xff2b2b2b), 
                              juce::DocumentWindow::allButtons),
          contentComponent(content),
          shouldDeleteOnClose(deleteOnClose)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        
        if (content) {
            setContentOwned(content, true);
        }
        
        // Always on top toggle
        setAlwaysOnTop(false);
        
        // Multi-monitor aware positioning
        centreWithSize(getWidth(), getHeight());
    }
    
    void closeButtonPressed() override {
        if (onCloseCallback) {
            onCloseCallback();
        }
        
        if (shouldDeleteOnClose) {
            delete this;
        } else {
            setVisible(false);
        }
    }
    
    void setAlwaysOnTopEnabled(bool enable) {
        setAlwaysOnTop(enable);
        alwaysOnTopEnabled = enable;
    }
    
    bool isAlwaysOnTopEnabled() const { return alwaysOnTopEnabled; }
    
    std::function<void()> onCloseCallback;
    
private:
    juce::Component* contentComponent;
    bool shouldDeleteOnClose;
    bool alwaysOnTopEnabled = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DetachableWindow)
};

//==============================================================================
// Dockable Panel - Panel que puede docked o detached
//==============================================================================
class DockablePanel : public juce::Component {
public:
    enum class DockPosition {
        Left, Right, Top, Bottom, Floating, Hidden
    };
    
    DockablePanel(const juce::String& panelName) : name(panelName) {
        // Header para drag & detach
        addAndMakeVisible(header);
        header.setText(name, juce::dontSendNotification);
        header.setColour(juce::Label::backgroundColourId, juce::Colour(0xff3a3a3a));
        header.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
        header.setJustificationType(juce::Justification::centred);
        
        // Detach button
        detachButton.setButtonText("⇱");
        detachButton.setTooltip("Detach window (make floating)");
        detachButton.onClick = [this] { detachPanel(); };
        addAndMakeVisible(detachButton);
        
        // Close button
        closeButton.setButtonText("✕");
        closeButton.setTooltip("Hide panel");
        closeButton.onClick = [this] { hidePanel(); };
        addAndMakeVisible(closeButton);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2b2b2b));
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRect(getLocalBounds(), 1);
    }
    
    void resized() override {
        auto area = getLocalBounds();
        auto headerArea = area.removeFromTop(28);
        
        closeButton.setBounds(headerArea.removeFromRight(28).reduced(4));
        detachButton.setBounds(headerArea.removeFromRight(28).reduced(4));
        header.setBounds(headerArea);
        
        if (contentComponent) {
            contentComponent->setBounds(area);
        }
    }
    
    void setContent(juce::Component* content) {
        if (contentComponent) {
            removeChildComponent(contentComponent);
        }
        contentComponent = content;
        if (content) {
            addAndMakeVisible(content);
            resized();
        }
    }
    
    juce::Component* getContent() const { return contentComponent; }
    
    void setDockPosition(DockPosition pos) {
        position = pos;
        if (onDockPositionChanged) {
            onDockPositionChanged(position);
        }
    }
    
    DockPosition getDockPosition() const { return position; }
    
    void detachPanel() {
        if (!floatingWindow) {
            auto* content = contentComponent;
            removeChildComponent(content);
            
            floatingWindow = std::make_unique<DetachableWindow>(name, content, false);
            floatingWindow->setSize(400, 500);
            floatingWindow->setVisible(true);
            floatingWindow->onCloseCallback = [this] { 
                reattachPanel(); 
            };
            
            position = DockPosition::Floating;
            setVisible(false);
            
            if (onDetached) {
                onDetached();
            }
        }
    }
    
    void reattachPanel() {
        if (floatingWindow) {
            auto* content = floatingWindow->getContentComponent();
            floatingWindow->clearContentComponent();
            floatingWindow.reset();
            
            setContent(content);
            setVisible(true);
            position = DockPosition::Right; // Default position
            
            if (onReattached) {
                onReattached();
            }
        }
    }
    
    void hidePanel() {
        position = DockPosition::Hidden;
        setVisible(false);
        if (onHidden) {
            onHidden();
        }
    }
    
    bool isDetached() const { return floatingWindow != nullptr; }
    
    std::function<void(DockPosition)> onDockPositionChanged;
    std::function<void()> onDetached;
    std::function<void()> onReattached;
    std::function<void()> onHidden;
    
private:
    juce::String name;
    juce::Label header;
    juce::TextButton detachButton;
    juce::TextButton closeButton;
    juce::Component* contentComponent = nullptr;
    DockPosition position = DockPosition::Right;
    std::unique_ptr<DetachableWindow> floatingWindow;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockablePanel)
};

//==============================================================================
// Docking Manager - Gestor de ventanas y paneles
//==============================================================================
class DockingManager {
public:
    DockingManager() = default;
    
    void registerPanel(const juce::String& id, DockablePanel* panel) {
        panels[id] = panel;
        
        panel->onDetached = [this, id] {
            if (onPanelDetached) {
                onPanelDetached(id);
            }
        };
        
        panel->onReattached = [this, id] {
            if (onPanelReattached) {
                onPanelReattached(id);
            }
        };
    }
    
    void unregisterPanel(const juce::String& id) {
        panels.erase(id);
    }
    
    DockablePanel* getPanel(const juce::String& id) {
        auto it = panels.find(id);
        return it != panels.end() ? it->second : nullptr;
    }
    
    void detachPanel(const juce::String& id) {
        if (auto* panel = getPanel(id)) {
            panel->detachPanel();
        }
    }
    
    void reattachPanel(const juce::String& id) {
        if (auto* panel = getPanel(id)) {
            panel->reattachPanel();
        }
    }
    
    void hidePanel(const juce::String& id) {
        if (auto* panel = getPanel(id)) {
            panel->hidePanel();
        }
    }
    
    void showPanel(const juce::String& id, DockablePanel::DockPosition position) {
        if (auto* panel = getPanel(id)) {
            panel->setDockPosition(position);
            panel->setVisible(true);
        }
    }
    
    juce::StringArray getRegisteredPanels() const {
        juce::StringArray ids;
        for (const auto& pair : panels) {
            ids.add(pair.first);
        }
        return ids;
    }
    
    std::function<void(const juce::String&)> onPanelDetached;
    std::function<void(const juce::String&)> onPanelReattached;
    
private:
    std::map<juce::String, DockablePanel*> panels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockingManager)
};

} // namespace GUI
} // namespace OmegaStudio
