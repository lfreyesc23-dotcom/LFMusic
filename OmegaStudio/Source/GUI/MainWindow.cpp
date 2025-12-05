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
    
    mainComponent_ = std::make_unique<MainComponent>(audioEngine);
    setContentOwned(mainComponent_.get(), true);
    
    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
    #endif
    
    setVisible(true);
}

//==============================================================================
MainWindow::~MainWindow() {
    mainComponent_ = nullptr;
}

//==============================================================================
void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace Omega::GUI
