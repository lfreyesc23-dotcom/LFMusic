//==============================================================================
// Application.h
// Main JUCE application class
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <memory>

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
    ~OmegaStudioApplication() override = default;
    
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
    std::unique_ptr<Audio::AudioEngine> audioEngine_;
    std::unique_ptr<GUI::MainWindow> mainWindow_;
};

} // namespace Omega
