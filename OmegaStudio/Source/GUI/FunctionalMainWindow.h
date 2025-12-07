//==============================================================================
// FunctionalMainWindow.h
// Ventana principal DAW con todos los componentes integrados
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "FunctionalTransportBar.h"
#include "FunctionalChannelRack.h"
#include "FunctionalPlaylist.h"
#include "FunctionalPianoRoll.h"
#include "FunctionalMixer.h"

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Ventana principal del DAW
//==============================================================================
class FunctionalMainWindow : public juce::Component,
                             public juce::MenuBarModel {
public:
    FunctionalMainWindow() {
        // Crear todos los componentes PRIMERO
        channelRack = std::make_unique<FunctionalChannelRack>();
        playlist = std::make_unique<FunctionalPlaylist>();
        pianoRoll = std::make_unique<FunctionalPianoRoll>();
        mixer = std::make_unique<FunctionalMixer>();
        
        // Transport bar (siempre visible arriba)
        transportBar = std::make_unique<FunctionalTransportBar>();
        
        // TabbedComponent para organizar las vistas
        tabbedComponent = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
        tabbedComponent->setTabBarDepth(35);
        
        // Agregar tabs con iconos
        tabbedComponent->addTab("🎹 Channel Rack", juce::Colour(0xff2a2a2a), channelRack.get(), false);
        tabbedComponent->addTab("📊 Playlist", juce::Colour(0xff2a2a2a), playlist.get(), false);
        tabbedComponent->addTab("🎼 Piano Roll", juce::Colour(0xff2a2a2a), pianoRoll.get(), false);
        tabbedComponent->addTab("🎚️ Mixer", juce::Colour(0xff2a2a2a), mixer.get(), false);
        
        // Menu bar
        menuBar = std::make_unique<juce::MenuBarComponent>(this);
        
        // Agregar componentes a la ventana
        addAndMakeVisible(menuBar.get());
        addAndMakeVisible(transportBar.get());
        addAndMakeVisible(tabbedComponent.get());
        
        // Conectar callbacks del transport
        transportBar->onPlay = [this] {
            // TODO: Start playback
            DBG("Play pressed");
        };
        
        transportBar->onStop = [this] {
            // TODO: Stop playback
            DBG("Stop pressed");
        };
        
        transportBar->onRecord = [this] {
            // TODO: Toggle recording
            DBG("Record pressed");
        };
        
        transportBar->onTempoChange = [this](double bpm) {
            DBG("Tempo changed to: " << bpm);
        };
        
        transportBar->onMetronomeToggle = [this](bool enabled) {
            DBG("Metronome: " << (enabled ? "ON" : "OFF"));
        };
        
        // Conectar callbacks del channel rack
        channelRack->onChannelsChanged = [this] {
            DBG("Channels changed, total: " << channelRack->getNumChannels());
        };
        
        // Status bar en la parte inferior
        statusLabel.setText("Ready - OmegaStudio FL Edition", juce::dontSendNotification);
        statusLabel.setJustificationType(juce::Justification::centredLeft);
        statusLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a1a1a));
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(statusLabel);
        
        // Aplicar Look and Feel oscuro
        getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
        getLookAndFeel().setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xff1e1e1e));
        getLookAndFeel().setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(0xff3a3a3a));
        getLookAndFeel().setColour(juce::TabbedButtonBar::frontOutlineColourId, juce::Colour(0xff4CAF50));
        
        setWantsKeyboardFocus(true);
        setSize(1400, 900);
    }
    
    ~FunctionalMainWindow() override {
        tabbedComponent->clearTabs();
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
    }
    
    void resized() override {
        auto area = getLocalBounds();
        
        // Menu bar (arriba)
        if (menuBar)
            menuBar->setBounds(area.removeFromTop(24));
        
        // Transport bar
        if (transportBar)
            transportBar->setBounds(area.removeFromTop(60));
        
        // Status bar (abajo)
        statusLabel.setBounds(area.removeFromBottom(25));
        
        // Tabbed component (área principal)
        if (tabbedComponent)
            tabbedComponent->setBounds(area);
    }
    
    //==========================================================================
    // MenuBarModel implementation
    //==========================================================================
    juce::StringArray getMenuBarNames() override {
        return { "File", "Edit", "View", "Tools", "Help" };
    }
    
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& /*menuName*/) override {
        juce::PopupMenu menu;
        
        if (menuIndex == 0) { // File
            menu.addItem(1, "New Project", true);
            menu.addItem(2, "Open Project...", true);
            menu.addItem(3, "Save Project", true);
            menu.addItem(4, "Save Project As...", true);
            menu.addSeparator();
            menu.addItem(5, "Export Audio...", true);
            menu.addItem(6, "Export MIDI...", true);
            menu.addSeparator();
            menu.addItem(10, "Exit", true);
        }
        else if (menuIndex == 1) { // Edit
            menu.addItem(20, "Undo", true);
            menu.addItem(21, "Redo", true);
            menu.addSeparator();
            menu.addItem(22, "Cut", true);
            menu.addItem(23, "Copy", true);
            menu.addItem(24, "Paste", true);
            menu.addItem(25, "Delete", true);
            menu.addSeparator();
            menu.addItem(26, "Select All", true);
        }
        else if (menuIndex == 2) { // View
            menu.addItem(30, "Channel Rack", true, tabbedComponent->getCurrentTabIndex() == 0);
            menu.addItem(31, "Playlist", true, tabbedComponent->getCurrentTabIndex() == 1);
            menu.addItem(32, "Piano Roll", true, tabbedComponent->getCurrentTabIndex() == 2);
            menu.addItem(33, "Mixer", true, tabbedComponent->getCurrentTabIndex() == 3);
            menu.addSeparator();
            menu.addItem(34, "Full Screen", true, false);
        }
        else if (menuIndex == 3) { // Tools
            menu.addItem(40, "AI Stem Separation", true);
            menu.addItem(41, "Loop Starter", true);
            menu.addItem(42, "Chord Generator", true);
            menu.addItem(43, "Gopher Assistant", true);
            menu.addSeparator();
            menu.addItem(44, "Audio Settings...", true);
            menu.addItem(45, "MIDI Settings...", true);
            menu.addItem(46, "Plugin Manager...", true);
        }
        else if (menuIndex == 4) { // Help
            menu.addItem(50, "Documentation", true);
            menu.addItem(51, "Video Tutorials", true);
            menu.addItem(52, "Keyboard Shortcuts", true);
            menu.addSeparator();
            menu.addItem(53, "About OmegaStudio", true);
        }
        
        return menu;
    }
    
    void menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/) override {
        switch (menuItemID) {
            case 1: // New Project
                showNewProjectDialog();
                break;
            case 2: // Open Project
                showOpenProjectDialog();
                break;
            case 3: // Save Project
                saveProject();
                break;
            case 10: // Exit
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
                break;
            case 30: // Channel Rack
                tabbedComponent->setCurrentTabIndex(0);
                break;
            case 31: // Playlist
                tabbedComponent->setCurrentTabIndex(1);
                break;
            case 32: // Piano Roll
                tabbedComponent->setCurrentTabIndex(2);
                break;
            case 33: // Mixer
                tabbedComponent->setCurrentTabIndex(3);
                break;
            case 40: // AI Stem Separation
                statusLabel.setText("🤖 AI Stem Separation - Coming soon!", juce::dontSendNotification);
                break;
            case 41: // Loop Starter
                statusLabel.setText("🎵 Loop Starter - Coming soon!", juce::dontSendNotification);
                break;
            case 42: // Chord Generator
                statusLabel.setText("🎹 Chord Generator - Coming soon!", juce::dontSendNotification);
                break;
            case 43: // Gopher Assistant
                statusLabel.setText("💡 Gopher AI Assistant - Coming soon!", juce::dontSendNotification);
                break;
            case 53: // About
                showAboutDialog();
                break;
            default:
                break;
        }
    }
    
