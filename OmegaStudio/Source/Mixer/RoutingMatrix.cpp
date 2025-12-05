/*
  ==============================================================================

    RoutingMatrix.cpp

  ==============================================================================
*/

#include "RoutingMatrix.h"

namespace omega {

RoutingMatrix::RoutingMatrix() {}

void RoutingMatrix::connect(int sourceTrack, int destTrack, float gain) {
    Connection conn;
    conn.source = sourceTrack;
    conn.dest = destTrack;
    conn.gain = gain;
    connections_.push_back(conn);
}

void RoutingMatrix::disconnect(int sourceTrack, int destTrack) {
    connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
        [=](const Connection& c) { return c.source == sourceTrack && c.dest == destTrack; }),
        connections_.end());
}

bool RoutingMatrix::isConnected(int sourceTrack, int destTrack) const {
    for (const auto& conn : connections_) {
        if (conn.source == sourceTrack && conn.dest == destTrack) {
            return true;
        }
    }
    return false;
}

std::vector<int> RoutingMatrix::getDestinations(int sourceTrack) const {
    std::vector<int> dests;
    for (const auto& conn : connections_) {
        if (conn.source == sourceTrack) {
            dests.push_back(conn.dest);
        }
    }
    return dests;
}

std::vector<int> RoutingMatrix::getSources(int destTrack) const {
    std::vector<int> sources;
    for (const auto& conn : connections_) {
        if (conn.dest == destTrack) {
            sources.push_back(conn.source);
        }
    }
    return sources;
}

void RoutingMatrix::clear() {
    connections_.clear();
}

} // namespace omega
