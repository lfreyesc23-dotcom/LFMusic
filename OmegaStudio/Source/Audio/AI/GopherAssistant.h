#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>
#include <map>

namespace omega {
namespace AI {

struct GopherAction {
    juce::String actionType; // "explain", "suggest", "create", "modify"
    juce::String description;
    juce::var parameters;
    std::function<bool()> execute;
};

struct GopherContext {
    juce::String projectName;
    int numTracks = 0;
    double bpm = 120.0;
    juce::String keySignature = "C";
    bool hasAudioGraph = false;
    bool hasMixer = false;
};

class GopherAssistant {
public:
    GopherAssistant();
    
    void setContext(const GopherContext& ctx) { context_ = ctx; }
    
    // Process natural language command
    std::vector<GopherAction> processCommand(const juce::String& command);
    
    // Specialized actions
    GopherAction explainRouting();
    GopherAction suggestGainStaging();
    GopherAction suggestChordVoicings(const juce::String& key, const juce::String& scale);
    GopherAction createSidechain(int sourceTrack, int targetTrack);
    GopherAction optimizeMix();
    
    // Execute action
    bool executeAction(const GopherAction& action);
    
    // Get suggestions based on context
    std::vector<juce::String> getSuggestions();
    
private:
    GopherContext context_;
    
    // Command parsing
    struct ParsedCommand {
        juce::String verb;      // explain, suggest, create, etc.
        juce::String object;    // routing, mix, sidechain, etc.
        std::map<juce::String, juce::String> parameters;
    };
    
    ParsedCommand parseCommand(const juce::String& command);
    
    // Action generators
    std::vector<GopherAction> generateExplainActions(const ParsedCommand& cmd);
    std::vector<GopherAction> generateSuggestActions(const ParsedCommand& cmd);
    std::vector<GopherAction> generateCreateActions(const ParsedCommand& cmd);
    std::vector<GopherAction> generateModifyActions(const ParsedCommand& cmd);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GopherAssistant)
};

} // namespace AI
} // namespace omega
