//==============================================================================
// ContextMenuSystem.h
// FL Studio 2025 Universal Context Menu System
// Right-click menu profesional para todos los elementos
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <functional>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Context Menu Item - Item de menú con callbacks
//==============================================================================
struct ContextMenuItem {
    juce::String text;
    juce::String icon;
    juce::String shortcut;
    bool enabled = true;
    bool checked = false;
    bool separator = false;
    std::function<void()> callback;
    std::vector<ContextMenuItem> subItems;
    
    ContextMenuItem() = default;
    
    ContextMenuItem(const juce::String& itemText, 
                   std::function<void()> itemCallback = nullptr,
                   const juce::String& itemIcon = "",
                   const juce::String& itemShortcut = "")
        : text(itemText), icon(itemIcon), shortcut(itemShortcut), callback(itemCallback) {}
    
    static ContextMenuItem separator() {
        ContextMenuItem item;
        item.separator = true;
        return item;
    }
};

//==============================================================================
// Context Menu Builder - Constructor fluent de menús
//==============================================================================
class ContextMenuBuilder {
public:
    ContextMenuBuilder() = default;
    
    ContextMenuBuilder& addItem(const juce::String& text,
                               std::function<void()> callback,
                               const juce::String& icon = "",
                               const juce::String& shortcut = "") {
        items.push_back(ContextMenuItem(text, callback, icon, shortcut));
        return *this;
    }
    
    ContextMenuBuilder& addSeparator() {
        items.push_back(ContextMenuItem::separator());
        return *this;
    }
    
    ContextMenuBuilder& addSubMenu(const juce::String& text,
                                  const std::vector<ContextMenuItem>& subItems,
                                  const juce::String& icon = "") {
        ContextMenuItem item(text, nullptr, icon);
        item.subItems = subItems;
        items.push_back(item);
        return *this;
    }
    
    ContextMenuBuilder& addCheckItem(const juce::String& text,
                                    bool checked,
                                    std::function<void()> callback,
                                    const juce::String& icon = "") {
        ContextMenuItem item(text, callback, icon);
        item.checked = checked;
        items.push_back(item);
        return *this;
    }
    
    ContextMenuBuilder& addDisabledItem(const juce::String& text,
                                       const juce::String& icon = "") {
        ContextMenuItem item(text, nullptr, icon);
        item.enabled = false;
        items.push_back(item);
        return *this;
    }
    
    std::vector<ContextMenuItem> build() const {
        return items;
    }
    
private:
    std::vector<ContextMenuItem> items;
};

//==============================================================================
// Universal Context Menu - Menú contextual universal
//==============================================================================
class UniversalContextMenu {
public:
    static void show(const std::vector<ContextMenuItem>& items,
                    juce::Component* targetComponent,
                    int x = -1, int y = -1) {
        juce::PopupMenu menu;
        buildMenu(menu, items);
        
        juce::PopupMenu::Options options;
        if (targetComponent) {
            options = options.withTargetComponent(targetComponent);
        }
        
        if (x >= 0 && y >= 0) {
            options = options.withTargetScreenArea(juce::Rectangle<int>(x, y, 1, 1));
        }
        
        menu.showMenuAsync(options);
    }
    
    // Menú contextual para CHANNEL RACK
    static std::vector<ContextMenuItem> createChannelRackMenu() {
        return ContextMenuBuilder()
            .addItem("Insert Channel", nullptr, "➕", "Ctrl+I")
            .addItem("Delete Channel", nullptr, "🗑️", "Delete")
            .addSeparator()
            .addItem("Rename Channel", nullptr, "✏️", "F2")
            .addItem("Change Color", nullptr, "🎨")
            .addSeparator()
            .addItem("Open in New Window", nullptr, "🗔", "Ctrl+Shift+O")
            .addItem("Clone Channel", nullptr, "📋")
            .addSeparator()
            .addSubMenu("Route to Mixer", {
                ContextMenuItem("Insert 1", nullptr),
                ContextMenuItem("Insert 2", nullptr),
                ContextMenuItem("Master", nullptr)
            }, "🎚️")
            .addSeparator()
            .addItem("Automation", nullptr, "⚡", "Ctrl+A")
            .addItem("Piano Roll", nullptr, "🎹", "F7")
            .build();
    }
    
    // Menú contextual para MIXER
    static std::vector<ContextMenuItem> createMixerMenu() {
        return ContextMenuBuilder()
            .addItem("Insert Effect", nullptr, "🔌", "Ctrl+E")
            .addItem("Save Mixer Preset", nullptr, "💾", "Ctrl+S")
            .addSeparator()
            .addItem("Enable Sidechain", nullptr, "🔗")
            .addItem("Record Automation", nullptr, "⏺️", "R")
            .addSeparator()
            .addSubMenu("Send to", {
                ContextMenuItem("Send 1", nullptr),
                ContextMenuItem("Send 2", nullptr),
                ContextMenuItem("Send 3", nullptr)
            }, "📤")
            .addSeparator()
            .addItem("Save Snapshot", nullptr, "📸")
            .addItem("Link to Controller", nullptr, "🎛️", "Ctrl+L")
            .addSeparator()
            .addCheckItem("Show EQ", true, nullptr, "📊")
            .addCheckItem("Show Sends", false, nullptr, "📡")
            .build();
    }
    
