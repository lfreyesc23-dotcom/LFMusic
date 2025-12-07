#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>
#include <map>

namespace omega {
namespace AI {

struct LoopRequest {
    juce::String genre;
    juce::String key;
    float bpm = 120.0f;
    int bars = 8;
    juce::String mood;
};

struct LoopClip {
    juce::String name;
    juce::String type; // drum, bass, harmony, fx
    juce::AudioBuffer<float> audio;
    float startBeat = 0.0f;
    float lengthBeats = 4.0f;
    float gainDb = 0.0f;
    float pan = 0.0f;
};

class LoopStarterService {
public:
    bool generateArrangement(const LoopRequest& request,
                             std::vector<LoopClip>& outClips,
                             std::function<void(float)> progress = {}) {
        if (progress) progress(0.05f);
        // TODO: integrar sampler/DB real; aquí generamos placeholders silenciosos
        const float barLen = 4.0f;
        const float total = request.bars * barLen;

        auto addClip = [&](juce::String type, float start, float len) {
            LoopClip c;
            c.name = type + "_" + juce::String(start, 2);
            c.type = type;
            c.startBeat = start;
            c.lengthBeats = len;
            c.audio.setSize(2, static_cast<int>(request.bpm)); // placeholder small buffer
            outClips.push_back(std::move(c));
        };

        // Basic layout: drums on every bar, bass on 1/3, harmony on 1, FX riser to end
        for (float t = 0.0f; t < total; t += barLen) {
            addClip("drums", t, barLen);
            if (juce::fmod(t / barLen, 2.0f) < 0.5f)
                addClip("bass", t, barLen);
            if (t == 0.0f)
                addClip("harmony", t, juce::jmin(total, 8.0f));
        }
        addClip("fx", juce::jmax(0.0f, total - 4.0f), 4.0f);

        if (progress) progress(1.0f);
        return true;
    }
};

struct AssistantCommand {
    juce::String id;
    juce::String description;
    std::function<juce::String()> action;
};

class GopherAssistant {
public:
    void registerCommand(const AssistantCommand& cmd) { commands.add(cmd); }

    juce::String runCommand(const juce::String& id) {
        for (const auto& cmd : commands)
            if (cmd.id == id && cmd.action)
                return cmd.action();
        return {};
    }

    juce::String summarizeCommands() const {
        juce::StringArray lines;
        for (const auto& cmd : commands) {
            lines.add(cmd.id + ": " + cmd.description);
        }
        return lines.joinIntoString("\n");
    }

private:
    juce::Array<AssistantCommand> commands;
};

} // namespace AI
} // namespace omega
