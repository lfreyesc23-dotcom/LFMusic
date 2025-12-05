/*
  ==============================================================================
    AutomationSystem.cpp
    Implementation del sistema de automatización
  ==============================================================================
*/

#include "AutomationSystem.h"
#include <algorithm>
#include <cmath>

namespace OmegaStudio {

//==============================================================================
// AutomationPoint Implementation
//==============================================================================

juce::var AutomationPoint::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("time", timeInBeats);
    obj->setProperty("value", value);
    obj->setProperty("curveType", static_cast<int>(curveType));
    obj->setProperty("curvature", curvature);
    return juce::var(obj.get());
}

AutomationPoint AutomationPoint::fromVar(const juce::var& v) {
    AutomationPoint point;
    point.timeInBeats = v["time"];
    point.value = v["value"];
    point.curveType = static_cast<AutomationCurveType>((int)v["curveType"]);
    point.curvature = v["curvature"];
    return point;
}

//==============================================================================
// AutomationLane Implementation
//==============================================================================

AutomationLane::AutomationLane(const juce::String& paramId, float defValue)
    : parameterID(paramId), defaultValue(defValue) {
    parameterName = paramId;
}

void AutomationLane::addPoint(const AutomationPoint& point) {
    points.push_back(point);
    sortPoints();
}

void AutomationLane::addPoint(double timeInBeats, float value, AutomationCurveType curve) {
    points.emplace_back(timeInBeats, value, curve);
    sortPoints();
}

void AutomationLane::removePoint(int index) {
    if (index >= 0 && index < static_cast<int>(points.size()))
        points.erase(points.begin() + index);
}

void AutomationLane::removePointsInRange(double startBeat, double endBeat) {
    points.erase(
        std::remove_if(points.begin(), points.end(),
            [startBeat, endBeat](const AutomationPoint& p) {
                return p.timeInBeats >= startBeat && p.timeInBeats <= endBeat;
            }),
        points.end()
    );
}

void AutomationLane::clearAllPoints() {
    points.clear();
}

void AutomationLane::sortPoints() {
    std::sort(points.begin(), points.end(),
        [](const AutomationPoint& a, const AutomationPoint& b) {
            return a.timeInBeats < b.timeInBeats;
        });
}

int AutomationLane::findPointIndexAtTime(double time, double tolerance) const {
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        if (std::abs(points[i].timeInBeats - time) < tolerance)
            return i;
    }
    return -1;
}

float AutomationLane::getValueAtTime(double timeInBeats) const {
    if (points.empty())
        return defaultValue;
    
    // Before first point
    if (timeInBeats <= points.front().timeInBeats)
        return points.front().value;
    
    // After last point
    if (timeInBeats >= points.back().timeInBeats)
        return points.back().value;
    
    // Find surrounding points
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];
        
        if (timeInBeats >= p1.timeInBeats && timeInBeats <= p2.timeInBeats) {
            switch (p1.curveType) {
                case AutomationCurveType::Linear:
                    return interpolateLinear(p1, p2, timeInBeats);
                case AutomationCurveType::Bezier:
                    return interpolateBezier(p1, p2, timeInBeats);
                case AutomationCurveType::Step:
                    return p1.value;
                case AutomationCurveType::Exponential:
                    return interpolateExponential(p1, p2, timeInBeats);
                case AutomationCurveType::Logarithmic:
                    return interpolateLogarithmic(p1, p2, timeInBeats);
            }
        }
    }
    
    return defaultValue;
}

float AutomationLane::interpolateLinear(const AutomationPoint& p1, const AutomationPoint& p2, double time) const {
    double t = (time - p1.timeInBeats) / (p2.timeInBeats - p1.timeInBeats);
    return p1.value + (p2.value - p1.value) * static_cast<float>(t);
}

float AutomationLane::interpolateBezier(const AutomationPoint& p1, const AutomationPoint& p2, double time) const {
    double t = (time - p1.timeInBeats) / (p2.timeInBeats - p1.timeInBeats);
    
    // Simple cubic bezier
    double curve = p1.curvature;
    double smoothT = t * t * (3.0 - 2.0 * t);  // Smoothstep
    smoothT = smoothT * (1.0 - curve) + t * curve;
    
    return p1.value + (p2.value - p1.value) * static_cast<float>(smoothT);
}

