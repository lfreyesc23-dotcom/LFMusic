//==============================================================================
// QuickAccessToolbar.h
// FL Studio 2025 Quick Access Toolbar
// Barra personalizable con drag & drop de funciones favoritas
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <functional>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Tool Button - Botón de herramienta personalizable
//==============================================================================
class ToolButton : public juce::Button {
public:
    ToolButton(const juce::String& name, const juce::String& icon)
        : juce::Button(name), iconText(icon) {
        setTooltip(name);
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                    bool shouldDrawButtonAsDown) override {
        auto bounds = getLocalBounds().toFloat().reduced(2);
        
        // Background
        auto baseColour = juce::Colour(0xff3a3a3a);
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.3f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.2f);
        
        if (getToggleState()) {
            baseColour = juce::Colour(0xffff8736);
        }
        
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(baseColour.darker(0.3f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        // Icon
        g.setColour(getToggleState() ? juce::Colours::white : juce::Colour(0xffdddddd));
        g.setFont(juce::Font(16.0f));
        g.drawText(iconText, bounds, juce::Justification::centred);
        
        // Shine effect
        if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            auto shineBounds = bounds.removeFromTop(bounds.getHeight() * 0.5f);
            g.fillRoundedRectangle(shineBounds, 4.0f);
        }
    }
    
    void setIcon(const juce::String& icon) {
        iconText = icon;
        repaint();
    }
    
    juce::String getIcon() const { return iconText; }
    
private:
    juce::String iconText;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolButton)
};

//==============================================================================
// Tool Item - Item de herramienta con metadata
//==============================================================================
struct ToolItem {
    juce::String id;
    juce::String name;
    juce::String icon;
    juce::String category;
    std::function<void()> action;
    bool toggleable = false;
    
    ToolItem() = default;
    
    ToolItem(const juce::String& itemId,
            const juce::String& itemName,
            const juce::String& itemIcon,
            std::function<void()> itemAction,
            const juce::String& itemCategory = "General",
            bool isToggleable = false)
        : id(itemId), name(itemName), icon(itemIcon), 
          category(itemCategory), action(itemAction), toggleable(isToggleable) {}
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", id);
        obj->setProperty("name", name);
        obj->setProperty("icon", icon);
        obj->setProperty("category", category);
        obj->setProperty("toggleable", toggleable);
        return juce::var(obj);
    }
    
    static ToolItem fromVar(const juce::var& v) {
        ToolItem item;
        if (auto* obj = v.getDynamicObject()) {
            item.id = obj->getProperty("id").toString();
            item.name = obj->getProperty("name").toString();
            item.icon = obj->getProperty("icon").toString();
            item.category = obj->getProperty("category").toString();
            item.toggleable = obj->getProperty("toggleable");
        }
        return item;
    }
};

