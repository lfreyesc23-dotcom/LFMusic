#include "RemoteServer.h"

namespace OmegaStudio {
namespace Remote {

RemoteServer::RemoteServer() {
    startTimerHz(30); // poll 30 Hz for lightweight control
}

RemoteServer::~RemoteServer() {
    stop();
}

bool RemoteServer::start(const RemoteConfig& cfg) {
    config = cfg;
    socket = std::make_unique<juce::StreamingSocket>();
    if (!socket->createListener(config.port))
        return false;
    return true;
}

void RemoteServer::stop() {
    stopTimer();
    if (client)
        client->close();
    if (socket)
        socket->close();
    client.reset();
    socket.reset();
}

void RemoteServer::timerCallback() {
    if (socket && !client) {
        client.reset(socket->waitForNextConnection());
    }
    if (client && client->isConnected()) {
        char buffer[512]{};
        const int len = client->read(buffer, sizeof(buffer) - 1, true);
        if (len > 0) {
            juce::String cmd(buffer, (size_t) len);
            const auto trimmed = cmd.trim();
            if (commandHandler) {
                auto response = commandHandler(trimmed);
                if (response.isNotEmpty())
                    client->write(response.toRawUTF8(), (int)response.getNumBytesAsUTF8());
            } else if (onCommand) {
                onCommand(trimmed);
            }
        }
    }
}

} // namespace Remote
} // namespace OmegaStudio