    // Menú contextual para PIANO ROLL
    static std::vector<ContextMenuItem> createPianoRollMenu() {
        return ContextMenuBuilder()
            .addItem("Quantize", nullptr, "📐", "Ctrl+Q")
            .addItem("Humanize", nullptr, "👤", "Alt+H")
            .addSeparator()
            .addSubMenu("Chord Stamps", {
                ContextMenuItem("Major", nullptr, "🎵"),
                ContextMenuItem("Minor", nullptr, "🎵"),
                ContextMenuItem("7th", nullptr, "🎵"),
                ContextMenuItem("Sus", nullptr, "🎵")
            }, "🎼")
            .addSeparator()
            .addItem("Riff Machine", nullptr, "✨", "Ctrl+R")
            .addItem("Arpeggiate", nullptr, "🎶", "Alt+A")
            .addSeparator()
            .addSubMenu("Scale", {
                ContextMenuItem("C Major", nullptr),
                ContextMenuItem("A Minor", nullptr),
                ContextMenuItem("E Minor", nullptr),
                ContextMenuItem("Custom...", nullptr)
            }, "🎹")
            .addSeparator()
            .addItem("Stretch Notes", nullptr, "↔️")
            .addItem("Randomize Velocity", nullptr, "🎲")
            .build();
    }
    
    // Menú contextual para PLAYLIST
    static std::vector<ContextMenuItem> createPlaylistMenu() {
        return ContextMenuBuilder()
            .addItem("Insert Time Marker", nullptr, "📍", "Ctrl+M")
            .addItem("Create Region", nullptr, "📦", "Ctrl+R")
            .addSeparator()
            .addItem("Split at Playhead", nullptr, "✂️", "Ctrl+K")
            .addItem("Merge Clips", nullptr, "🔗", "Ctrl+J")
            .addSeparator()
            .addCheckItem("Show Ghost Clips", false, nullptr, "👻")
            .addCheckItem("Snap to Grid", true, nullptr, "🧲", "S")
            .addSeparator()
            .addSubMenu("Grid Size", {
                ContextMenuItem("1/4 bar", nullptr),
                ContextMenuItem("1/8 bar", nullptr),
                ContextMenuItem("1/16 bar", nullptr),
                ContextMenuItem("1/32 bar", nullptr)
            }, "⊞")
            .addSeparator()
            .addItem("Render to Audio", nullptr, "🎵", "Ctrl+Alt+R")
            .build();
    }
    
    // Menú contextual para BROWSER
    static std::vector<ContextMenuItem> createBrowserMenu() {
        return ContextMenuBuilder()
            .addItem("Add to Favorites", nullptr, "⭐", "Ctrl+D")
            .addItem("Rate Sample", nullptr, "🌟")
            .addSeparator()
            .addItem("Edit Tags", nullptr, "🏷️", "Ctrl+T")
            .addItem("Show in Finder", nullptr, "📁", "Ctrl+Shift+F")
            .addSeparator()
            .addSubMenu("View Mode", {
                ContextMenuItem("List View", nullptr, "☰"),
                ContextMenuItem("Grid View", nullptr, "⊞"),
                ContextMenuItem("Icon View", nullptr, "🖼️")
            }, "👁️")
            .addSeparator()
            .addItem("Refresh Library", nullptr, "🔄", "F5")
            .addItem("Export Pack", nullptr, "📦")
            .build();
    }
    
    // Menú contextual GENÉRICO para cualquier componente
    static std::vector<ContextMenuItem> createGenericMenu() {
        return ContextMenuBuilder()
            .addItem("Open in New Window", nullptr, "🗔")
            .addItem("Detach Window", nullptr, "⇱")
            .addSeparator()
            .addItem("Link to Controller", nullptr, "🎛️", "Ctrl+L")
            .addItem("Automate", nullptr, "⚡", "Ctrl+A")
            .addSeparator()
            .addItem("Copy", nullptr, "📋", "Ctrl+C")
            .addItem("Paste", nullptr, "📄", "Ctrl+V")
            .addSeparator()
            .addItem("Settings", nullptr, "⚙️")
            .addItem("Help", nullptr, "❓", "F1")
            .build();
    }
    
private:
    static void buildMenu(juce::PopupMenu& menu, const std::vector<ContextMenuItem>& items) {
        int itemId = 1;
        
        for (const auto& item : items) {
            if (item.separator) {
                menu.addSeparator();
            }
            else if (!item.subItems.empty()) {
                juce::PopupMenu subMenu;
                buildMenu(subMenu, item.subItems);
                
                juce::String fullText = item.icon.isEmpty() 
                    ? item.text 
                    : item.icon + " " + item.text;
                    
                menu.addSubMenu(fullText, subMenu, item.enabled);
            }
            else {
                juce::String fullText = item.icon.isEmpty() 
                    ? item.text 
                    : item.icon + " " + item.text;
                
                if (!item.shortcut.isEmpty()) {
                    fullText += "  [" + item.shortcut + "]";
                }
                
                menu.addItem(itemId++, fullText, item.enabled, item.checked, 
                           [callback = item.callback] {
                    if (callback) callback();
                });
            }
        }
    }
};

//==============================================================================
// Context Menu Component - Componente con menú contextual integrado
//==============================================================================
class ContextMenuComponent : public juce::Component {
public:
    ContextMenuComponent() = default;
    
    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isPopupMenu()) {
            showContextMenu(event.x, event.y);
        }
    }
    
    virtual void showContextMenu(int x, int y) {
        if (contextMenuProvider) {
            auto items = contextMenuProvider();
            UniversalContextMenu::show(items, this, x, y);
        }
    }
    
    std::function<std::vector<ContextMenuItem>()> contextMenuProvider;
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContextMenuComponent)
};

} // namespace GUI
} // namespace OmegaStudio
