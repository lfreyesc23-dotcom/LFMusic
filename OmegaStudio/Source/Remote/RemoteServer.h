#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace Remote {

struct RemoteConfig {
    int port = 7788;
    bool enableWebRTC = false;
};

class RemoteServer : private juce::Timer {
public:
    RemoteServer();
    ~RemoteServer() override;

    bool start(const RemoteConfig& cfg);
    void stop();

    std::function<void(const juce::String&)> onCommand;
    std::function<juce::String(const juce::String&)> commandHandler; // returns response JSON

private:
    RemoteConfig config;
    std::unique_ptr<juce::StreamingSocket> socket;
    std::unique_ptr<juce::StreamingSocket> client;

    void timerCallback() override;
};

} // namespace Remote
} // namespace OmegaStudio
