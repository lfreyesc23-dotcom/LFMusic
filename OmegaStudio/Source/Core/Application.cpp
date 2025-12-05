//==============================================================================
// Application.cpp
// Main application implementation
//==============================================================================

#include "Application.h"
#include "../Audio/Engine/AudioEngine.h"
#include "../GUI/MainWindow.h"
#include "../Utils/Constants.h"

namespace Omega {

//==============================================================================
void OmegaStudioApplication::initialise(const juce::String& commandLine) {
    juce::ignoreUnused(commandLine);
    
    juce::Logger::writeToLog("===========================================");
    juce::Logger::writeToLog("   OMEGA STUDIO - Next-Gen DAW");
    juce::Logger::writeToLog("   Version 1.0.0");
    juce::Logger::writeToLog("===========================================");
    
    // Initialize audio engine
    audioEngine_ = std::make_unique<Audio::AudioEngine>();
    
    Audio::AudioEngineConfig config;
    config.sampleRate = Audio::DEFAULT_SAMPLE_RATE;
    config.bufferSize = Audio::DEFAULT_BUFFER_SIZE;
    config.numInputChannels = Audio::DEFAULT_INPUT_CHANNELS;
    config.numOutputChannels = Audio::DEFAULT_OUTPUT_CHANNELS;
    
    if (!audioEngine_->initialize(config)) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Audio Initialization Failed",
            "Could not initialize audio engine: " + audioEngine_->getLastError(),
            "OK"
        );
    } else {
        audioEngine_->start();
        juce::Logger::writeToLog("Audio engine started successfully");
    }
    
    // Create main window
    mainWindow_ = std::make_unique<GUI::MainWindow>(
        getApplicationName(),
        audioEngine_.get()
    );
    mainWindow_->setVisible(true);
}

//==============================================================================
void OmegaStudioApplication::shutdown() {
    juce::Logger::writeToLog("Shutting down Omega Studio...");
    
    // Destroy window first
    mainWindow_.reset();
    
    // Then shutdown audio
    if (audioEngine_) {
        audioEngine_->shutdown();
        audioEngine_.reset();
    }
    
    juce::Logger::writeToLog("Shutdown complete");
}

//==============================================================================
void OmegaStudioApplication::systemRequestedQuit() {
    quit();
}

//==============================================================================
void OmegaStudioApplication::anotherInstanceStarted(const juce::String& commandLine) {
    juce::ignoreUnused(commandLine);
}

//==============================================================================
Audio::AudioEngine* OmegaStudioApplication::getAudioEngine() noexcept {
    return audioEngine_.get();
}

} // namespace Omega
