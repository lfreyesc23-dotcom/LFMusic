/*
  ==============================================================================

    MacroSystem.h
    Action recording and macro automation system

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>

namespace omega {

struct MacroAction {
    juce::String actionId;
    juce::var parameters;
    double timestamp;
    
    MacroAction() : timestamp(0.0) {}
    MacroAction(const juce::String& id, const juce::var& params, double time)
        : actionId(id), parameters(params), timestamp(time) {}
};

class Macro {
public:
    juce::String name;
    std::vector<MacroAction> actions;
    bool preserveTiming = true;
    
    void addAction(const juce::String& actionId, const juce::var& params, double time);
    void clear();
    int getActionCount() const { return static_cast<int>(actions.size()); }
};

class MacroRecorder {
public:
    void startRecording();
    void stopRecording();
    void recordAction(const juce::String& actionId, const juce::var& params = juce::var());
    bool isRecording() const { return recording_; }
    Macro getRecordedMacro() const { return currentMacro_; }
    
private:
    bool recording_ = false;
    Macro currentMacro_;
    double recordStartTime_ = 0.0;
};

class MacroPlayer {
public:
    void playMacro(const Macro& macro, std::function<void(const juce::String&, const juce::var&)> callback);
    void stop();
    bool isPlaying() const { return playing_; }
    
private:
    bool playing_ = false;
};

} // namespace omega
