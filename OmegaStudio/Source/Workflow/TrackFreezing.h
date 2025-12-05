/*
  ==============================================================================

    TrackFreezing.h
    CPU-efficient track freezing system

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace omega {

class TrackFreezer {
public:
    struct FreezeState {
        bool isFrozen = false;
        juce::File freezeFile;
        juce::AudioBuffer<float> freezeBuffer;
        double cpuUsage = 0.0;
    };
    
    TrackFreezer();
    
    bool freezeTrack(int trackIndex);
    bool unfreezeTrack(int trackIndex);
    bool isFrozen(int trackIndex) const;
    
    float getCPUSaving() const { return cpuSaving_; }
    
private:
    std::map<int, FreezeState> frozenTracks_;
    float cpuSaving_ = 0.0f;
};

} // namespace omega
