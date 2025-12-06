//==============================================================================
// UndoRedoSystem.h - Sistema Undo/Redo Visual con Historial
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <functional>

namespace OmegaStudio {

//==============================================================================
/** Sistema Undo/Redo Profesional
 *  - Historial navegable visualmente
 *  - Grupos de acciones (transactions)
 *  - Límite configurable de historial
 *  - Serialización de estado
 *  - Keyboard shortcuts (Cmd+Z, Cmd+Shift+Z)
 */
class UndoRedoSystem {
public:
    //==========================================================================
    struct Action {
        juce::String description;
        juce::String category;          // "Edit", "Create", "Delete", etc.
        juce::Time timestamp;
        
        std::function<void()> undo;
        std::function<void()> redo;
        
        // Estado para visualización
        juce::ValueTree stateBefore;
        juce::ValueTree stateAfter;
        
        int id = -1;
    };
    
    //==========================================================================
    UndoRedoSystem(int maxHistorySize = 100);
    ~UndoRedoSystem() = default;
    
    // Execute action (esto agrega al historial automáticamente)
    void perform(const Action& action);
    void perform(const juce::String& description,
                 std::function<void()> doAction,
                 std::function<void()> undoAction);
    
    // Undo/Redo
    bool canUndo() const;
    bool canRedo() const;
    
    void undo();
    void redo();
    void undoMultiple(int count);
    void redoMultiple(int count);
    
    // Jump to specific action
    void jumpToAction(int actionId);
    
    // History
    int getNumActions() const { return static_cast<int>(history_.size()); }
    int getCurrentPosition() const { return currentPosition_; }
    
    const Action& getAction(int index) const { return *history_[index]; }
    std::vector<const Action*> getHistory() const;
    
    // Transactions (agrupar múltiples acciones)
    void beginTransaction(const juce::String& transactionName);
    void endTransaction();
    void abortTransaction();
    bool isInTransaction() const { return inTransaction_; }
    
    // Clear
    void clearHistory();
    void clearFuture();  // Clear redo stack
    
    // Settings
    void setMaxHistorySize(int size);
    int getMaxHistorySize() const { return maxHistorySize_; }
    
    // Listeners
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void historyChanged() {}
        virtual void actionPerformed(const Action& action) {}
        virtual void actionUndone(const Action& action) {}
        virtual void actionRedone(const Action& action) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    std::vector<std::unique_ptr<Action>> history_;
    int currentPosition_ = -1;
    int maxHistorySize_ = 100;
    
    // Transaction support
    bool inTransaction_ = false;
    juce::String transactionName_;
    std::vector<std::unique_ptr<Action>> transactionActions_;
    
    // Listeners
    juce::ListenerList<Listener> listeners_;
    
    // ID generation
    int nextActionId_ = 1;
    
    void notifyHistoryChanged();
    void trimHistory();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoRedoSystem)
};

//==============================================================================
/** Quick Search System - Búsqueda global estilo Cmd+K */
class QuickSearchSystem {
public:
    //==========================================================================
    enum class ItemType {
        Command,        // Comando de menú
        File,           // Archivo del proyecto
        Track,          // Pista
        Plugin,         // Plugin
        Preset,         // Preset
        Sample,         // Sample
        Setting,        // Configuración
        Help            // Ayuda/documentación
    };
    
    struct SearchResult {
        juce::String title;
        juce::String subtitle;
        juce::String category;
        ItemType type;
        
        float relevanceScore = 0.0f;
        
        std::function<void()> action;  // Acción al seleccionar
        
        // Visual
        juce::Colour color = juce::Colours::white;
        juce::String icon;  // Icon name o emoji
    };
    
    //==========================================================================
    QuickSearchSystem();
    ~QuickSearchSystem() = default;
    
    // Search
    std::vector<SearchResult> search(const juce::String& query, int maxResults = 20);
    
    // Register searchable items
    void registerCommand(const juce::String& name, const juce::String& category,
                        std::function<void()> action, const juce::String& keywords = "");
    
    void registerFile(const juce::String& path, const juce::String& description = "");
    void registerTrack(const juce::String& name, int index);
    void registerPlugin(const juce::String& name, const juce::String& vendor);
    void registerPreset(const juce::String& name, const juce::String& category);
    
    // Clear
    void clearAll();
    void clearCategory(const juce::String& category);
    
    // Recent searches
    void addToRecentSearches(const juce::String& query);
    juce::StringArray getRecentSearches(int maxCount = 10) const;
    void clearRecentSearches();
    
private:
    //==========================================================================
    struct SearchableItem {
        juce::String name;
        juce::String category;
        juce::String keywords;
        ItemType type;
        std::function<void()> action;
        juce::Colour color;
        juce::String icon;
    };
    
    std::vector<SearchableItem> items_;
    juce::StringArray recentSearches_;
    
    // Fuzzy search
    float calculateRelevance(const juce::String& query, const SearchableItem& item) const;
    float fuzzyMatch(const juce::String& pattern, const juce::String& text) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickSearchSystem)
};

//==============================================================================
/** Workspace Layouts - Layouts guardables (Recording, Mixing, Mastering) */
class WorkspaceLayout {
public:
    //==========================================================================
    struct Layout {
        juce::String name;
        
