#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <map>

namespace OmegaStudio {
namespace Audio {

/**
 * @struct MacroParameter
 * @brief Parámetro macro que controla múltiples parámetros
 */
struct MacroParameter {
    juce::String name { "Macro 1" };
    float value { 0.0f };  // 0.0 - 1.0
    
    struct Mapping {
        juce::AudioProcessorParameter* param { nullptr };
        float minValue { 0.0f };
        float maxValue { 1.0f };
        bool inverted { false };
        
        float getMappedValue(float macroValue) const {
            float v = inverted ? (1.0f - macroValue) : macroValue;
            return juce::jmap(v, minValue, maxValue);
        }
    };
    
    std::vector<Mapping> mappings;
    
    void setValue(float v) {
        value = juce::jlimit(0.0f, 1.0f, v);
        
        // Update all mapped parameters
        for (auto& mapping : mappings) {
            if (mapping.param != nullptr) {
                float mappedValue = mapping.getMappedValue(value);
                mapping.param->setValue(mappedValue);
            }
        }
    }
    
    void addMapping(juce::AudioProcessorParameter* param, 
                   float min = 0.0f, 
                   float max = 1.0f,
                   bool invert = false) {
        Mapping m;
        m.param = param;
        m.minValue = min;
        m.maxValue = max;
        m.inverted = invert;
        mappings.push_back(m);
    }
};

/**
 * @class AudioRack
 * @brief Contenedor de plugins con macros
 */
class AudioRack {
public:
    AudioRack(const juce::String& name = "Rack") 
        : name_(name) {
        
        // Initialize 8 macros
        for (int i = 0; i < 8; ++i) {
            MacroParameter macro;
            macro.name = "Macro " + juce::String(i + 1);
            macros_.push_back(macro);
        }
    }
    
    void setName(const juce::String& name) { name_ = name; }
    juce::String getName() const { return name_; }
    
    /**
     * Agrega plugin al rack
     */
    void addProcessor(std::unique_ptr<juce::AudioProcessor> processor) {
        if (processor == nullptr) return;
        
        processors_.push_back(std::move(processor));
        processorOrder_.push_back(processors_.size() - 1);
    }
    
    /**
     * Reordena procesadores (serial routing)
     */
    void reorderProcessor(int fromIndex, int toIndex) {
        if (fromIndex < 0 || fromIndex >= (int)processorOrder_.size() ||
            toIndex < 0 || toIndex >= (int)processorOrder_.size()) {
            return;
        }
        
        int processorIndex = processorOrder_[fromIndex];
        processorOrder_.erase(processorOrder_.begin() + fromIndex);
        processorOrder_.insert(processorOrder_.begin() + toIndex, processorIndex);
    }
    
    /**
     * Procesa audio a través del rack
     */
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
        if (!enabled_) return;
        
        for (int idx : processorOrder_) {
            if (idx < (int)processors_.size() && processors_[idx] != nullptr) {
                auto* processor = processors_[idx].get();
                
                if (processor->isSuspended()) continue;
                
                processor->processBlock(buffer, midiMessages);
            }
        }
    }
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        for (auto& processor : processors_) {
            if (processor != nullptr) {
                processor->prepareToPlay(sampleRate, samplesPerBlock);
            }
        }
    }
    
    void releaseResources() {
        for (auto& processor : processors_) {
            if (processor != nullptr) {
                processor->releaseResources();
            }
        }
    }
    
    /**
     * Macros
     */
    MacroParameter& getMacro(int index) {
        jassert(index >= 0 && index < 8);
        return macros_[index];
    }
    
    void setMacroValue(int index, float value) {
        if (index >= 0 && index < 8) {
            macros_[index].setValue(value);
        }
    }
    
    float getMacroValue(int index) const {
        return (index >= 0 && index < 8) ? macros_[index].value : 0.0f;
    }
    
