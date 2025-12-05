//==============================================================================
// MainWindow.h
// Main application window
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <memory>

namespace Omega {
namespace Audio { class AudioEngine; }
namespace GUI { class MainComponent; }

namespace GUI {

//==============================================================================
// MainWindow
//==============================================================================
class MainWindow : public juce::DocumentWindow {
public:
    MainWindow(const juce::String& name, Audio::AudioEngine* audioEngine);
    ~MainWindow() override;
    
    void closeButtonPressed() override;
    
private:
    std::unique_ptr<MainComponent> mainComponent_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace GUI
} // namespace Omega