        // Window positions/sizes
        std::map<juce::String, juce::Rectangle<int>> windowBounds;
        
        // Visible panels
        std::set<juce::String> visiblePanels;
        
        // Tool states
        std::map<juce::String, bool> toolStates;
        
        // Custom properties
        juce::ValueTree customProperties;
    };
    
    //==========================================================================
    WorkspaceLayout();
    ~WorkspaceLayout() = default;
    
    // Layout management
    void saveLayout(const juce::String& name);
    void loadLayout(const juce::String& name);
    void deleteLayout(const juce::String& name);
    
    juce::StringArray getAvailableLayouts() const;
    bool hasLayout(const juce::String& name) const;
    
    // Default layouts
    void createDefaultLayouts();
    
    // Current layout state
    void captureCurrentLayout(Layout& layout);
    void applyLayout(const Layout& layout);
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::map<juce::String, Layout> layouts_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceLayout)
};

//==============================================================================
/** Multi-Selection System - Selección avanzada con filtros */
class MultiSelectionSystem {
public:
    //==========================================================================
    enum class SelectionMode {
        Replace,        // Reemplazar selección
        Add,            // Agregar a selección (Shift)
        Subtract,       // Quitar de selección (Cmd)
        Toggle          // Toggle (Ctrl)
    };
    
    struct SelectableItem {
        int id;
        juce::String type;  // "note", "clip", "track", etc.
        juce::Rectangle<float> bounds;
        std::map<juce::String, juce::var> properties;
    };
    
    //==========================================================================
    MultiSelectionSystem();
    ~MultiSelectionSystem() = default;
    
    // Selection
    void select(int itemId, SelectionMode mode = SelectionMode::Replace);
    void selectMultiple(const std::vector<int>& itemIds, SelectionMode mode = SelectionMode::Replace);
    void selectInRegion(const juce::Rectangle<float>& region, SelectionMode mode = SelectionMode::Replace);
    void selectAll();
    void deselectAll();
    void invertSelection();
    
    // Queries
    bool isSelected(int itemId) const;
    int getNumSelected() const { return static_cast<int>(selectedItems_.size()); }
    std::vector<int> getSelectedIds() const;
    
    // Filtering
    void selectByType(const juce::String& type);
    void selectByProperty(const juce::String& propertyName, const juce::var& value);
    
    // Transform selected
    void transformSelected(std::function<void(SelectableItem&)> transform);
    
    // Register items
    void registerItem(const SelectableItem& item);
    void unregisterItem(int itemId);
    void clearItems();
    
    const SelectableItem* getItem(int itemId) const;
    
    // Listeners
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void selectionChanged() {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    std::map<int, SelectableItem> items_;
    std::set<int> selectedItems_;
    
    juce::ListenerList<Listener> listeners_;
    
    void notifySelectionChanged();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiSelectionSystem)
};

//==============================================================================
/** Contextual Tooltips - Tooltips inteligentes con shortcuts */
class ContextualTooltipSystem {
public:
    //==========================================================================
    struct TooltipInfo {
        juce::String title;
        juce::String description;
        juce::String shortcut;
        juce::String category;
        
        // Tips avanzados
        juce::StringArray tips;
        juce::String videoUrl;  // Link a tutorial
        
        bool showOnlyOnModifier = false;  // Solo mostrar con Alt/Cmd presionado
    };
    
    //==========================================================================
    ContextualTooltipSystem();
    ~ContextualTooltipSystem() = default;
    
    // Register tooltips
    void registerTooltip(const juce::String& componentId, const TooltipInfo& info);
    void unregisterTooltip(const juce::String& componentId);
    
    // Get tooltip
    const TooltipInfo* getTooltip(const juce::String& componentId) const;
    
    // Settings
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    void setDelay(int milliseconds) { delayMs_ = milliseconds; }
    int getDelay() const { return delayMs_; }
    
    // Show tutorial video
    void openTutorial(const juce::String& url);
    
private:
    //==========================================================================
    std::map<juce::String, TooltipInfo> tooltips_;
    bool enabled_ = true;
    int delayMs_ = 500;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContextualTooltipSystem)
};

//==============================================================================
/** Quick Access Toolbar - Barra de herramientas customizable */
class QuickAccessToolbar {
public:
    //==========================================================================
    struct Tool {
        juce::String id;
        juce::String name;
        juce::String icon;
        std::function<void()> action;
        juce::KeyPress shortcut;
        
        bool separator = false;  // Es un separador visual
    };
    
    //==========================================================================
    QuickAccessToolbar();
    ~QuickAccessToolbar() = default;
    
    // Tools
    void addTool(const Tool& tool);
    void removeTool(const juce::String& toolId);
    void clearTools();
    
    void moveTool(int fromIndex, int toIndex);
    
    int getNumTools() const { return static_cast<int>(tools_.size()); }
    const Tool& getTool(int index) const { return tools_[index]; }
    
    // Presets
    void loadPreset(const juce::String& presetName);
    void saveAsPreset(const juce::String& presetName);
    
    juce::StringArray getAvailablePresets() const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::vector<Tool> tools_;
    std::map<juce::String, juce::ValueTree> presets_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickAccessToolbar)
};

} // namespace OmegaStudio