float AutomationLane::interpolateExponential(const AutomationPoint& p1, const AutomationPoint& p2, double time) const {
    double t = (time - p1.timeInBeats) / (p2.timeInBeats - p1.timeInBeats);
    double expT = (std::exp(t * 2.0) - 1.0) / (std::exp(2.0) - 1.0);
    return p1.value + (p2.value - p1.value) * static_cast<float>(expT);
}

float AutomationLane::interpolateLogarithmic(const AutomationPoint& p1, const AutomationPoint& p2, double time) const {
    double t = (time - p1.timeInBeats) / (p2.timeInBeats - p1.timeInBeats);
    double logT = std::log(1.0 + t * 9.0) / std::log(10.0);
    return p1.value + (p2.value - p1.value) * static_cast<float>(logT);
}

void AutomationLane::movePoint(int index, double newTime, float newValue) {
    if (index >= 0 && index < static_cast<int>(points.size())) {
        points[index].timeInBeats = newTime;
        points[index].value = newValue;
        sortPoints();
    }
}

void AutomationLane::setCurveType(int index, AutomationCurveType type) {
    if (index >= 0 && index < static_cast<int>(points.size()))
        points[index].curveType = type;
}

void AutomationLane::setCurvature(int index, float curvature) {
    if (index >= 0 && index < static_cast<int>(points.size()))
        points[index].curvature = juce::jlimit(0.0f, 1.0f, curvature);
}

void AutomationLane::scaleValues(float multiplier) {
    for (auto& point : points)
        point.value *= multiplier;
}

void AutomationLane::offsetValues(float offset) {
    for (auto& point : points)
        point.value += offset;
}

void AutomationLane::quantizeToGrid(double gridSize) {
    for (auto& point : points) {
        double quantized = std::round(point.timeInBeats / gridSize) * gridSize;
        point.timeInBeats = quantized;
    }
    sortPoints();
}

juce::var AutomationLane::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("parameterID", parameterID);
    obj->setProperty("parameterName", parameterName);
    obj->setProperty("defaultValue", defaultValue);
    obj->setProperty("mode", static_cast<int>(mode));
    
    juce::Array<juce::var> pointsArray;
    for (const auto& point : points)
        pointsArray.add(point.toVar());
    obj->setProperty("points", pointsArray);
    
    return juce::var(obj.get());
}

AutomationLane AutomationLane::fromVar(const juce::var& v) {
    AutomationLane lane(v["parameterID"], v["defaultValue"]);
    lane.parameterName = v["parameterName"].toString();
    lane.mode = static_cast<AutomationMode>((int)v["mode"]);
    
    if (auto* pointsArray = v["points"].getArray()) {
        for (const auto& pointVar : *pointsArray)
            lane.points.push_back(AutomationPoint::fromVar(pointVar));
    }
    
    return lane;
}

//==============================================================================
// TrackAutomation Implementation
//==============================================================================

TrackAutomation::TrackAutomation() = default;

AutomationLane* TrackAutomation::addLane(const juce::String& paramID, float defaultValue) {
    auto lane = std::make_unique<AutomationLane>(paramID, defaultValue);
    auto* ptr = lane.get();
    lanes[paramID] = std::move(lane);
    return ptr;
}

void TrackAutomation::removeLane(const juce::String& paramID) {
    lanes.erase(paramID);
}

AutomationLane* TrackAutomation::getLane(const juce::String& paramID) {
    auto it = lanes.find(paramID);
    return (it != lanes.end()) ? it->second.get() : nullptr;
}

const AutomationLane* TrackAutomation::getLane(const juce::String& paramID) const {
    auto it = lanes.find(paramID);
    return (it != lanes.end()) ? it->second.get() : nullptr;
}

std::vector<juce::String> TrackAutomation::getLaneIDs() const {
    std::vector<juce::String> ids;
    for (const auto& pair : lanes)
        ids.push_back(pair.first);
    return ids;
}

