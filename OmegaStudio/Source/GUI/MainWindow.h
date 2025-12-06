//==============================================================================
// MainWindow.h
// Main application window - FL STUDIO 2025 INTERFACE
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace Omega {
namespace Audio { class AudioEngine; }
namespace GUI { 
    class MainComponent; 
    class FLStudio2025MainWindow;
}

namespace GUI {

//==============================================================================
// MainWindow - NOW WITH FL STUDIO 2025 INTERFACE
//==============================================================================
class MainWindow : public juce::DocumentWindow {
public:
    MainWindow(const juce::String& name, Audio::AudioEngine* audioEngine);
    ~MainWindow() override;
    
    void closeButtonPressed() override;
    
private:
    FLStudio2025MainWindow* flStudioInterface_;  // NEW: FL Studio 2025 interface
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace GUI
} // namespace Omega
