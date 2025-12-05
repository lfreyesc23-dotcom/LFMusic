/*
  ==============================================================================

    TrackFreezing.cpp

  ==============================================================================
*/

#include "TrackFreezing.h"

namespace omega {

TrackFreezer::TrackFreezer() {}

bool TrackFreezer::freezeTrack(int trackIndex) {
    FreezeState state;
    state.isFrozen = true;
    state.freezeBuffer.setSize(2, 48000 * 10); // 10 seconds placeholder
    state.freezeBuffer.clear();
    state.cpuUsage = 15.0;
    
    frozenTracks_[trackIndex] = state;
    cpuSaving_ += 15.0f;
    
    return true;
}

bool TrackFreezer::unfreezeTrack(int trackIndex) {
    auto it = frozenTracks_.find(trackIndex);
    if (it == frozenTracks_.end()) return false;
    
    cpuSaving_ -= it->second.cpuUsage;
    frozenTracks_.erase(it);
    
    return true;
}

bool TrackFreezer::isFrozen(int trackIndex) const {
    return frozenTracks_.find(trackIndex) != frozenTracks_.end();
}

} // namespace omega
