#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Performance Mode - Ableton Live style clip launcher
 */
class PerformanceModeWindow : public juce::DocumentWindow
{
public:
    PerformanceModeWindow()
        : DocumentWindow("Performance Mode",
                         juce::Colour(0xff2b2b2b),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new PerformanceComponent(), true);
        setResizable(true, true);
        centreWithSize(1000, 700);
    }

private:
    //==============================================================================
    /**
     * Clip Data
     */
    struct Clip
    {
        juce::String name;
        juce::Colour color;
        bool isRecording = false;
        bool isPlaying = false;
        bool isEmpty = true;
        double length = 4.0; // bars
        
        enum class FollowAction
        {
            None,
            Stop,
            PlayNext,
            PlayPrevious,
            PlayRandom,
            Loop
        };
        
        FollowAction followAction = FollowAction::Loop;
        int followActionBars = 4;
        
        Clip()
            : color(juce::Colour::fromHSV(juce::Random::getSystemRandom().nextFloat(), 0.7f, 0.8f, 1.0f))
        {}
    };

    //==============================================================================
    /**
     * Clip Slot Component
     */
    class ClipSlot : public juce::Component,
                     private juce::Timer
    {
    public:
        ClipSlot(int trackIdx, int sceneIdx)
            : trackIndex(trackIdx), sceneIndex(sceneIdx)
        {
            startTimerHz(30); // Animation
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(2.0f);
            
            if (clip.isEmpty)
            {
                // Empty slot
                g.setColour(juce::Colour(0xff2a2a2a));
                g.fillRoundedRectangle(bounds, 4.0f);
                
                g.setColour(juce::Colours::grey.darker());
                g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
            }
            else
            {
                // Filled slot
                juce::Colour slotColor = clip.color;
                
                if (clip.isPlaying)
                {
                    // Animated playing state
                    float alpha = (std::sin(playPhase) * 0.3f + 0.7f);
                    slotColor = slotColor.withMultipliedAlpha(alpha);
                }
                
                g.setColour(slotColor);
                g.fillRoundedRectangle(bounds, 4.0f);
                
                // Border
                g.setColour(slotColor.brighter());
                g.drawRoundedRectangle(bounds, 4.0f, 2.0f);
                
                // Clip name
                g.setColour(juce::Colours::white);
                g.drawText(clip.name.isEmpty() ? "Clip" : clip.name,
                           bounds.reduced(5.0f),
                           juce::Justification::topLeft);
                
                // Status indicators
                if (clip.isRecording)
                {
                    g.setColour(juce::Colours::red);
                    g.fillEllipse(bounds.getRight() - 15.0f, bounds.getY() + 5.0f, 10.0f, 10.0f);
                }
            }
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown())
            {
                showContextMenu();
            }
            else if (e.mods.isCommandDown())
            {
                // Record clip
                clip.isEmpty = false;
                clip.isRecording = !clip.isRecording;
                repaint();
            }
            else
            {
                // Play/stop clip
                if (!clip.isEmpty)
                {
                    clip.isPlaying = !clip.isPlaying;
                    repaint();
                }
            }
        }

        void timerCallback() override
        {
            if (clip.isPlaying)
            {
                playPhase += 0.1f;
                repaint();
            }
        }

        bool isPlaying() const { return clip.isPlaying; }
        int getTrackIndex() const { return trackIndex; }
        int getSceneIndex() const { return sceneIndex; }

    private:
        void showContextMenu()
        {
            juce::PopupMenu menu;
            menu.addItem(1, "Rename");
            menu.addItem(2, "Change Color");
            menu.addItem(3, "Set Length");
            menu.addSeparator();
            menu.addItem(4, "Duplicate");
            menu.addItem(5, "Delete");
            menu.addSeparator();
            
            juce::PopupMenu followMenu;
            followMenu.addItem(10, "None", true, clip.followAction == Clip::FollowAction::None);
            followMenu.addItem(11, "Stop", true, clip.followAction == Clip::FollowAction::Stop);
            followMenu.addItem(12, "Play Next", true, clip.followAction == Clip::FollowAction::PlayNext);
            followMenu.addItem(13, "Play Previous", true, clip.followAction == Clip::FollowAction::PlayPrevious);
            followMenu.addItem(14, "Play Random", true, clip.followAction == Clip::FollowAction::PlayRandom);
            followMenu.addItem(15, "Loop", true, clip.followAction == Clip::FollowAction::Loop);
            menu.addSubMenu("Follow Action", followMenu);
            
            menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result)
            {
                handleMenuResult(result);
            });
        }

        void handleMenuResult(int result)
        {
            switch (result)
            {
                case 1: // Rename
                    clip.name = "Renamed Clip";
                    repaint();
                    break;
                case 2: // Change color
                    clip.color = juce::Colour::fromHSV(juce::Random::getSystemRandom().nextFloat(), 0.7f, 0.8f, 1.0f);
                    repaint();
                    break;
                case 5: // Delete
                    clip.isEmpty = true;
                    clip.isPlaying = false;
                    repaint();
                    break;
                case 10: clip.followAction = Clip::FollowAction::None; break;
                case 11: clip.followAction = Clip::FollowAction::Stop; break;
                case 12: clip.followAction = Clip::FollowAction::PlayNext; break;
                case 13: clip.followAction = Clip::FollowAction::PlayPrevious; break;
                case 14: clip.followAction = Clip::FollowAction::PlayRandom; break;
                case 15: clip.followAction = Clip::FollowAction::Loop; break;
            }
        }

        Clip clip;
        int trackIndex;
        int sceneIndex;
        float playPhase = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSlot)
    };

    //==============================================================================
    /**
     * Scene Launch Button
     */
    class SceneLauncher : public juce::Component
    {
    public:
        SceneLauncher(int idx) : sceneIndex(idx) {}

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat().reduced(2.0f);
            
            g.setColour(juce::Colour(0xffff8800));
            g.fillRoundedRectangle(bounds, 4.0f);
            
            g.setColour(juce::Colours::white);
            g.drawText(juce::String(sceneIndex + 1), bounds, juce::Justification::centred);
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            if (onLaunch)
                onLaunch();
        }

        std::function<void()> onLaunch;

    private:
        int sceneIndex;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SceneLauncher)
    };

    //==============================================================================
    /**
     * Main Performance Component
     */
    class PerformanceComponent : public juce::Component
    {
    public:
        PerformanceComponent()
        {
            // Create 8 tracks x 8 scenes grid
            for (int track = 0; track < 8; ++track)
            {
                for (int scene = 0; scene < 8; ++scene)
                {
                    auto* slot = new ClipSlot(track, scene);
                    addAndMakeVisible(slot);
                    clipSlots.add(slot);
                }
            }
            
            // Scene launchers
            for (int i = 0; i < 8; ++i)
            {
                auto* launcher = new SceneLauncher(i);
                launcher->onLaunch = [this, i]() { launchScene(i); };
                addAndMakeVisible(launcher);
                sceneLaunchers.add(launcher);
            }
            
            // Control buttons
            addAndMakeVisible(stopAllButton);
            stopAllButton.setButtonText("Stop All Clips");
            stopAllButton.onClick = [this]() { stopAllClips(); };
            
            addAndMakeVisible(quantizeButton);
            quantizeButton.setButtonText("Quantize: 1 Bar");
            quantizeButton.onClick = [this]() { cycleQuantize(); };
            
            addAndMakeVisible(midiMapButton);
            midiMapButton.setButtonText("MIDI Map");
            midiMapButton.onClick = [this]() { toggleMidiMap(); };
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            
            // Controls
            auto controls = bounds.removeFromTop(40);
            stopAllButton.setBounds(controls.removeFromLeft(120).reduced(5));
            quantizeButton.setBounds(controls.removeFromLeft(120).reduced(5));
            midiMapButton.setBounds(controls.removeFromLeft(100).reduced(5));
            
            // Scene launchers
            auto launcherArea = bounds.removeFromRight(60);
            int sceneHeight = launcherArea.getHeight() / 8;
            for (int i = 0; i < 8; ++i)
            {
                sceneLaunchers[i]->setBounds(launcherArea.removeFromTop(sceneHeight));
            }
            
            // Clip grid
            int slotWidth = bounds.getWidth() / 8;
            int slotHeight = bounds.getHeight() / 8;
            
            for (int scene = 0; scene < 8; ++scene)
            {
                for (int track = 0; track < 8; ++track)
                {
                    int index = scene * 8 + track;
                    clipSlots[index]->setBounds(track * slotWidth, scene * slotHeight, slotWidth, slotHeight);
                }
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff1a1a1a));
        }

    private:
        void launchScene(int sceneIndex)
        {
            // Launch all clips in scene
            for (int track = 0; track < 8; ++track)
            {
                int index = sceneIndex * 8 + track;
                // Trigger clip playback
            }
        }

        void stopAllClips()
        {
            for (auto* slot : clipSlots)
            {
                // Stop all playing clips
            }
        }

        void cycleQuantize()
        {
            quantizeValue = (quantizeValue % 4) + 1;
            quantizeButton.setButtonText("Quantize: " + juce::String(quantizeValue) + " Bar" + (quantizeValue > 1 ? "s" : ""));
        }

        void toggleMidiMap()
        {
            midiMapMode = !midiMapMode;
            midiMapButton.setToggleState(midiMapMode, juce::dontSendNotification);
        }

        juce::OwnedArray<ClipSlot> clipSlots;
        juce::OwnedArray<SceneLauncher> sceneLaunchers;
        
        juce::TextButton stopAllButton;
        juce::TextButton quantizeButton;
        juce::TextButton midiMapButton;
        
        int quantizeValue = 1;
        bool midiMapMode = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceComponent)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceModeWindow)
};

} // namespace GUI
} // namespace OmegaStudio
