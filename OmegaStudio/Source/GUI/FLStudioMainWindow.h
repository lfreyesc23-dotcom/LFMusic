//==============================================================================
// FLStudioMainWindow.h
// Interfaz completa estilo FL Studio con TODAS las funciones
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "FLStudioLookAndFeel.h"
#include "ChannelRackUI.h"
#include "../Sequencer/ChannelRack.h"

// Forward declarations
namespace Omega {
    namespace Audio { class AudioEngine; }
}

namespace OmegaStudio {
    namespace Audio { class AudioEngine; }
}

namespace Omega {
namespace GUI {

//==============================================================================
// Top Toolbar - Barra superior con logo, menú y controles
//==============================================================================
class FLTopToolbar : public juce::Component {
public:
    FLTopToolbar();
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    std::function<void()> onNewProject;
    std::function<void()> onOpenProject;
    std::function<void()> onSaveProject;
    std::function<void()> onExportAudio;
    
private:
    std::unique_ptr<juce::TextButton> newButton_;
    std::unique_ptr<juce::TextButton> openButton_;
    std::unique_ptr<juce::TextButton> saveButton_;
    std::unique_ptr<juce::TextButton> exportButton_;
    std::unique_ptr<juce::Label> cpuLabel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLTopToolbar)
};

//==============================================================================
// Side Panel - Panel lateral para navegación
//==============================================================================
class FLSidePanel : public juce::Component {
public:
    FLSidePanel();
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    enum class View {
        ChannelRack,
        Playlist,
        PianoRoll,
        Mixer,
        Browser,
        Recording
    };
    
    std::function<void(View)> onViewSelected;
    
private:
    struct ViewButton : public juce::TextButton {
        ViewButton(const juce::String& name, View v) 
            : juce::TextButton(name), view(v) {}
        View view;
    };
    
    juce::OwnedArray<ViewButton> viewButtons_;
    View currentView_ = View::ChannelRack;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLSidePanel)
};

//==============================================================================
// Transport Bar - Controles de reproducción
//==============================================================================
class FLTransportBar : public juce::Component, private juce::Timer {
public:
    FLTransportBar();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    std::function<void(bool)> onPlayPause;
    std::function<void()> onStop;
    std::function<void(bool)> onRecord;
    std::function<void(bool)> onLoop;
    std::function<void(double)> onTempoChanged;
    
    void setPlaying(bool playing);
    void setRecording(bool recording);
    void setLooping(bool looping);
    void setTempo(double bpm);
    void setPosition(double beats);
    
private:
    std::unique_ptr<juce::TextButton> playButton_;
    std::unique_ptr<juce::TextButton> stopButton_;
    std::unique_ptr<juce::TextButton> recordButton_;
    std::unique_ptr<juce::TextButton> loopButton_;
    std::unique_ptr<juce::Slider> tempoSlider_;
    std::unique_ptr<juce::Label> tempoLabel_;
    std::unique_ptr<juce::Label> positionLabel_;
    
    bool isPlaying_ = false;
    bool isRecording_ = false;
    bool isLooping_ = true;
    double tempo_ = 140.0;
    double position_ = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLTransportBar)
};

//==============================================================================
// Browser Panel - Navegador de archivos y presets
//==============================================================================
class FLBrowserPanel : public juce::Component {
public:
    FLBrowserPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    std::function<void(juce::File)> onFileSelected;
    
private:
    std::unique_ptr<juce::TreeView> fileTree_;
    std::unique_ptr<juce::TextEditor> searchBox_;
    std::unique_ptr<juce::ListBox> favoritesBox_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLBrowserPanel)
};

//==============================================================================
// Recording Panel - Panel de grabación de audio/MIDI
//==============================================================================
class FLRecordingPanel : public juce::Component, private juce::Timer {
public:
    FLRecordingPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    std::function<void()> onStartRecording;
    std::function<void()> onStopRecording;
    std::function<void()> onPlayRecording;
    
private:
    std::unique_ptr<juce::TextButton> recordButton_;
    std::unique_ptr<juce::TextButton> playButton_;
    std::unique_ptr<juce::TextButton> stopButton_;
    std::unique_ptr<juce::ComboBox> inputSelector_;
    std::unique_ptr<juce::Label> levelMeter_;
    std::unique_ptr<juce::Label> timeLabel_;
    
    bool isRecording_ = false;
    float inputLevel_ = 0.0f;
    double recordTime_ = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLRecordingPanel)
};

//==============================================================================
// Main Window FL Studio Style - Ventana principal completa
//==============================================================================
class FLStudioMainWindow : public juce::Component {
public:
    FLStudioMainWindow();
    ~FLStudioMainWindow() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Audio Engine integration
    void setAudioEngine(Omega::Audio::AudioEngine* engine) { audioEngine_ = engine; }
    
    // Cambiar entre vistas
    void showChannelRack();
    void showPlaylist();
    void showPianoRoll();
    void showMixer();
    void showBrowser();
    void showRecording();
    
private:
    FLStudioLookAndFeel flLookAndFeel_;
    
    // UI Components
    std::unique_ptr<FLTopToolbar> topToolbar_;
    std::unique_ptr<FLSidePanel> sidePanel_;
    std::unique_ptr<FLTransportBar> transportBar_;
    
    // Views (solo una visible a la vez)
    std::unique_ptr<ChannelRackUI> channelRackUI_;
    std::unique_ptr<FLBrowserPanel> browserPanel_;
    std::unique_ptr<FLRecordingPanel> recordingPanel_;
    
    // Backend engines
    Omega::Audio::AudioEngine* audioEngine_ = nullptr;
    std::unique_ptr<OmegaStudio::Sequencer::ChannelRackEngine> channelRackEngine_;
    
    // Current view
    juce::Component* currentView_ = nullptr;
    
    void setCurrentView(juce::Component* view);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudioMainWindow)
};

} // namespace GUI
} // namespace Omega
