//==============================================================================
// Application.h
// Main JUCE application class
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace Omega {

// Forward declarations
namespace Audio { class AudioEngine; }
namespace GUI { class MainWindow; }

//==============================================================================
// OmegaStudio Application
//==============================================================================
class OmegaStudioApplication : public juce::JUCEApplication {
public:
    OmegaStudioApplication() = default;
    ~OmegaStudioApplication() override;
    
    //==========================================================================
    // JUCEApplication Interface
    //==========================================================================
    const juce::String getApplicationName() override { return "Omega Studio"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    
    //==========================================================================
    // Lifecycle
    //==========================================================================
    void initialise(const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted(const juce::String& commandLine) override;
    
    //==========================================================================
    // Accessors
    //==========================================================================
    [[nodiscard]] Audio::AudioEngine* getAudioEngine() noexcept;
    
private:
    Audio::AudioEngine* audioEngine_ = nullptr;
    GUI::MainWindow* mainWindow_ = nullptr;
};

} // namespace Omega
