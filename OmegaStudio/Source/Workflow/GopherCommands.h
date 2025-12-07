#pragma once

#include <JuceHeader.h>
#include "../Audio/AI/AIServiceStubs.h"
#include "../Sequencer/PlaylistEngine.h"
#include "LoopStarterIntegration.h"

namespace OmegaStudio {
namespace Workflow {

class GopherCommandRegistry {
public:
    GopherCommandRegistry(AI::GopherAssistant& assistant,
                          Sequencer::PlaylistEngine& playlist,
                          AI::LoopStarterService& loopStarter)
        : assistant_(assistant), playlist_(playlist), loopStarter_(loopStarter) {}

    void registerDefaults() {
        assistant_.registerCommand({
            "list.commands",
            "Listar comandos disponibles",
            [this]() { return assistant_.summarizeCommands(); }
        });

        assistant_.registerCommand({
            "loop.generate",
            "Generar arreglo de loops base (drums/bass/harmony/fx)",
            [this]() {
                AI::LoopRequest req;
                req.genre = "electronic";
                req.key = "Cmin";
                req.bpm = 128.0f;
                req.bars = 8;
                LoopStarterIntegrator integrator(playlist_);
                integrator.applyArrangement(loopStarter_, req, {});
                return juce::String("LoopStarter aplicado: ") + juce::String(req.bars) + " compases.";
            }
        });
    }

private:
    AI::GopherAssistant& assistant_;
    Sequencer::PlaylistEngine& playlist_;
    AI::LoopStarterService& loopStarter_;
};

} // namespace Workflow
} // namespace OmegaStudio
