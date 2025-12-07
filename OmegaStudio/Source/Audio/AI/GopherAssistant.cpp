#include "GopherAssistant.h"
#include <algorithm>

namespace omega {
namespace AI {

GopherAssistant::GopherAssistant() {
}

GopherAssistant::ParsedCommand GopherAssistant::parseCommand(const juce::String& command) {
    ParsedCommand parsed;
    auto words = juce::StringArray::fromTokens(command.toLowerCase(), " ", "");
    
    if (words.isEmpty())
        return parsed;
    
    // Extract verb (first word typically)
    parsed.verb = words[0];
    
    // Extract object (noun after verb)
    if (words.size() > 1) {
        parsed.object = words[1];
    }
    
    // Extract key-value parameters
    for (int i = 2; i < words.size(); ++i) {
        if (words[i].contains("=")) {
            auto parts = juce::StringArray::fromTokens(words[i], "=", "");
            if (parts.size() == 2) {
                parsed.parameters[parts[0]] = parts[1];
            }
        }
    }
    
    return parsed;
}

std::vector<GopherAction> GopherAssistant::processCommand(const juce::String& command) {
    auto parsed = parseCommand(command);
    
    if (parsed.verb == "explain")
        return generateExplainActions(parsed);
    else if (parsed.verb == "suggest")
        return generateSuggestActions(parsed);
    else if (parsed.verb == "create")
        return generateCreateActions(parsed);
    else if (parsed.verb == "modify" || parsed.verb == "adjust")
        return generateModifyActions(parsed);
    
    return {};
}

std::vector<GopherAction> GopherAssistant::generateExplainActions(const ParsedCommand& cmd) {
    std::vector<GopherAction> actions;
    
    if (cmd.object == "routing") {
        actions.push_back(explainRouting());
    } else if (cmd.object == "mix" || cmd.object == "mixing") {
        GopherAction action;
        action.actionType = "explain";
        action.description = "Mixing is the process of combining multiple audio tracks, adjusting their levels, panning, and effects to create a cohesive final output.";
        actions.push_back(action);
    } else if (cmd.object == "gain") {
        actions.push_back(suggestGainStaging());
    }
    
    return actions;
}

std::vector<GopherAction> GopherAssistant::generateSuggestActions(const ParsedCommand& cmd) {
    std::vector<GopherAction> actions;
    
    if (cmd.object == "gain" || cmd.object == "gainstaging") {
        actions.push_back(suggestGainStaging());
    } else if (cmd.object == "voicings" || cmd.object == "chords") {
        juce::String key = cmd.parameters.count("key") ? cmd.parameters.at("key") : "C";
        juce::String scale = cmd.parameters.count("scale") ? cmd.parameters.at("scale") : "Major";
        actions.push_back(suggestChordVoicings(key, scale));
    } else if (cmd.object == "mix" || cmd.object == "optimize") {
        actions.push_back(optimizeMix());
    }
    
    return actions;
}

std::vector<GopherAction> GopherAssistant::generateCreateActions(const ParsedCommand& cmd) {
    std::vector<GopherAction> actions;
    
    if (cmd.object == "sidechain") {
        int sourceTrack = cmd.parameters.count("source") ? 
            cmd.parameters.at("source").getIntValue() : 0;
        int targetTrack = cmd.parameters.count("target") ? 
            cmd.parameters.at("target").getIntValue() : 1;
        actions.push_back(createSidechain(sourceTrack, targetTrack));
    } else if (cmd.object == "track") {
        GopherAction action;
        action.actionType = "create";
        action.description = "Create a new audio track";
        action.execute = []() {
            // TODO: Hook into track creation system
            DBG("Creating new track...");
            return true;
        };
        actions.push_back(action);
    }
    
    return actions;
}

std::vector<GopherAction> GopherAssistant::generateModifyActions(const ParsedCommand& cmd) {
    std::vector<GopherAction> actions;
    
    if (cmd.object == "volume" || cmd.object == "gain") {
        GopherAction action;
        action.actionType = "modify";
        action.description = "Adjust track volumes for optimal gain staging";
        action.execute = []() {
            // TODO: Implement volume adjustment
            DBG("Adjusting track volumes...");
            return true;
        };
        actions.push_back(action);
    }
    
    return actions;
}

GopherAction GopherAssistant::explainRouting() {
    GopherAction action;
    action.actionType = "explain";
    action.description = "Audio routing defines how audio signals flow through the audio graph. "
                        "Signals start from sources (instruments, audio files), "
                        "flow through processors (effects, dynamics), "
                        "and end at outputs (master bus, stems). "
                        "Proper routing ensures clean signal flow and prevents feedback loops.";
    action.execute = []() { return true; };
    return action;
}

GopherAction GopherAssistant::suggestGainStaging() {
    GopherAction action;
    action.actionType = "suggest";
    action.description = "Gain Staging Best Practices:\n"
                        "1. Keep peaks around -6dB to -3dB on individual tracks\n"
                        "2. Master bus should peak at -6dB before mastering\n"
                        "3. Use trim/gain plugins at the start of chains\n"
                        "4. Monitor RMS levels, not just peaks\n"
                        "5. Leave headroom for the mastering stage";
    action.execute = []() {
        // TODO: Implement automatic gain staging
        DBG("Applying gain staging suggestions...");
        return true;
    };
    return action;
}

GopherAction GopherAssistant::suggestChordVoicings(const juce::String& key, 
                                                   const juce::String& scale) {
    GopherAction action;
    action.actionType = "suggest";
    action.description = "Suggested chord voicings for " + key + " " + scale + ":\n"
                        "1. Use drop-2 voicings for smooth voice leading\n"
                        "2. Spread voicings wider for clarity\n"
                        "3. Use inversions to minimize hand movement\n"
                        "4. Common progression: I - V - vi - IV";
    action.parameters = juce::var(new juce::DynamicObject());
    action.parameters.getDynamicObject()->setProperty("key", key);
    action.parameters.getDynamicObject()->setProperty("scale", scale);
    action.execute = []() { return true; };
    return action;
}

GopherAction GopherAssistant::createSidechain(int sourceTrack, int targetTrack) {
    GopherAction action;
    action.actionType = "create";
    action.description = "Creating sidechain compression from track " + 
                        juce::String(sourceTrack) + " to track " + 
                        juce::String(targetTrack);
    action.parameters = juce::var(new juce::DynamicObject());
    action.parameters.getDynamicObject()->setProperty("sourceTrack", sourceTrack);
    action.parameters.getDynamicObject()->setProperty("targetTrack", targetTrack);
    action.execute = [sourceTrack, targetTrack]() {
        // TODO: Hook into sidechain routing system
        DBG("Creating sidechain: " << sourceTrack << " -> " << targetTrack);
        return true;
    };
    return action;
}

GopherAction GopherAssistant::optimizeMix() {
    GopherAction action;
    action.actionType = "suggest";
    action.description = "Mix Optimization Suggestions:\n"
                        "1. Apply high-pass filters on non-bass elements (80-120 Hz)\n"
                        "2. Use complementary EQ between competing elements\n"
                        "3. Pan similar instruments to different positions\n"
                        "4. Use parallel compression on drums\n"
                        "5. Add reverb via sends, not inserts\n"
                        "6. Check mix in mono for phase issues";
    action.execute = []() {
        // TODO: Implement automatic mix optimization
        DBG("Applying mix optimization...");
        return true;
    };
    return action;
}

bool GopherAssistant::executeAction(const GopherAction& action) {
    if (action.execute) {
        return action.execute();
    }
    return false;
}

std::vector<juce::String> GopherAssistant::getSuggestions() {
    std::vector<juce::String> suggestions;
    
    if (!context_.hasAudioGraph) {
        suggestions.push_back("Create audio routing graph");
    }
    
    if (context_.numTracks > 8) {
        suggestions.push_back("Organize tracks into groups");
    }
    
    if (!context_.hasMixer) {
        suggestions.push_back("Set up mixer for gain staging");
    }
    
    suggestions.push_back("Explain audio routing");
    suggestions.push_back("Suggest gain staging");
    suggestions.push_back("Create sidechain compression");
    suggestions.push_back("Optimize mix");
    
    return suggestions;
}

} // namespace AI
} // namespace omega
