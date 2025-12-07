#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio::AI {

/**
 * @brief Gopher AI Assistant - Production Helper  
 * Provides production tips, mix analysis, and workflow suggestions
 */
class GopherAssistant {
public:
    struct ProductionSuggestion {
        juce::String action;
        juce::String reasoning;
        juce::String estimatedTime;
    };

    GopherAssistant();

    // Ask questions
    juce::String askQuestion(const juce::String& question);
    juce::String getTip(const juce::String& category);

    // Analysis
    juce::String analyzeMix(const juce::AudioBuffer<float>& buffer);
    juce::StringArray suggestEffects(const juce::String& trackType);
    ProductionSuggestion suggestNextStep(const juce::String& currentStage);

private:
    void initializeKnowledgeBase();

    std::map<juce::String, std::vector<juce::String>> knowledgeBase_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GopherAssistant)
};

} // namespace OmegaStudio::AI