    /**
     * Mapea parámetro a macro
     */
    void mapParameterToMacro(int macroIndex, 
                            int processorIndex,
                            int parameterIndex,
                            float minValue = 0.0f,
                            float maxValue = 1.0f,
                            bool inverted = false) {
        if (macroIndex < 0 || macroIndex >= 8) return;
        if (processorIndex < 0 || processorIndex >= (int)processors_.size()) return;
        
        auto* processor = processors_[processorIndex].get();
        if (processor == nullptr) return;
        
        auto& params = processor->getParameters();
        if (parameterIndex < 0 || parameterIndex >= params.size()) return;
        
        macros_[macroIndex].addMapping(params[parameterIndex], 
                                       minValue, maxValue, inverted);
    }
    
    /**
     * Preset management
     */
    void savePreset(const juce::File& file) {
        juce::XmlElement xml("RackPreset");
        xml.setAttribute("name", name_);
        
        // Save macros
        auto* macrosXml = xml.createNewChildElement("Macros");
        for (int i = 0; i < 8; ++i) {
            auto* macroXml = macrosXml->createNewChildElement("Macro");
            macroXml->setAttribute("index", i);
            macroXml->setAttribute("name", macros_[i].name);
            macroXml->setAttribute("value", macros_[i].value);
            
            // Save mappings
            for (const auto& mapping : macros_[i].mappings) {
                auto* mappingXml = macroXml->createNewChildElement("Mapping");
                // Store parameter identifier (simplified for now)
                mappingXml->setAttribute("min", mapping.minValue);
                mappingXml->setAttribute("max", mapping.maxValue);
                mappingXml->setAttribute("inverted", mapping.inverted);
            }
        }
        
        // Save processor states
        auto* processorsXml = xml.createNewChildElement("Processors");
        for (size_t i = 0; i < processors_.size(); ++i) {
            if (processors_[i] != nullptr) {
                auto* procXml = processorsXml->createNewChildElement("Processor");
                procXml->setAttribute("index", (int)i);
                procXml->setAttribute("name", processors_[i]->getName());
                
                juce::MemoryBlock state;
                processors_[i]->getStateInformation(state);
                procXml->setAttribute("state", state.toBase64Encoding());
            }
        }
        
        xml.writeTo(file);
    }
    
    void loadPreset(const juce::File& file) {
        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr) return;
        
        name_ = xml->getStringAttribute("name", "Rack");
        
        // Load macros
        if (auto* macrosXml = xml->getChildByName("Macros")) {
            for (auto* macroXml : macrosXml->getChildIterator()) {
                int index = macroXml->getIntAttribute("index");
                if (index >= 0 && index < 8) {
                    macros_[index].name = macroXml->getStringAttribute("name");
                    macros_[index].value = macroXml->getDoubleAttribute("value");
                }
            }
        }
        
        // Load processor states (simplified)
        // Full implementation would restore processor instances
    }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    int getNumProcessors() const { return processors_.size(); }
    juce::AudioProcessor* getProcessor(int index) {
        return (index >= 0 && index < (int)processors_.size()) 
            ? processors_[index].get() : nullptr;
    }
    
private:
    juce::String name_;
    std::vector<std::unique_ptr<juce::AudioProcessor>> processors_;
    std::vector<int> processorOrder_;  // Indices for serial routing
    std::vector<MacroParameter> macros_;
    bool enabled_ { true };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRack)
};

/**
 * @class RackManager
 * @brief Administra múltiples racks (nested racks)
 */
class RackManager {
public:
    RackManager() = default;
    
    AudioRack* createRack(const juce::String& name) {
        auto rack = std::make_unique<AudioRack>(name);
        auto* ptr = rack.get();
        racks_.push_back(std::move(rack));
        return ptr;
    }
    
    void removeRack(int index) {
        if (index >= 0 && index < (int)racks_.size()) {
            racks_.erase(racks_.begin() + index);
        }
    }
    
    AudioRack* getRack(int index) {
        return (index >= 0 && index < (int)racks_.size()) 
            ? racks_[index].get() : nullptr;
    }
    
    int getNumRacks() const { return racks_.size(); }
    
    /**
     * Process all racks (for master chain)
     */
    void processAll(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
        for (auto& rack : racks_) {
            if (rack != nullptr && rack->isEnabled()) {
                rack->process(buffer, midiMessages);
            }
        }
    }
    
private:
    std::vector<std::unique_ptr<AudioRack>> racks_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackManager)
};

