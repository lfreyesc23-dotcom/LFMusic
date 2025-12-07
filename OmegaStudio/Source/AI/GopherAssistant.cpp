//==============================================================================
// GopherAssistant.cpp - AI production assistant
//==============================================================================

#include "GopherAssistant.h"

namespace OmegaStudio::AI {

GopherAssistant::GopherAssistant() {
    initializeKnowledgeBase();
}

void GopherAssistant::initializeKnowledgeBase() {
    // Production tips database
    knowledgeBase_["mixing"] = {
        "Use subtractive EQ before additive EQ",
        "High-pass filter everything except kick and bass",
        "Leave headroom for mastering (-6dB peak)",
        "Use parallel compression on drums",
        "Apply reverb to sends, not inserts"
    };
    
    knowledgeBase_["mastering"] = {
        "Start with reference tracks",
        "Use linear phase EQ for subtle corrections",
        "Apply multiband compression carefully",
        "Limit to -0.3dB for streaming services",
        "Check mono compatibility"
    };
    
    knowledgeBase_["synthesis"] = {
        "Start with simple waveforms",
        "Use multiple oscillators for thickness",
        "Apply filter envelope for movement",
        "Add subtle detuning for width",
        "Layer different synth types"
    };
    
    knowledgeBase_["composition"] = {
        "Build arrangements in 8-bar sections",
        "Use tension and release",
        "Vary melody rhythms",
        "Add counter-melodies for interest",
        "Less is often more"
    };
}

juce::String GopherAssistant::askQuestion(const juce::String& question) {
    auto lowerQuestion = question.toLowerCase();
    
    // Simple keyword matching
    if (lowerQuestion.contains("mix") || lowerQuestion.contains("balance")) {
        return getTip("mixing");
    }
    else if (lowerQuestion.contains("master")) {
        return getTip("mastering");
    }
    else if (lowerQuestion.contains("synth") || lowerQuestion.contains("sound")) {
        return getTip("synthesis");
    }
    else if (lowerQuestion.contains("melody") || lowerQuestion.contains("chord")) {
        return getTip("composition");
    }
    
    return "I can help with mixing, mastering, synthesis, and composition. What would you like to know?";
}

juce::String GopherAssistant::getTip(const juce::String& category) {
    if (knowledgeBase_.count(category) > 0) {
        auto& tips = knowledgeBase_[category];
        if (!tips.empty()) {
            int index = juce::Random::getSystemRandom().nextInt(tips.size());
            return tips[index];
        }
    }
    return "No tips available for this category.";
}

juce::String GopherAssistant::analyzeMix(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) {
        return "No audio to analyze.";
    }
    
    // Simple RMS analysis
    float rmsLeft = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float peak = buffer.getMagnitude(0, 0, buffer.getNumSamples());
    
    juce::String analysis = "Mix Analysis:\n\n";
    
    // Check levels
    if (peak > 0.95f) {
        analysis += "⚠️ WARNING: Signal is clipping! Reduce levels.\n";
    }
    else if (rmsLeft < 0.1f) {
        analysis += "💡 TIP: Levels are quite low. Consider increasing gain.\n";
    }
    else {
        analysis += "✅ Levels look good.\n";
    }
    
    // Dynamic range check
    float dynamicRange = 20.0f * std::log10(peak / (rmsLeft + 0.0001f));
    if (dynamicRange < 6.0f) {
        analysis += "⚠️ Low dynamic range. Mix might sound over-compressed.\n";
    }
    else if (dynamicRange > 15.0f) {
        analysis += "💡 High dynamic range. Good dynamics!\n";
    }
    
    analysis += "\nRMS Level: " + juce::String(rmsLeft, 3) + "\n";
    analysis += "Peak Level: " + juce::String(peak, 3) + "\n";
    analysis += "Dynamic Range: " + juce::String(dynamicRange, 1) + " dB";
    
    return analysis;
}

juce::StringArray GopherAssistant::suggestEffects(const juce::String& trackType) {
    juce::StringArray suggestions;
    
    auto lower = trackType.toLowerCase();
    
    if (lower.contains("vocal")) {
        suggestions.add("Parametric EQ (cut 100Hz, boost 10kHz)");
        suggestions.add("Compressor (4:1 ratio, fast attack)");
        suggestions.add("De-esser");
        suggestions.add("Reverb (plate or hall)");
        suggestions.add("Delay (1/8 note)");
    }
    else if (lower.contains("drum") || lower.contains("kick")) {
        suggestions.add("Transient Shaper");
        suggestions.add("Compressor (slow attack for punch)");
        suggestions.add("Saturator");
        suggestions.add("Reverb (room)");
    }
    else if (lower.contains("bass")) {
        suggestions.add("High-pass filter (30Hz)");
        suggestions.add("Multiband Compressor");
        suggestions.add("Saturation");
        suggestions.add("Chorus (subtle)");
    }
    else if (lower.contains("synth") || lower.contains("pad")) {
        suggestions.add("Chorus");
        suggestions.add("Reverb (hall or ambient)");
        suggestions.add("Delay");
        suggestions.add("EQ (carve space)");
    }
    else {
        suggestions.add("EQ");
        suggestions.add("Compressor");
        suggestions.add("Reverb");
    }
    
    return suggestions;
}

GopherAssistant::ProductionSuggestion GopherAssistant::suggestNextStep(
    const juce::String& currentStage)
{
    ProductionSuggestion suggestion;
    
    auto lower = currentStage.toLowerCase();
    
    if (lower.contains("start") || lower.contains("begin")) {
        suggestion.action = "Create a basic drum pattern";
        suggestion.reasoning = "Starting with drums provides a solid rhythmic foundation";
        suggestion.estimatedTime = "10-15 minutes";
    }
    else if (lower.contains("drum")) {
        suggestion.action = "Add bassline";
        suggestion.reasoning = "Bass locks in with drums to create the groove";
        suggestion.estimatedTime = "15-20 minutes";
    }
    else if (lower.contains("bass")) {
        suggestion.action = "Add chord progression or melody";
        suggestion.reasoning = "Harmonic content brings musicality to the track";
        suggestion.estimatedTime = "20-30 minutes";
    }
    else if (lower.contains("mix")) {
        suggestion.action = "Apply master bus processing";
        suggestion.reasoning = "Light compression and EQ glues the mix together";
        suggestion.estimatedTime = "30-45 minutes";
    }
    else {
        suggestion.action = "Listen to reference tracks";
        suggestion.reasoning = "References help guide your production decisions";
        suggestion.estimatedTime = "10 minutes";
    }
    
    return suggestion;
}

} // namespace OmegaStudio::AI
