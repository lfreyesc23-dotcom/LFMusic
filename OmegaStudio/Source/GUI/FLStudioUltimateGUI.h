//==============================================================================
// FLStudioUltimateGUI.h
// FL Studio 2025 - GUI DEFINITIVO CON TODOS LOS SISTEMAS INTEGRADOS
// Docking, Workspaces, Status Bar, Context Menus, Animations, Themes
//==============================================================================

#pragma once

#include <JuceHeader.h>

// Importar todos los sistemas nuevos
#include "DockingSystem.h"
#include "WorkspaceManager.h"
#include "StatusBar.h"
#include "ContextMenuSystem.h"
#include "QuickAccessToolbar.h"
#include "AdvancedPlaylistFeatures.h"
#include "AdvancedMixerFeatures.h"
#include "AdvancedBrowserSystem.h"
#include "AdvancedPianoRollFeatures.h"
#include "AnimationSystem.h"
#include "ThemeSystem.h"

// GUI components existentes
#include "FunctionalChannelRack.h"
#include "FunctionalPianoRoll.h"
#include "FunctionalMixer.h"
#include "FunctionalPlaylist.h"
#include "FunctionalTransportBar.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// FL STUDIO ULTIMATE WINDOW - Ventana principal con TODOS los sistemas
//==============================================================================
class FLStudioUltimateWindow : public juce::Component,
                               public juce::KeyListener,
                               private juce::ChangeListener {
public:
    FLStudioUltimateWindow() {
        // Aplicar tema
        applyCurrentTheme();
        ThemeManager::getInstance().addChangeListener(this);
        
        // Quick Access Toolbar
        addAndMakeVisible(quickToolbar);
        setupQuickToolbar();
        
        // Transport Bar
        addAndMakeVisible(transport);
        
        // Status Bar
        addAndMakeVisible(statusBar);
        statusBar.showHint("Welcome to OmegaStudio - Press F1 for help", "✨", "F1");
        
        // Setup dockable panels
        setupDockablePanels();
        
        // Workspace Manager con F-keys
        addKeyListener(&workspaceManager);
        setupWorkspaces();
        
        // Main content area con tabs
        setupMainContent();
        
        // Cargar último workspace
        workspaceManager.loadWorkspace("Production");
        
        setSize(1600, 900);
        
        // Start performance monitoring
        startTimer(250); // Update status bar 4x/segundo
    }
    
    ~FLStudioUltimateWindow() override {
        ThemeManager::getInstance().removeChangeListener(this);
        removeKeyListener(&workspaceManager);
        
        // Save current workspace
        workspaceManager.saveCurrentWorkspace("Last Session");
    }
    
    void paint(juce::Graphics& g) override {
        auto& colors = ThemeManager::getInstance().getColors();
        
        // Background con gradiente según tema
        juce::ColourGradient gradient(
            colors.backgroundMedium, (float)getWidth() / 2, 0,
            colors.backgroundDark, (float)getWidth() / 2, (float)getHeight(),
            false);
        g.setGradientFill(gradient);
        g.fillAll();
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        // Quick Access Toolbar (top)
        quickToolbar.setBounds(area.removeFromTop(40));
        
        // Transport Bar
        transport.setBounds(area.removeFromTop(60));
        
        // Status Bar (bottom)
        statusBar.setBounds(area.removeFromBottom(28));
        
        // Dockable panels layout
        layoutDockablePanels(area);
    }
    
    // Context menu override
    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isPopupMenu()) {
            showMainContextMenu(event.x, event.y);
        }
    }
    
    // Keyboard shortcuts
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override {
        // Ctrl+S: Save project
        if (key.isKeyCode(juce::KeyPress::spaceKey) && key.getModifiers().isCommandDown()) {
            statusBar.showHint("Project saved", "💾", "Ctrl+S");
            return true;
        }
        
        // Ctrl+O: Open project
        if (key.getKeyCode() == 'O' && key.getModifiers().isCommandDown()) {
            statusBar.showHint("Open project", "📂", "Ctrl+O");
            return true;
        }
        
        // Ctrl+T: Show theme settings
        if (key.getKeyCode() == 'T' && key.getModifiers().isCommandDown()) {
            showThemeSettings();
            return true;
        }
        
        return false;
    }
    
