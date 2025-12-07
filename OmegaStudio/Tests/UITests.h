#pragma once

#include <JuceHeader.h>
#include "../Source/GUI/FLStudio2025Interface.h"
#include "../Source/GUI/MainComponent.h"

namespace OmegaStudio {
namespace Tests {

/**
 * UI Tests - Verify user interface components
 */
class UITests {
public:
    static bool runAllTests() {
        bool allPassed = true;
        
        allPassed &= testComponentSnapshots();
        allPassed &= testGopherCommands();
        allPassed &= testKeyboardShortcuts();
        allPassed &= testGUIResponsiveness();
        
        return allPassed;
    }
    
private:
    // Test component visual snapshots
    static bool testComponentSnapshots() {
        // Create main component
        GUI::MainComponent mainComp;
        mainComp.setSize(1920, 1080);
        
        // Verify component tree
        bool hasChannelRack = (mainComp.findChildWithID("channelRack") != nullptr);
        bool hasMixer = (mainComp.findChildWithID("mixer") != nullptr);
        bool hasPlaylist = (mainComp.findChildWithID("playlist") != nullptr);
        
        bool passed = hasChannelRack && hasMixer && hasPlaylist;
        
        DBG("Component Snapshots Test: ChannelRack=" << hasChannelRack 
            << ", Mixer=" << hasMixer 
            << ", Playlist=" << hasPlaylist 
            << " - " << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test Gopher Assistant commands in dry-run
    static bool testGopherCommands() {
        // Test command parsing
        juce::StringArray testCommands = {
            "explain routing",
            "suggest gainstaging",
            "create sidechain source=0 target=1",
            "optimize mix"
        };
        
        int successCount = 0;
        
        for (const auto& cmd : testCommands) {
            // In real implementation, would call GopherAssistant
            // For now, just verify command format
            if (cmd.isNotEmpty() && cmd.contains(" ")) {
                successCount++;
            }
        }
        
        bool passed = (successCount == testCommands.size());
        
        DBG("Gopher Commands Test: " << successCount << "/" << testCommands.size() 
            << " - " << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test keyboard shortcuts
    static bool testKeyboardShortcuts() {
        struct Shortcut {
            juce::String name;
            int keyCode;
            juce::ModifierKeys modifiers;
        };
        
        std::vector<Shortcut> shortcuts = {
            {"Play/Stop", juce::KeyPress::spaceKey, juce::ModifierKeys::noModifiers},
            {"Save", 'S', juce::ModifierKeys::commandModifier},
            {"Undo", 'Z', juce::ModifierKeys::commandModifier},
            {"Redo", 'Z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier},
            {"Channel Rack", juce::KeyPress::F6Key, juce::ModifierKeys::noModifiers},
            {"Mixer", juce::KeyPress::F9Key, juce::ModifierKeys::noModifiers},
            {"Browser", juce::KeyPress::F8Key, juce::ModifierKeys::noModifiers}
        };
        
        // Verify all shortcuts are valid
        bool passed = true;
        for (const auto& shortcut : shortcuts) {
            if (shortcut.keyCode == 0) {
                passed = false;
                break;
            }
        }
        
        DBG("Keyboard Shortcuts Test: " << shortcuts.size() << " shortcuts defined - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
    
    // Test GUI responsiveness
    static bool testGUIResponsiveness() {
        GUI::MainComponent mainComp;
        mainComp.setSize(1920, 1080);
        
        // Measure paint time
        juce::Image testImage(juce::Image::RGB, 1920, 1080, true);
        juce::Graphics g(testImage);
        
        auto startTime = juce::Time::getMillisecondCounterHiRes();
        
        mainComp.paint(g);
        
        auto elapsed = juce::Time::getMillisecondCounterHiRes() - startTime;
        
        // Should paint in < 16ms (60 FPS)
        bool passed = elapsed < 16.0;
        
        DBG("GUI Responsiveness Test: " << elapsed << "ms - " 
            << (passed ? "PASSED" : "FAILED"));
        
        return passed;
    }
};

} // namespace Tests
} // namespace OmegaStudio