std::map<juce::String, float> TrackAutomation::getValuesAtTime(double timeInBeats) const {
    std::map<juce::String, float> values;
    for (const auto& pair : lanes)
        values[pair.first] = pair.second->getValueAtTime(timeInBeats);
    return values;
}

float TrackAutomation::getValueAtTime(const juce::String& paramID, double timeInBeats) const {
    if (auto* lane = getLane(paramID))
        return lane->getValueAtTime(timeInBeats);
    return 0.0f;
}

void TrackAutomation::startRecording(const juce::String& paramID) {
    recordingLanes.insert(paramID);
}

void TrackAutomation::stopRecording(const juce::String& paramID) {
    recordingLanes.erase(paramID);
}

void TrackAutomation::recordPoint(const juce::String& paramID, double timeInBeats, float value) {
    if (!isRecording(paramID))
        return;
    
    auto* lane = getLane(paramID);
    if (!lane)
        lane = addLane(paramID);
    
    lane->addPoint(timeInBeats, value);
}

bool TrackAutomation::isRecording(const juce::String& paramID) const {
    return recordingLanes.find(paramID) != recordingLanes.end();
}

void TrackAutomation::clearAllAutomation() {
    lanes.clear();
}

juce::var TrackAutomation::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    juce::Array<juce::var> lanesArray;
    for (const auto& pair : lanes)
        lanesArray.add(pair.second->toVar());
    obj->setProperty("lanes", lanesArray);
    
    return juce::var(obj.get());
}

void TrackAutomation::loadFromVar(const juce::var& v) {
    lanes.clear();
    
    if (auto* lanesArray = v["lanes"].getArray()) {
        for (const auto& laneVar : *lanesArray) {
            auto lane = AutomationLane::fromVar(laneVar);
            lanes[lane.getParameterID()] = std::make_unique<AutomationLane>(std::move(lane));
        }
    }
}

//==============================================================================
// AutomationManager Implementation
//==============================================================================

AutomationManager::AutomationManager() = default;

TrackAutomation* AutomationManager::getTrackAutomation(int trackIndex) {
    auto it = trackAutomations.find(trackIndex);
    return (it != trackAutomations.end()) ? it->second.get() : nullptr;
}

const TrackAutomation* AutomationManager::getTrackAutomation(int trackIndex) const {
    auto it = trackAutomations.find(trackIndex);
    return (it != trackAutomations.end()) ? it->second.get() : nullptr;
}

void AutomationManager::ensureTrackAutomation(int trackIndex) {
    if (trackAutomations.find(trackIndex) == trackAutomations.end())
        trackAutomations[trackIndex] = std::make_unique<TrackAutomation>();
}

void AutomationManager::removeTrackAutomation(int trackIndex) {
    trackAutomations.erase(trackIndex);
}

void AutomationManager::setGlobalMode(AutomationMode mode) {
    globalMode = mode;
}

void AutomationManager::startRecording() {
    recording = true;
}

void AutomationManager::stopRecording() {
    recording = false;
}

void AutomationManager::setPlaybackPosition(double timeInBeats) {
    playbackPosition = timeInBeats;
}

void AutomationManager::applyAutomationAtCurrentTime() {
    if (!parameterCallback)
        return;
    
    for (const auto& trackPair : trackAutomations) {
        int trackIndex = trackPair.first;
        const auto* track = trackPair.second.get();
        
        auto values = track->getValuesAtTime(playbackPosition);
        for (const auto& valuePair : values) {
            parameterCallback(trackIndex, valuePair.first, valuePair.second);
        }
    }
}

void AutomationManager::pushUndoState() {
    AutomationState state;
    state.data = toVar();
    
    undoStack.push_back(state);
    if (undoStack.size() > static_cast<size_t>(maxUndoLevels))
        undoStack.erase(undoStack.begin());
    
    redoStack.clear();
}

void AutomationManager::undo() {
    if (undoStack.empty())
        return;
    
    // Store current for redo
    AutomationState currentState;
    currentState.data = toVar();
    redoStack.push_back(currentState);
    
    // Get undo state
    auto state = undoStack.back();
    undoStack.pop_back();
    
    // Restore state
    loadFromVar(state.data);
}