//==============================================================================
// Quick Access Toolbar - Toolbar personalizable
//==============================================================================
class QuickAccessToolbar : public juce::Component,
                          public juce::DragAndDropTarget {
public:
    enum class Size {
        Small,  // 24x24
        Medium, // 32x32
        Large   // 48x48
    };
    
    QuickAccessToolbar() {
        // Registrar herramientas disponibles
        registerDefaultTools();
        
        // Cargar configuración guardada
        loadConfiguration();
        
        // Background style
        setOpaque(true);
    }
    
    void paint(juce::Graphics& g) override {
        // Background con gradiente
        juce::ColourGradient gradient(
            juce::Colour(0xff3a3a3a), 0, 0,
            juce::Colour(0xff2d2d2d), 0, (float)getHeight(),
            false);
        g.setGradientFill(gradient);
        g.fillAll();
        
        // Border inferior
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawLine(0, (float)getHeight(), (float)getWidth(), (float)getHeight(), 2.0f);
        
        // Si está en modo edición, mostrar indicador
        if (editMode) {
            g.setColour(juce::Colour(0xffff8736).withAlpha(0.3f));
            g.fillAll();
            
            g.setColour(juce::Colour(0xffff8736));
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.drawText("EDIT MODE - Drag tools to customize", 
                      getLocalBounds(), juce::Justification::centredRight);
        }
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(4, 4);
        
        // Size selector
        sizeSelector.setBounds(area.removeFromRight(80).reduced(0, 2));
        area.removeFromRight(8);
        
        // Edit button
        editButton.setBounds(area.removeFromRight(60).reduced(0, 2));
        area.removeFromRight(12);
        
        // Layout tool buttons
        int buttonSize = getButtonSize();
        int spacing = 4;
        
        for (auto* button : toolButtons) {
            if (!area.isEmpty()) {
                button->setBounds(area.removeFromLeft(buttonSize).withHeight(buttonSize));
                area.removeFromLeft(spacing);
            }
        }
    }
    
    // API pública
    void addTool(const ToolItem& item) {
        auto* button = new ToolButton(item.name, item.icon);
        button->setClickingTogglesState(item.toggleable);
        button->onClick = item.action;
        
        toolButtons.add(button);
        addAndMakeVisible(button);
        activeTools.push_back(item);
        
        resized();
    }
    
    void removeTool(int index) {
        if (index >= 0 && index < toolButtons.size()) {
            removeChildComponent(toolButtons[index]);
            toolButtons.remove(index);
            activeTools.erase(activeTools.begin() + index);
            resized();
        }
    }
    
    void clearTools() {
        toolButtons.clear();
        activeTools.clear();
        resized();
    }
    
    void setToolbarSize(Size newSize) {
        currentSize = newSize;
        resized();
    }
    
    void setEditMode(bool enabled) {
        editMode = enabled;
        repaint();
    }
    
    // DragAndDropTarget implementation
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails&) override {
        return editMode;
    }
    
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& details) override {
        if (auto* toolItem = dynamic_cast<const ToolItem*>(details.description.getDynamicObject())) {
            addTool(*toolItem);
            saveConfiguration();
        }
    }
    
    std::vector<ToolItem> getAvailableTools() const {
        return availableTools;
    }
    
    std::function<void(const ToolItem&)> onToolAdded;
    std::function<void(int)> onToolRemoved;
    
