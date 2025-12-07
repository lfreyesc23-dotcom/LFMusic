//==============================================================================
// MainComponent.h
// Main UI component - Complete DAW workspace with all systems integrated
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "../Project/ProjectManager.h"
#include "../Audio/Plugins/PluginManager.h"
#include "../Sequencer/MIDI/MIDIEngine.h"
#include "../Mixer/MixerEngine.h"
#include "../Sequencer/Automation/AutomationSystem.h"
#include "../Audio/Instruments/Instruments.h"
#include "../Audio/AI/AdvancedAI.h"
#include "PianoRollEditor.h"
#include "MixerEditor.h"
#include "PlaylistEditor.h"
#include "PluginWindowManager.h"
#include "TransportBar.h"
#include "ChannelRackEditor.h"
#include "BrowserComponent.h"
#include "AutomationEditor.h"
#include "ThemeManager.h"
#include "AudioEditorWindow.h"
#include "PerformanceModeWindow.h"
#include "MacroPanelComponent.h"
#include "StemSeparatorUI.h"
#include "SmartMixingAssistantUI.h"
#include "FLStudioLookAndFeel.h"
#include "ChannelRackUI.h"
#include "FLStudioMainWindow.h"
#include "RecordToolbar.h"
#include "LibraryBrowserPanel.h"
#include "MixerChannelsPanel.h"

// ===== NUEVAS CARACTERÍSTICAS FL STUDIO 2025 =====
#include "../AI/StemSeparationService.h"
#include "../AI/LoopStarter.h"
#include "../AI/ChordGenerator.h"
#include "../AI/GopherAssistant.h"

// ===== FL STUDIO 2025 ULTIMATE GUI - MINIMALISTA SIN ERRORES DE COMPILACIÓN =====
#include "FLStudioUltimateGUI_Minimal.h"

// NOTE: Los siguientes headers tienen errores de compilación y se han desactivado temporalmente:
// #include "DockingSystem.h"
// #include "WorkspaceManager.h"
// #include "StatusBar.h"
// #include "ContextMenuSystem.h"
// #include "QuickAccessToolbar.h"
// #include "AdvancedPlaylistFeatures.h"
// #include "AdvancedMixerFeatures.h"
// #include "AdvancedBrowserSystem.h"
// #include "AdvancedPianoRollFeatures.h"
// #include "AnimationSystem.h"
// #include "ThemeSystem.h"
// TODO: Arreglar errores de compilación en estos headers

