#pragma once

#include <JuceHeader.h>
#include "../Audio/AI/AIServiceStubs.h"
#include "../Sequencer/PlaylistEngine.h"

namespace OmegaStudio {
namespace Workflow {

class LoopStarterIntegrator {
public:
    explicit LoopStarterIntegrator(Sequencer::PlaylistEngine& engine)
        : engine_(engine) {}

    bool applyArrangement(AI::LoopStarterService& service,
                          const AI::LoopRequest& request,
                          std::function<void(float)> progress = {}) {
        std::vector<AI::LoopClip> clips;
        if (!service.generateArrangement(request, clips, progress))
            return false;

        // Ensure tracks exist: 0 drums, 1 bass, 2 harmony, 3 fx
        ensureTrackCount(4);

        for (const auto& clip : clips) {
            int track = trackForType(clip.type);
            auto patternId = engine_.createPattern(clip.name);
            if (auto* p = engine_.getPattern(patternId)) {
                p->lengthInBars = clip.lengthBeats / 4.0; // beats to bars (assuming 4/4)
                if (!clip.audio.hasBeenCleared()) {
                    // In real impl, persist clip.audio to file and set filePath
                    Sequencer::Pattern::AudioClip ac;
                    ac.filePath = ""; // placeholder path
                    ac.startOffset = 0.0;
                    ac.length = clip.lengthBeats / 4.0;
                    ac.gain = juce::Decibels::decibelsToGain(clip.gainDb);
                    p->audioClips.push_back(ac);
                }
            }
            engine_.addPatternToPlaylist(patternId, track, clip.startBeat / 4.0, clip.lengthBeats / 4.0);
        }
        if (progress) progress(1.0f);
        return true;
    }

private:
    Sequencer::PlaylistEngine& engine_;

    void ensureTrackCount(int n) {
        while ((int)engine_.getTracks().size() < n)
            engine_.addTrack("LoopStarter Track " + juce::String(engine_.getTracks().size()));
    }

    int trackForType(const juce::String& type) {
        if (type == "drums") return 0;
        if (type == "bass") return 1;
        if (type == "harmony") return 2;
        if (type == "fx") return 3;
        return 3;
    }
};

} // namespace Workflow
} // namespace OmegaStudio