private:
    juce::OwnedArray<ToolButton> toolButtons;
    std::vector<ToolItem> activeTools;
    std::vector<ToolItem> availableTools;
    
    juce::ComboBox sizeSelector;
    juce::TextButton editButton;
    Size currentSize = Size::Medium;
    bool editMode = false;
    
    int getButtonSize() const {
        switch (currentSize) {
            case Size::Small:  return 24;
            case Size::Medium: return 32;
            case Size::Large:  return 48;
            default:          return 32;
        }
    }
    
    void registerDefaultTools() {
        // Transport
        availableTools.push_back(ToolItem("play", "Play", "▶️", nullptr, "Transport"));
        availableTools.push_back(ToolItem("stop", "Stop", "⏹️", nullptr, "Transport"));
        availableTools.push_back(ToolItem("record", "Record", "⏺️", nullptr, "Transport"));
        availableTools.push_back(ToolItem("loop", "Loop", "🔁", nullptr, "Transport", true));
        
        // Windows
        availableTools.push_back(ToolItem("pianoroll", "Piano Roll", "🎹", nullptr, "Windows"));
        availableTools.push_back(ToolItem("mixer", "Mixer", "🎚️", nullptr, "Windows"));
        availableTools.push_back(ToolItem("browser", "Browser", "📁", nullptr, "Windows"));
        availableTools.push_back(ToolItem("playlist", "Playlist", "📝", nullptr, "Windows"));
        
        // Tools
        availableTools.push_back(ToolItem("quantize", "Quantize", "📐", nullptr, "Tools"));
        availableTools.push_back(ToolItem("metronome", "Metronome", "🎵", nullptr, "Tools", true));
        availableTools.push_back(ToolItem("snap", "Snap", "🧲", nullptr, "Tools", true));
        availableTools.push_back(ToolItem("undo", "Undo", "↶", nullptr, "Tools"));
        availableTools.push_back(ToolItem("redo", "Redo", "↷", nullptr, "Tools"));
        
        // AI Features
        availableTools.push_back(ToolItem("stemsep", "Stem Separation", "🎵", nullptr, "AI"));
        availableTools.push_back(ToolItem("gopher", "Gopher AI", "🤖", nullptr, "AI"));
        availableTools.push_back(ToolItem("loopstarter", "Loop Starter", "✨", nullptr, "AI"));
        
        // Effects
        availableTools.push_back(ToolItem("reverb", "Reverb", "🌊", nullptr, "Effects"));
        availableTools.push_back(ToolItem("delay", "Delay", "⏱️", nullptr, "Effects"));
        availableTools.push_back(ToolItem("compressor", "Compressor", "📉", nullptr, "Effects"));
        
        // Workspaces
        availableTools.push_back(ToolItem("ws_recording", "Recording Layout", "🎙️", nullptr, "Workspace"));
        availableTools.push_back(ToolItem("ws_production", "Production Layout", "🎛️", nullptr, "Workspace"));
        availableTools.push_back(ToolItem("ws_mixing", "Mixing Layout", "🎚️", nullptr, "Workspace"));
    }
    
    void saveConfiguration() {
        juce::File configFile = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("OmegaStudio")
            .getChildFile("toolbar_config.json");
        
        juce::Array<juce::var> toolsArray;
        for (const auto& tool : activeTools) {
            toolsArray.add(tool.toVar());
        }
        
        auto* obj = new juce::DynamicObject();
        obj->setProperty("size", (int)currentSize);
        obj->setProperty("tools", toolsArray);
        
        juce::var data(obj);
        configFile.replaceWithText(juce::JSON::toString(data, true));
    }
    
    void loadConfiguration() {
        juce::File configFile = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("OmegaStudio")
            .getChildFile("toolbar_config.json");
        
        if (configFile.existsAsFile()) {
            auto json = juce::JSON::parse(configFile);
            if (auto* obj = json.getDynamicObject()) {
                currentSize = (Size)(int)obj->getProperty("size");
                
                if (auto* toolsArray = obj->getProperty("tools").getArray()) {
                    for (const auto& toolVar : *toolsArray) {
                        auto item = ToolItem::fromVar(toolVar);
                        // Find matching tool in available tools and add
                        for (const auto& available : availableTools) {
                            if (available.id == item.id) {
                                addTool(available);
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            // Default configuration
            addDefaultTools();
        }
    }
    
    void addDefaultTools() {
        // Default tools: play, stop, record, mixer, piano roll
        for (const auto& tool : availableTools) {
            if (tool.id == "play" || tool.id == "stop" || tool.id == "record" ||
                tool.id == "mixer" || tool.id == "pianoroll") {
                addTool(tool);
            }
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickAccessToolbar)
};

//==============================================================================
// Toolbar Customizer Dialog - Diálogo para personalizar toolbar
//==============================================================================
class ToolbarCustomizerDialog : public juce::Component {
public:
    ToolbarCustomizerDialog(QuickAccessToolbar& toolbar)
        : targetToolbar(toolbar) {
        setSize(600, 400);
        
        // Available tools list
        addAndMakeVisible(availableList);
        availableList.setMultipleSelectionEnabled(false);
        
        // Populate available tools
        auto tools = toolbar.getAvailableTools();
        for (const auto& tool : tools) {
            availableList.addItem(tool.icon + " " + tool.name, availableList.getNumRows() + 1);
        }
        
        // Add button
        addButton.setButtonText("Add →");
        addButton.onClick = [this] { addSelectedTool(); };
        addAndMakeVisible(addButton);
        
        // Instructions
        instructions.setText("Drag tools from left to add to toolbar", juce::dontSendNotification);
        instructions.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(instructions);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2b2b2b));
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(12);
        
        instructions.setBounds(area.removeFromTop(30));
        area.removeFromTop(8);
        
        auto listsArea = area.removeFromTop(area.getHeight() - 40);
        availableList.setBounds(listsArea);
        
        area.removeFromTop(8);
        addButton.setBounds(area.withSizeKeepingCentre(120, 32));
    }
    
private:
    QuickAccessToolbar& targetToolbar;
    juce::ListBox availableList;
    juce::TextButton addButton;
    juce::Label instructions;
    
    void addSelectedTool() {
        // Implementation for adding tool
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarCustomizerDialog)
};

} // namespace GUI
} // namespace OmegaStudio