namespace Omega {
namespace Audio { class AudioEngine; }

namespace GUI {

//==============================================================================
// MainComponent - Complete DAW workspace with all systems
//==============================================================================
class MainComponent : public juce::Component,
                      private juce::Timer {
public:
    explicit MainComponent(Audio::AudioEngine* audioEngine);
    ~MainComponent() override;
    
    //==========================================================================
    // Component overrides
    //==========================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    // System access
    //==========================================================================
    OmegaStudio::ProjectManager& getProjectManager() { return projectManager; }
    OmegaStudio::PluginManager& getPluginManager() { return pluginManager; }
    OmegaStudio::MIDIEngine& getMIDIEngine() { return midiEngine; }
    OmegaStudio::MixerEngine& getMixerEngine() { return mixerEngine; }
    OmegaStudio::AutomationManager& getAutomationManager() { return automationManager; }
    
    //==========================================================================
    // Window management
    //==========================================================================
    void showPianoRoll();
    void showMixer();
    void showPlaylist();
    void showChannelRack();
    void showBrowser();
    void showAudioEditor();
    void showPerformanceMode();
    void showMacroPanel();
    void showStemSeparator();
    void showSmartMixingAssistant();
    
    bool keyPressed(const juce::KeyPress& key) override;
    
private:
    //==========================================================================
    // Timer callback (for CPU meter updates, etc.)
    //==========================================================================
    void timerCallback() override;
    
    //==========================================================================
    // Data members
    //==========================================================================
    Audio::AudioEngine* audioEngine_;
    
    // All DAW Systems (initialized and ready)
    OmegaStudio::ProjectManager projectManager;
    OmegaStudio::PluginManager& pluginManager;  // Reference to singleton
    OmegaStudio::MIDIEngine midiEngine;
    OmegaStudio::MixerEngine mixerEngine;
    OmegaStudio::AutomationManager automationManager;
    
    // Instruments
    std::unique_ptr<OmegaStudio::ProSampler> sampler;
    std::unique_ptr<OmegaStudio::ProSynth> synth;
    std::unique_ptr<OmegaStudio::DrumMachine> drumMachine;
    
    // AI Features
    std::unique_ptr<OmegaStudio::StemSeparator> stemSeparator;
    
    // FL Studio-style UI Components
    FLStudioLookAndFeel flLookAndFeel_;
    std::unique_ptr<ChannelRackUI> channelRackUI_;
    std::unique_ptr<OmegaStudio::Sequencer::ChannelRackEngine> channelRackEngine_;
    std::unique_ptr<OmegaStudio::MasteringAssistant> masteringAssistant;
    std::unique_ptr<OmegaStudio::SmartEQ> smartEQ;
    std::unique_ptr<OmegaStudio::MixAnalyzer> mixAnalyzer;
    
    // New FL Studio 2025 style panels
    std::unique_ptr<OmegaStudio::GUI::RecordToolbar> recordToolbar_;
    std::unique_ptr<OmegaStudio::GUI::LibraryBrowserPanel> libraryPanel_;
    std::unique_ptr<OmegaStudio::GUI::MixerChannelsPanel> mixerPanel_;
    bool showBrowserPanel_{true};
    bool showMixerPanel_{true};
    
    // GUI Windows (FL Studio style)
    std::unique_ptr<OmegaStudio::GUI::PianoRollWindow> pianoRollWindow;
    std::unique_ptr<OmegaStudio::GUI::MixerWindow> mixerWindow;
    std::unique_ptr<OmegaStudio::GUI::PlaylistWindow> playlistWindow;
    std::unique_ptr<OmegaStudio::GUI::ChannelRackWindow> channelRackWindow;
    std::unique_ptr<OmegaStudio::GUI::BrowserWindow> browserWindow;
    std::unique_ptr<OmegaStudio::GUI::AutomationWindow> automationWindow;
    std::unique_ptr<OmegaStudio::GUI::AudioEditorWindow> audioEditorWindow;
    std::unique_ptr<OmegaStudio::GUI::PerformanceModeWindow> performanceModeWindow;
    std::unique_ptr<OmegaStudio::GUI::TransportBar> transportBar;
    std::unique_ptr<OmegaStudio::GUI::MacroPanelComponent> macroPanel;
    std::unique_ptr<OmegaStudio::GUI::StemSeparatorWindow> stemSeparatorWindow;
    std::unique_ptr<OmegaStudio::GUI::SmartMixingAssistantWindow> smartMixingAssistantWindow;
    
    // UI State
    double cpuLoad_{0.0};
    juce::String deviceName_;
    double sampleRate_{0.0};
    int bufferSize_{0};
    
    // Status display
    int numTracks_{0};
    bool projectModified_{false};
    
    // ===== FL STUDIO 2025 COMPLETE SYSTEMS =====
    
    // ===== NEW: FL STUDIO ULTIMATE GUI (ALL SYSTEMS INTEGRATED) =====
    std::unique_ptr<OmegaStudio::GUI::FLStudioUltimateGUI> flStudioUltimateGUI_;
    // Contains: Docking System, Workspace Manager, Status Bar, Context Menus,
    //          Quick Access Toolbar, Animation System, Theme System,
    //          Advanced Playlist, Advanced Mixer, Advanced Browser, Advanced Piano Roll
    
    // AI Services (NEW 2025) - FULLY IMPLEMENTED
    std::unique_ptr<OmegaStudio::AI::StemSeparationService> stemSeparationService_;
    std::unique_ptr<OmegaStudio::AI::LoopStarter> loopStarter_;
    std::unique_ptr<OmegaStudio::AI::ChordGenerator> chordGenerator_;
    std::unique_ptr<OmegaStudio::AI::GopherAssistant> gopherAssistant_;
    
    // Audio Recording & Clips (NEW 2025)
    // std::unique_ptr<OmegaStudio::Audio::AudioRecorder> audioRecorder_;  // Ya existe
    
    // Arrangement & MIDI (Enhanced 2025) - HEADERS READY
    // NOTE: Commented out due to namespace conflicts - implement later
    // std::unique_ptr<OmegaStudio::Arrangement::Playlist> playlist_;
    // std::unique_ptr<OmegaStudio::MIDI::PianoRoll> pianoRoll_;
    
    // Mixer 128 Channels (Enhanced 2025) - HEADER READY
    // std::unique_ptr<OmegaStudio::Mixer::Mixer> mixer128_;
    
    // FL Mobile Rack + Premium Effects (NEW 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::Effects::FLMobileRack> mobileRack_;
    // std::unique_ptr<OmegaStudio::Effects::Emphasis> emphasis_;
    // std::unique_ptr<OmegaStudio::Effects::LuxeVerb> luxeVerb_;
    // std::unique_ptr<OmegaStudio::Effects::GrossBeat> grossBeat_;
    // std::unique_ptr<OmegaStudio::Effects::Vocodex> vocodex_;
    
    // Premium Instruments (NEW 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::Instruments::Sytrus> sytrus_;
    // std::unique_ptr<OmegaStudio::Instruments::Harmor> harmor_;
    // std::unique_ptr<OmegaStudio::Instruments::FLEX> flex_;
    // std::unique_ptr<OmegaStudio::Instruments::Sakura> sakura_;
    
    // Audio Editors (NEW 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::AudioEditors::Edison> edison_;
    // std::unique_ptr<OmegaStudio::AudioEditors::Newtone> newtone_;
    // std::unique_ptr<OmegaStudio::AudioEditors::Newtime> newtime_;
    
    // Visualizers (NEW 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::Visualizers::WaveCandy> waveCandy_;
    // std::unique_ptr<OmegaStudio::Visualizers::FruitySpectroman> spectroman_;
    // std::unique_ptr<OmegaStudio::Visualizers::ZGameEditorVisualizer> zGameEditor_;
    
    // MIDI Advanced (NEW 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::MIDI::MIDIOut> midiOut_;
    // std::unique_ptr<OmegaStudio::MIDI::FruityVoltageController> cvGate_;
    
    // FL Remote & Content (NEW 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::Remote::FLStudioRemote> flRemote_;
    // std::unique_ptr<OmegaStudio::Remote::SoundContentLibrary> contentLibrary_;
    
    // Plugin Hosting (Enhanced 2025) - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::Plugins::PluginHosting> pluginHosting_;
    
    // FL Studio 2025 GUI Components - TODO: Implement .cpp
    // std::unique_ptr<OmegaStudio::GUI::FLStudio2025LookAndFeel> fl2025LookAndFeel_;
    // std::unique_ptr<OmegaStudio::GUI::DockingSystem> dockingSystem_;
    // std::unique_ptr<OmegaStudio::GUI::WorkspaceManager> workspaceManager_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace GUI
} // namespace Omega