void AutomationManager::redo() {
    if (redoStack.empty())
        return;
    
    // Store current for undo
    AutomationState currentState;
    currentState.data = toVar();
    undoStack.push_back(currentState);
    
    // Get redo state
    auto state = redoStack.back();
    redoStack.pop_back();
    
    // Restore state
    loadFromVar(state.data);
}

juce::var AutomationManager::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("globalMode", static_cast<int>(globalMode));
    obj->setProperty("playbackPosition", playbackPosition);
    
    juce::Array<juce::var> tracksArray;
    for (const auto& pair : trackAutomations) {
        juce::DynamicObject::Ptr trackObj = new juce::DynamicObject();
        trackObj->setProperty("index", pair.first);
        trackObj->setProperty("automation", pair.second->toVar());
        tracksArray.add(juce::var(trackObj.get()));
    }
    obj->setProperty("tracks", tracksArray);
    
    return juce::var(obj.get());
}

void AutomationManager::loadFromVar(const juce::var& v) {
    trackAutomations.clear();
    globalMode = static_cast<AutomationMode>((int)v["globalMode"]);
    playbackPosition = v["playbackPosition"];
    
    if (auto* tracksArray = v["tracks"].getArray()) {
        for (const auto& trackVar : *tracksArray) {
            int index = trackVar["index"];
            auto track = std::make_unique<TrackAutomation>();
            track->loadFromVar(trackVar["automation"]);
            trackAutomations[index] = std::move(track);
        }
    }
}

//==============================================================================
// PatternAutomation Implementation
//==============================================================================

PatternAutomation::PatternAutomation() = default;

juce::var PatternAutomation::CCEvent::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("time", timeInBeats);
    obj->setProperty("ccNumber", ccNumber);
    obj->setProperty("value", value);
    return juce::var(obj.get());
}

PatternAutomation::CCEvent PatternAutomation::CCEvent::fromVar(const juce::var& v) {
    CCEvent event;
    event.timeInBeats = v["time"];
    event.ccNumber = v["ccNumber"];
    event.value = v["value"];
    return event;
}

void PatternAutomation::addCCEvent(double time, int ccNum, int value) {
    events.push_back({time, ccNum, juce::jlimit(0, 127, value)});
    sortEvents();
}

void PatternAutomation::removeCCEvent(int index) {
    if (index >= 0 && index < static_cast<int>(events.size()))
        events.erase(events.begin() + index);
}

void PatternAutomation::clearAllEvents() {
    events.clear();
}

void PatternAutomation::sortEvents() {
    std::sort(events.begin(), events.end(),
        [](const CCEvent& a, const CCEvent& b) {
            return a.timeInBeats < b.timeInBeats;
        });
}

std::vector<PatternAutomation::CCEvent> PatternAutomation::getEventsInRange(double startBeat, double endBeat) const {
    std::vector<CCEvent> result;
    for (const auto& event : events) {
        if (event.timeInBeats >= startBeat && event.timeInBeats <= endBeat)
            result.push_back(event);
    }
    return result;
}

void PatternAutomation::quantizeToGrid(double gridSize) {
    for (auto& event : events) {
        double quantized = std::round(event.timeInBeats / gridSize) * gridSize;
        event.timeInBeats = quantized;
    }
    sortEvents();
}

void PatternAutomation::scaleValues(float multiplier) {
    for (auto& event : events)
        event.value = juce::jlimit(0, 127, static_cast<int>(event.value * multiplier));
}

juce::var PatternAutomation::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    juce::Array<juce::var> eventsArray;
    for (const auto& event : events)
        eventsArray.add(event.toVar());
    obj->setProperty("events", eventsArray);
    
    return juce::var(obj.get());
}

PatternAutomation PatternAutomation::fromVar(const juce::var& v) {
    PatternAutomation pattern;
    
    if (auto* eventsArray = v["events"].getArray()) {
        for (const auto& eventVar : *eventsArray)
            pattern.events.push_back(CCEvent::fromVar(eventVar));
    }
    
    return pattern;
}

} // namespace OmegaStudio