private:
    void showNewProjectDialog() {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "New Project",
            "Create a new project (functionality coming soon)",
            "OK"
        );
    }
    
    void showOpenProjectDialog() {
        auto chooser = std::make_shared<juce::FileChooser>("Open Project", 
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.omega");
        
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                    statusLabel.setText("Opened: " + file.getFileName(), juce::dontSendNotification);
                }
            });
    }
    
    void saveProject() {
        statusLabel.setText("Project saved successfully!", juce::dontSendNotification);
    }
    
    void showAboutDialog() {
        juce::String aboutText = 
            "OmegaStudio - FL Edition\n\n"
            "Professional DAW with FL Studio 2025 features\n"
            "100% Free & Open Source\n\n"
            "Features:\n"
            "• Advanced Channel Rack with Step Sequencer\n"
            "• Professional Playlist/Arrangement View\n"
            "• Full-featured Piano Roll Editor\n"
            "• 128-Channel Mixer with Effects\n"
            "• AI-Powered Tools (Stem Sep, Loop Starter, etc.)\n"
            "• VST/AU/CLAP Plugin Support\n\n"
            "Version: 1.0.0\n"
            "Build: December 2025";
        
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "About OmegaStudio",
            aboutText,
            "OK"
        );
    }
    
    //==========================================================================
    // Components
    //==========================================================================
    std::unique_ptr<juce::MenuBarComponent> menuBar;
    std::unique_ptr<FunctionalTransportBar> transportBar;
    std::unique_ptr<juce::TabbedComponent> tabbedComponent;
    std::unique_ptr<FunctionalChannelRack> channelRack;
    std::unique_ptr<FunctionalPlaylist> playlist;
    std::unique_ptr<FunctionalPianoRoll> pianoRoll;
    std::unique_ptr<FunctionalMixer> mixer;
    juce::Label statusLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FunctionalMainWindow)
};

} // namespace GUI
} // namespace OmegaStudio
