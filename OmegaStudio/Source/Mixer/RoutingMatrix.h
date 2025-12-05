/*
  ==============================================================================

    RoutingMatrix.h
    Signal routing matrix with visual flow

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <map>

namespace omega {

class RoutingMatrix {
public:
    RoutingMatrix();
    
    void connect(int sourceTrack, int destTrack, float gain = 1.0f);
    void disconnect(int sourceTrack, int destTrack);
    bool isConnected(int sourceTrack, int destTrack) const;
    
    std::vector<int> getDestinations(int sourceTrack) const;
    std::vector<int> getSources(int destTrack) const;
    
    void clear();
    
private:
    struct Connection {
        int source;
        int dest;
        float gain;
    };
    
    std::vector<Connection> connections_;
};

} // namespace omega
