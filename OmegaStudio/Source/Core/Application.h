//==============================================================================
// Application.h
// Main JUCE application class
//==============================================================================

#pragma once

#include <JuceHeader.h>
// #include "../GUI/FLStudioCompleteGUI.h" // Temporarily disabled

namespace Omega {

// Forward declarations
namespace Audio { class AudioEngine; }

//==============================================================================
// Main Application Window
//==============================================================================
class MainApplicationWindow : public juce::DocumentWindow {
public:
    MainApplicationWindow(const juce::String& name)
        : juce::DocumentWindow(name,
                               juce::Colour(0xff2d2d2d),
                               juce::DocumentWindow::allButtons) {
        
        setUsingNativeTitleBar(true);
        // mainComponent = std::make_unique<OmegaStudio::GUI::FLStudioCompleteWindow>();
        // setContentOwned(mainComponent.get(), true);
        
        // Simple placeholder component
        auto* content = new juce::Component();
        content->setSize(1200, 800);
        setContentOwned(content, true);
        
        #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
        #else
            setResizable(true, true);
            centreWithSize(1200, 800);
        #endif
        
        setVisible(true);
    }
    
    void closeButtonPressed() override {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    
private:
    // std::unique_ptr<OmegaStudio::GUI::FLStudioCompleteWindow> mainComponent;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainApplicationWindow)
};

//==============================================================================
// OmegaStudio Application
//==============================================================================
class OmegaStudioApplication : public juce::JUCEApplication {
public:
    OmegaStudioApplication() = default;
    
    //==========================================================================
    // JUCEApplication Interface
    //==========================================================================
    const juce::String getApplicationName() override { return "OmegaStudio - FL Edition"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    
    //==========================================================================
    // Lifecycle
    //==========================================================================
    void initialise(const juce::String& /*commandLine*/) override {
        mainWindow.reset(new MainApplicationWindow(getApplicationName()));
    }
    
    void shutdown() override {
        mainWindow = nullptr;
    }
    
    void systemRequestedQuit() override {
        quit();
    }
    
    void anotherInstanceStarted(const juce::String& /*commandLine*/) override {
        // Handle when another instance starts
    }
    
private:
    std::unique_ptr<MainApplicationWindow> mainWindow;
};

} // namespace Omega
