#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Macro Panel - 8 macro knobs with assignment matrix
 */
class MacroPanelComponent : public juce::Component
{
public:
    MacroPanelComponent()
    {
        // Create 8 macro knobs
        for (int i = 0; i < 8; ++i)
        {
            auto* knob = new MacroKnob(i);
            addAndMakeVisible(knob);
            macroKnobs.add(knob);
        }
        
        // Assignment button
        addAndMakeVisible(assignButton);
        assignButton.setButtonText("Assignment Matrix");
        assignButton.onClick = [this]() { showAssignmentMatrix(); };
        
        // MIDI learn toggle
        addAndMakeVisible(midiLearnButton);
        midiLearnButton.setButtonText("MIDI Learn");
        midiLearnButton.setClickingTogglesState(true);
        midiLearnButton.onClick = [this]() {
            midiLearnMode = midiLearnButton.getToggleState();
            if (midiLearnMode)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                         "MIDI Learn",
                                                         "Move a MIDI controller to assign to next touched macro");
            }
        };
        
        // Preset management
        addAndMakeVisible(presetCombo);
        presetCombo.addItem("Default", 1);
        presetCombo.addItem("Custom 1", 2);
        presetCombo.addItem("Custom 2", 3);
        presetCombo.setSelectedId(1);
        
        addAndMakeVisible(savePresetButton);
        savePresetButton.setButtonText("Save Preset");
        savePresetButton.onClick = [this]() { savePreset(); };
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Header
        auto header = bounds.removeFromTop(40);
        assignButton.setBounds(header.removeFromLeft(150).reduced(5));
        midiLearnButton.setBounds(header.removeFromLeft(100).reduced(5));
        header.removeFromLeft(10);
        presetCombo.setBounds(header.removeFromLeft(120).reduced(5));
        savePresetButton.setBounds(header.removeFromLeft(100).reduced(5));
        
        // Macro knobs in 2 rows
        int knobSize = 100;
        int spacing = 10;
        
        for (int row = 0; row < 2; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                int index = row * 4 + col;
                int x = col * (knobSize + spacing) + spacing;
                int y = row * (knobSize + 60) + spacing;
                macroKnobs[index]->setBounds(x, y + 40, knobSize, knobSize + 40);
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("Macro Controls", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    //==============================================================================
    /**
     * Single Macro Knob
     */
    class MacroKnob : public juce::Component
    {
    public:
        MacroKnob(int idx) : index(idx)
        {
            addAndMakeVisible(slider);
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
            slider.setRange(0.0, 1.0, 0.01);
            slider.setValue(0.5);
            slider.onValueChange = [this]() {
                // Trigger all assignments
                for (auto& assignment : assignments)
                {
                    float value = static_cast<float>(slider.getValue());
                    float scaled = assignment.min + (assignment.max - assignment.min) * value;
                    // Apply to target parameter
                }
            };
            
            addAndMakeVisible(nameLabel);
            nameLabel.setText("Macro " + juce::String(index + 1), juce::dontSendNotification);
            nameLabel.setJustificationType(juce::Justification::centred);
            nameLabel.setEditable(true);
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            nameLabel.setBounds(bounds.removeFromTop(20));
            slider.setBounds(bounds);
        }

        void addAssignment(const juce::String& target, float min, float max)
        {
            Assignment a;
            a.targetParameter = target;
            a.min = min;
            a.max = max;
            assignments.add(a);
        }

        int getIndex() const { return index; }

    private:
        struct Assignment
        {
            juce::String targetParameter;
            float min = 0.0f;
            float max = 1.0f;
        };

        int index;
        juce::Slider slider;
        juce::Label nameLabel;
        juce::Array<Assignment> assignments;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroKnob)
    };

    //==============================================================================
    /**
     * Assignment Matrix Window
     */
    class AssignmentMatrixWindow : public juce::DocumentWindow
    {
    public:
        AssignmentMatrixWindow()
            : DocumentWindow("Macro Assignment Matrix",
                             juce::Colour(0xff2b2b2b),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MatrixComponent(), true);
            setResizable(true, true);
            centreWithSize(800, 600);
        }

    private:
        class MatrixComponent : public juce::Component,
                                private juce::TableListBoxModel
        {
        public:
            MatrixComponent()
            {
                addAndMakeVisible(table);
                table.setModel(this);
                table.getHeader().addColumn("Macro", 1, 80);
                table.getHeader().addColumn("Target Parameter", 2, 200);
                table.getHeader().addColumn("Min", 3, 80);
                table.getHeader().addColumn("Max", 4, 80);
                table.getHeader().addColumn("Curve", 5, 100);
                table.getHeader().addColumn("Actions", 6, 100);
                
                // Add some example rows
                for (int i = 0; i < 10; ++i)
                {
                    assignments.add({i % 8, "Parameter " + juce::String(i), 0.0f, 1.0f, "Linear"});
                }
                
                addAndMakeVisible(addButton);
                addButton.setButtonText("Add Assignment");
                addButton.onClick = [this]() {
                    assignments.add({0, "New Parameter", 0.0f, 1.0f, "Linear"});
                    table.updateContent();
                };
            }

            void resized() override
            {
                auto bounds = getLocalBounds();
                addButton.setBounds(bounds.removeFromTop(30).reduced(5));
                table.setBounds(bounds);
            }

            int getNumRows() override
            {
                return assignments.size();
            }

            void paintRowBackground(juce::Graphics& g, int, int, int, bool rowIsSelected) override
            {
                if (rowIsSelected)
                    g.fillAll(juce::Colour(0xff3a3a3a));
            }

            void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool) override
            {
                if (rowNumber >= assignments.size())
                    return;
                
                auto& assignment = assignments.getReference(rowNumber);
                
                g.setColour(juce::Colours::white);
                juce::String text;
                
                switch (columnId)
                {
                    case 1: text = "Macro " + juce::String(assignment.macroIndex + 1); break;
                    case 2: text = assignment.targetParameter; break;
                    case 3: text = juce::String(assignment.minValue, 2); break;
                    case 4: text = juce::String(assignment.maxValue, 2); break;
                    case 5: text = assignment.curve; break;
                }
                
                g.drawText(text, 5, 0, width - 10, height, juce::Justification::centredLeft, true);
            }

        private:
            struct Assignment
            {
                int macroIndex;
                juce::String targetParameter;
                float minValue;
                float maxValue;
                juce::String curve;
            };

            juce::TableListBox table;
            juce::TextButton addButton;
            juce::Array<Assignment> assignments;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MatrixComponent)
        };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AssignmentMatrixWindow)
    };

    void showAssignmentMatrix()
    {
        if (matrixWindow == nullptr)
        {
            matrixWindow = std::make_unique<AssignmentMatrixWindow>();
            matrixWindow->setVisible(true);
        }
        else
        {
            matrixWindow->toFront(true);
        }
    }

    void savePreset()
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                 "Save Preset",
                                                 "Macro preset saved!");
    }

    juce::OwnedArray<MacroKnob> macroKnobs;
    juce::TextButton assignButton;
    juce::TextButton midiLearnButton;
    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton;
    
    bool midiLearnMode = false;
    std::unique_ptr<AssignmentMatrixWindow> matrixWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroPanelComponent)
};

} // namespace GUI
} // namespace OmegaStudio
