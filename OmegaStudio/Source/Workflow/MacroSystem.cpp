/*
  ==============================================================================

    MacroSystem.cpp

  ==============================================================================
*/

#include "MacroSystem.h"

namespace omega {

void Macro::addAction(const juce::String& actionId, const juce::var& params, double time) {
    actions.emplace_back(actionId, params, time);
}

void Macro::clear() {
    actions.clear();
}

void MacroRecorder::startRecording() {
    recording_ = true;
    currentMacro_.clear();
    recordStartTime_ = juce::Time::getMillisecondCounterHiRes() / 1000.0;
}

void MacroRecorder::stopRecording() {
    recording_ = false;
}

void MacroRecorder::recordAction(const juce::String& actionId, const juce::var& params) {
    if (!recording_) return;
    
    double currentTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    double relativeTime = currentTime - recordStartTime_;
    currentMacro_.addAction(actionId, params, relativeTime);
}

void MacroPlayer::playMacro(const Macro& macro, std::function<void(const juce::String&, const juce::var&)> callback) {
    playing_ = true;
    
    for (const auto& action : macro.actions) {
        if (!playing_) break;
        
        if (macro.preserveTiming && action.timestamp > 0.0) {
            juce::Thread::sleep(static_cast<int>(action.timestamp * 1000.0));
        }
        
        callback(action.actionId, action.parameters);
    }
    
    playing_ = false;
}

void MacroPlayer::stop() {
    playing_ = false;
}

} // namespace omega