private:
    // Core systems
    WorkspaceManager workspaceManager;
    DockingManager dockingManager;
    
    // UI Components
    QuickAccessToolbar quickToolbar;
    FunctionalTransportBar transport;
    StatusBar statusBar;
    
    // Dockable panels
    std::unique_ptr<DockablePanel> browserPanel;
    std::unique_ptr<DockablePanel> mixerPanel;
    std::unique_ptr<DockablePanel> channelRackPanel;
    std::unique_ptr<DockablePanel> pianoRollPanel;
    std::unique_ptr<DockablePanel> playlistPanel;
    std::unique_ptr<DockablePanel> fxDockPanel;
    
    // Content components
    std::unique_ptr<AdvancedBrowserPanel> browser;
    std::unique_ptr<FunctionalMixer> mixer;
    std::unique_ptr<FunctionalChannelRack> channelRack;
    std::unique_ptr<FunctionalPianoRoll> pianoRoll;
    std::unique_ptr<FunctionalPlaylist> playlist;
    std::unique_ptr<FXDockPanel> fxDock;
    
    // Advanced features
    std::unique_ptr<AdvancedRuler> playlistRuler;
    std::unique_ptr<ScaleHighlighter> scaleHighlighter;
    std::unique_ptr<SidechainRoutingVisualizer> sidechainViz;
    
    // Main content area
    juce::Rectangle<int> mainContentArea;
    
    void setupQuickToolbar() {
        // Add transport controls
        quickToolbar.addTool(ToolItem("play", "Play", "▶️", [this] {
            transport.togglePlay();
            statusBar.showHint("Playing", "▶️", "Space");
        }));
        
        quickToolbar.addTool(ToolItem("stop", "Stop", "⏹️", [this] {
            transport.stop();
            statusBar.showHint("Stopped", "⏹️", "Esc");
        }));
        
        quickToolbar.addTool(ToolItem("record", "Record", "⏺️", [this] {
            transport.toggleRecord();
            statusBar.showHint("Recording", "⏺️", "Ctrl+R");
        }));
        
        // Add window toggles
        quickToolbar.addTool(ToolItem("mixer", "Mixer", "🎚️", [this] {
            togglePanel(mixerPanel.get());
        }));
        
        quickToolbar.addTool(ToolItem("pianoroll", "Piano Roll", "🎹", [this] {
            togglePanel(pianoRollPanel.get());
        }));
        
        // AI tools
        quickToolbar.addTool(ToolItem("stemsep", "Stem Separation", "🎵", [this] {
            statusBar.showHint("Opening Stem Separator...", "🎵");
        }));
        
        quickToolbar.addTool(ToolItem("gopher", "Gopher AI", "🤖", [this] {
            statusBar.showHint("Opening AI Assistant...", "🤖");
        }));
    }
    
    void setupDockablePanels() {
        // Browser Panel
        browserPanel = std::make_unique<DockablePanel>("Browser");
        browser = std::make_unique<AdvancedBrowserPanel>();
        browserPanel->setContent(browser.get());
        browserPanel->setDockPosition(DockablePanel::DockPosition::Left);
        addAndMakeVisible(browserPanel.get());
        dockingManager.registerPanel("browser", browserPanel.get());
        workspaceManager.registerPanel("browser", browserPanel.get());
        
        // Mixer Panel
        mixerPanel = std::make_unique<DockablePanel>("Mixer");
        mixer = std::make_unique<FunctionalMixer>();
        mixerPanel->setContent(mixer.get());
        mixerPanel->setDockPosition(DockablePanel::DockPosition::Right);
        addAndMakeVisible(mixerPanel.get());
        dockingManager.registerPanel("mixer", mixerPanel.get());
        workspaceManager.registerPanel("mixer", mixerPanel.get());
        
        // Channel Rack Panel
        channelRackPanel = std::make_unique<DockablePanel>("Channel Rack");
        channelRack = std::make_unique<FunctionalChannelRack>();
        channelRackPanel->setContent(channelRack.get());
        channelRackPanel->setDockPosition(DockablePanel::DockPosition::Bottom);
        addAndMakeVisible(channelRackPanel.get());
        dockingManager.registerPanel("channelRack", channelRackPanel.get());
        workspaceManager.registerPanel("channelRack", channelRackPanel.get());
        
        // Piano Roll Panel
        pianoRollPanel = std::make_unique<DockablePanel>("Piano Roll");
        pianoRoll = std::make_unique<FunctionalPianoRoll>();
        
        // Add scale highlighter to piano roll
        scaleHighlighter = std::make_unique<ScaleHighlighter>();
        pianoRoll->addAndMakeVisible(scaleHighlighter.get());
        
        pianoRollPanel->setContent(pianoRoll.get());
        pianoRollPanel->setDockPosition(DockablePanel::DockPosition::Bottom);
        pianoRollPanel->setVisible(false);
        addAndMakeVisible(pianoRollPanel.get());
        dockingManager.registerPanel("pianoRoll", pianoRollPanel.get());
        workspaceManager.registerPanel("pianoRoll", pianoRollPanel.get());
        
        // Playlist Panel
        playlistPanel = std::make_unique<DockablePanel>("Playlist");
        playlist = std::make_unique<FunctionalPlaylist>();
        
        // Add advanced ruler
        playlistRuler = std::make_unique<AdvancedRuler>();
        playlist->addAndMakeVisible(playlistRuler.get());
        
        playlistPanel->setContent(playlist.get());
        playlistPanel->setDockPosition(DockablePanel::DockPosition::Bottom);
        addAndMakeVisible(playlistPanel.get());
        dockingManager.registerPanel("playlist", playlistPanel.get());
        workspaceManager.registerPanel("playlist", playlistPanel.get());
        
        // FX Dock Panel
        fxDockPanel = std::make_unique<DockablePanel>("FX Chain");
        fxDock = std::make_unique<FXDockPanel>();
        fxDockPanel->setContent(fxDock.get());
        fxDockPanel->setDockPosition(DockablePanel::DockPosition::Right);
        fxDockPanel->setVisible(false);
        addAndMakeVisible(fxDockPanel.get());
        dockingManager.registerPanel("fxChain", fxDockPanel.get());
        workspaceManager.registerPanel("fxChain", fxDockPanel.get());
        
        // Setup callbacks para status bar hints
        setupPanelHints();
    }
    
    void setupPanelHints() {
        browserPanel->onDetached = [this] {
            statusBar.showHint("Browser detached - Drag to reposition", "🗔");
        };
        
        mixerPanel->onDetached = [this] {
            statusBar.showHint("Mixer detached - Use multiple monitors", "🎚️");
        };
        
        pianoRollPanel->onDetached = [this] {
            statusBar.showHint("Piano Roll detached", "🎹");
        };
    }
    
    void setupWorkspaces() {
        workspaceManager.onWorkspaceChanged = [this](const WorkspaceLayout& layout) {
            statusBar.showHint("Workspace: " + layout.name, "🗂️", layout.shortcutKey);
            
            // Apply accent color from workspace
            ThemeManager::getInstance().setAccentColor(layout.accentColour);
        };
    }
    
    void setupMainContent() {
        // Main content will be managed by dockable panels
    }
    
    void layoutDockablePanels(juce::Rectangle<int> area) {
        mainContentArea = area;
        
        // Left panels (Browser)
        if (browserPanel->isVisible() && !browserPanel->isDetached()) {
            browserPanel->setBounds(area.removeFromLeft(280));
            area.removeFromLeft(2);
        }
        
        // Right panels (Mixer, FX Chain)
        if (mixerPanel->isVisible() && !mixerPanel->isDetached()) {
            mixerPanel->setBounds(area.removeFromRight(320));
            area.removeFromRight(2);
        }
        
        if (fxDockPanel->isVisible() && !fxDockPanel->isDetached()) {
            fxDockPanel->setBounds(area.removeFromRight(300));
            area.removeFromRight(2);
        }
        
        // Bottom panels (Channel Rack, Piano Roll, Playlist)
        int bottomHeight = area.getHeight() / 2;
        
        if (playlistPanel->isVisible() && !playlistPanel->isDetached()) {
            playlistPanel->setBounds(area.removeFromBottom(bottomHeight));
            area.removeFromBottom(2);
        }
        
        if (pianoRollPanel->isVisible() && !pianoRollPanel->isDetached()) {
            pianoRollPanel->setBounds(area.removeFromBottom(bottomHeight));
            area.removeFromBottom(2);
        }
        
        if (channelRackPanel->isVisible() && !channelRackPanel->isDetached()) {
            channelRackPanel->setBounds(area.removeFromBottom(bottomHeight));
            area.removeFromBottom(2);
        }
    }
    
    void togglePanel(DockablePanel* panel) {
        if (panel) {
            panel->setVisible(!panel->isVisible());
            resized();
        }
    }
    
    void showMainContextMenu(int x, int y) {
        auto menu = ContextMenuBuilder()
            .addItem("New Project", [this] { 
                statusBar.showHint("New project", "📄", "Ctrl+N");
            }, "📄", "Ctrl+N")
            .addItem("Open Project", [this] {
                statusBar.showHint("Open project", "📂", "Ctrl+O");
            }, "📂", "Ctrl+O")
            .addItem("Save Project", [this] {
                statusBar.showHint("Project saved", "💾", "Ctrl+S");
            }, "💾", "Ctrl+S")
            .addSeparator()
            .addSubMenu("Workspaces", {
                ContextMenuItem("Recording (F5)", [this] {
                    workspaceManager.loadWorkspace("Recording");
                }),
                ContextMenuItem("Production (F6)", [this] {
                    workspaceManager.loadWorkspace("Production");
                }),
                ContextMenuItem("Mixing (F7)", [this] {
                    workspaceManager.loadWorkspace("Mixing");
                }),
                ContextMenuItem("Mastering (F8)", [this] {
                    workspaceManager.loadWorkspace("Mastering");
                }),
                ContextMenuItem::separator(),
                ContextMenuItem("Save Current Layout...", [this] {
                    saveCurrentWorkspace();
                })
            }, "🗂️")
            .addSubMenu("Windows", {
                ContextMenuItem("Browser", [this] { togglePanel(browserPanel.get()); }, "📁"),
                ContextMenuItem("Mixer", [this] { togglePanel(mixerPanel.get()); }, "🎚️"),
                ContextMenuItem("Channel Rack", [this] { togglePanel(channelRackPanel.get()); }, "🥁"),
                ContextMenuItem("Piano Roll", [this] { togglePanel(pianoRollPanel.get()); }, "🎹"),
                ContextMenuItem("Playlist", [this] { togglePanel(playlistPanel.get()); }, "📝"),
                ContextMenuItem("FX Chain", [this] { togglePanel(fxDockPanel.get()); }, "🔌")
            }, "🪟")
            .addSeparator()
            .addItem("Theme Settings", [this] {
                showThemeSettings();
            }, "🎨", "Ctrl+T")
            .addItem("Preferences", [this] {
                statusBar.showHint("Opening preferences...", "⚙️");
            }, "⚙️", "Ctrl+,")
            .addSeparator()
            .addItem("About OmegaStudio", [this] {
                showAbout();
            }, "ℹ️")
            .build();
        
        UniversalContextMenu::show(menu, this, x, y);
    }
    
    void showThemeSettings() {
        // Create theme settings dialog
        auto* themePanel = new ThemeSettingsPanel();
        themePanel->setSize(400, 500);
        
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(themePanel);
        options.dialogTitle = "Theme Settings";
        options.dialogBackgroundColour = ThemeManager::getInstance().getColors().backgroundMedium;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        
        auto* dialog = options.launchAsync();
        
        statusBar.showHint("Theme settings opened", "🎨");
    }
    
    void showAbout() {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "About OmegaStudio",
            "OmegaStudio - FL Studio 2025 Killer Edition\n\n"
            "Version 1.0.0\n"
            "Build Date: December 7, 2025\n\n"
            "Professional DAW with:\n"
            "• Docking System\n"
            "• Workspace Layouts (F5-F12)\n"
            "• Advanced Animations\n"
            "• Complete Theme System\n"
            "• AI-Powered Features\n"
            "• 100% Free & Open Source\n\n"
            "Developed with ❤️ by OmegaStudio Team",
            "OK"
        );
    }
    
    void saveCurrentWorkspace() {
        juce::AlertWindow window("Save Workspace", 
                                "Enter a name for this workspace:",
                                juce::AlertWindow::QuestionIcon);
        
        window.addTextEditor("name", "My Workspace");
        window.addButton("Save", 1);
        window.addButton("Cancel", 0);
        
        if (window.runModalLoop() == 1) {
            juce::String name = window.getTextEditorContents("name");
            if (name.isNotEmpty()) {
                workspaceManager.saveCurrentWorkspace(name);
                statusBar.showHint("Workspace saved: " + name, "💾");
            }
        }
    }
    
    void applyCurrentTheme() {
        auto& theme = ThemeManager::getInstance();
        auto& colors = theme.getColors();
        
        // Apply to this component
        setOpaque(true);
        
        // Update all child components with theme colors
        repaint();
    }
    
    // ChangeListener implementation (for theme changes)
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        applyCurrentTheme();
        statusBar.showHint("Theme updated", "🎨");
    }
    
    // Timer callback for status bar updates
    void timerCallback() override {
        // Update CPU/RAM usage (get from audio engine)
        // statusBar.updateCPUUsage(audioEngine.getCPUUsage());
        // statusBar.updateRAMUsage(audioEngine.getRAMUsage());
        
        // Update time display
        // statusBar.setTime(transport.getCurrentTime());
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudioUltimateWindow)
};

} // namespace GUI
} // namespace OmegaStudio
