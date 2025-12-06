//==============================================================================
// MainWindow.cpp - FL STUDIO 2025 INTERFACE INTEGRATION
//==============================================================================

#include "MainWindow.h"
#include "FLStudio2025Interface.h"
#include "MainComponent.h"
#include "../Utils/Constants.h"

namespace Omega::GUI {

//==============================================================================
MainWindow::MainWindow(const juce::String& name, Audio::AudioEngine* audioEngine)
    : DocumentWindow(name,
                     FLColors::DarkBg,  // FL Studio dark background
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    
    // Create FL Studio 2025 interface and give ownership to DocumentWindow
    flStudioInterface_ = new FLStudio2025MainWindow();
    setContentOwned(flStudioInterface_, true);
    
    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, true);
        // FL Studio-style default size (Full HD optimized)
        centreWithSize(1920, 1080);
        
        // Allow minimum size for proper FL Studio layout
        setResizeLimits(1280, 720, 3840, 2160);
    #endif
    
    setVisible(true);
}

//==============================================================================
MainWindow::~MainWindow() {
    // flStudioInterface_ is owned and destroyed by DocumentWindow
}

//==============================================================================
void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace Omega::GUI
