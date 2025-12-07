#pragma once

#include <JuceHeader.h>
#include "RemoteServer.h"

namespace OmegaStudio {
namespace Remote {

/**
 * RemoteAPI: interpreta comandos JSON simples sobre el socket remoto.
 * Formato: {"cmd":"transport.play"} o {"cmd":"mixer.setGain", "track":0, "gainDb":-6}
 */
class RemoteAPI {
public:
    struct Callbacks {
        std::function<void()> onPlay;
        std::function<void()> onStop;
        std::function<void()> onToggleLoop;
        std::function<void(int track, float gainDb)> onSetGain;
        std::function<void(int pad)> onPadTrigger;
    };

    explicit RemoteAPI(Callbacks cb) : callbacks(std::move(cb)) {}

    juce::String handle(const juce::String& msg) {
        juce::var v = juce::JSON::parse(msg);
        if (!v.isObject()) return "{\"status\":\"error\",\"reason\":\"invalid_json\"}";
        auto* o = v.getDynamicObject();
        if (!o) return "{\"status\":\"error\",\"reason\":\"no_object\"}";
        auto cmd = o->getProperty("cmd").toString();
        if (cmd.isEmpty()) return "{\"status\":\"error\",\"reason\":\"missing_cmd\"}";

        if (cmd == "transport.play" && callbacks.onPlay) callbacks.onPlay();
        else if (cmd == "transport.stop" && callbacks.onStop) callbacks.onStop();
        else if (cmd == "transport.loop" && callbacks.onToggleLoop) callbacks.onToggleLoop();
        else if (cmd == "mixer.setGain" && callbacks.onSetGain) {
            int track = (int)o->getProperty("track", 0);
            float gain = (float)o->getProperty("gainDb", 0.0f);
            callbacks.onSetGain(track, gain);
        } else if (cmd == "pads.trigger" && callbacks.onPadTrigger) {
            int pad = (int)o->getProperty("pad", 0);
            callbacks.onPadTrigger(pad);
        }
        return "{\"status\":\"ok\"}";
    }

private:
    Callbacks callbacks;
};

} // namespace Remote
} // namespace OmegaStudio