/**
 * @class MacroKnobComponent
 * @brief UI para macro knob
 */
class MacroKnobComponent : public juce::Component,
                          private juce::Slider::Listener {
public:
    MacroKnobComponent(MacroParameter& macro) : macro_(macro) {
        addAndMakeVisible(slider_);
        slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        slider_.setRange(0.0, 1.0, 0.001);
        slider_.setValue(macro_.value);
        slider_.addListener(this);
        
        addAndMakeVisible(label_);
        label_.setText(macro_.name, juce::dontSendNotification);
        label_.setJustificationType(juce::Justification::centred);
        label_.setEditable(true);
        label_.onTextChange = [this] {
            macro_.name = label_.getText();
        };
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        label_.setBounds(bounds.removeFromTop(20));
        slider_.setBounds(bounds);
    }
    
private:
    void sliderValueChanged(juce::Slider* slider) override {
        if (slider == &slider_) {
            macro_.setValue(slider->getValue());
        }
    }
    
    MacroParameter& macro_;
    juce::Slider slider_;
    juce::Label label_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroKnobComponent)
};

/**
 * @class RackComponent
 * @brief UI completa para rack
 */
class RackComponent : public juce::Component {
public:
    RackComponent(AudioRack& rack) : rack_(rack) {
        // Create 8 macro knobs
        for (int i = 0; i < 8; ++i) {
            auto knob = std::make_unique<MacroKnobComponent>(rack_.getMacro(i));
            addAndMakeVisible(*knob);
            macroKnobs_.push_back(std::move(knob));
        }
        
        addAndMakeVisible(nameLabel_);
        nameLabel_.setText(rack_.getName(), juce::dontSendNotification);
        nameLabel_.setFont(juce::Font(20.0f, juce::Font::bold));
        
        addAndMakeVisible(enableButton_);
        enableButton_.setButtonText("Enable");
        enableButton_.setToggleState(rack_.isEnabled(), juce::dontSendNotification);
        enableButton_.onClick = [this] {
            rack_.setEnabled(enableButton_.getToggleState());
        };
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(getLocalBounds(), 2);
        
        // Draw processor chain
        auto chainBounds = juce::Rectangle<int>(10, 60, getWidth() - 20, 100);
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRect(chainBounds);
        
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        
        int numProcs = rack_.getNumProcessors();
        for (int i = 0; i < numProcs; ++i) {
            auto* proc = rack_.getProcessor(i);
            if (proc != nullptr) {
                int x = chainBounds.getX() + 10 + i * 80;
                int y = chainBounds.getY() + 10;
                
                g.drawRect(x, y, 70, 30);
                g.drawText(proc->getName(), x, y, 70, 30, 
                          juce::Justification::centred);
                
                // Draw connection line
                if (i < numProcs - 1) {
                    g.drawLine(x + 70, y + 15, x + 80, y + 15, 2.0f);
                }
            }
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        
        auto topBar = bounds.removeFromTop(30);
        nameLabel_.setBounds(topBar.removeFromLeft(200));
        enableButton_.setBounds(topBar.removeFromRight(80));
        
        bounds.removeFromTop(130);  // Space for processor chain
        
        // Layout macro knobs in 2 rows of 4
        auto macroArea = bounds.removeFromTop(160);
        auto row1 = macroArea.removeFromTop(80);
        auto row2 = macroArea;
        
        int knobWidth = row1.getWidth() / 4;
        for (int i = 0; i < 4; ++i) {
            macroKnobs_[i]->setBounds(row1.removeFromLeft(knobWidth).reduced(5));
            macroKnobs_[i + 4]->setBounds(row2.removeFromLeft(knobWidth).reduced(5));
        }
    }
    
private:
    AudioRack& rack_;
    std::vector<std::unique_ptr<MacroKnobComponent>> macroKnobs_;
    juce::Label nameLabel_;
    juce::ToggleButton enableButton_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackComponent)
};

} // namespace Audio
} // namespace OmegaStudio
