//==============================================================================
// MainWindow.cpp
//==============================================================================

#include "MainWindow.h"
#include "MainComponent.h"
#include "../Utils/Constants.h"

namespace Omega::GUI {

//==============================================================================
MainWindow::MainWindow(const juce::String& name, Audio::AudioEngine* audioEngine)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    
    // Create MainComponent and give ownership to DocumentWindow
    mainComponent_ = new MainComponent(audioEngine);
    setContentOwned(mainComponent_, true);
    
    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, true);
        // Use reasonable default size (not full 1920x1080)
        centreWithSize(1400, 900);
    #endif
    
    setVisible(true);
}

//==============================================================================
MainWindow::~MainWindow() {
    // mainComponent_ is owned and destroyed by DocumentWindow
}

//==============================================================================
void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace Omega::GUI
