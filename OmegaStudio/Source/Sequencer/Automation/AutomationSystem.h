/*
  ==============================================================================
    AutomationSystem.h
    
    Sistema profesional de automatización de parámetros:
    - Automation lanes con múltiples curvas
    - Recording de automatización en tiempo real
    - Edición de curves (Linear, Bezier, Step, Exponential)
    - Touch, Latch, Write automation modes
    - Undo/Redo de automation edits
    - Pattern automation para MIDI CCs
    
    Enterprise-grade automation system
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace OmegaStudio {

//==============================================================================
/** Tipo de curva de automatización */
enum class AutomationCurveType {
    Linear,         // Interpolación lineal
    Bezier,         // Curva bezier suave
    Step,           // Sin interpolación (steps)
    Exponential,    // Curva exponencial
    Logarithmic     // Curva logarítmica
};

/** Modo de grabación de automatización */
enum class AutomationMode {
    Off,            // Sin automatización
    Read,           // Solo lectura
    Touch,          // Graba mientras se toca, vuelve a read al soltar
    Latch,          // Graba desde que se toca hasta stop
    Write           // Graba todo el tiempo
};

//==============================================================================
/** Punto de automatización individual */
struct AutomationPoint {
    double timeInBeats { 0.0 };
    float value { 0.0f };
    AutomationCurveType curveType { AutomationCurveType::Linear };
    float curvature { 0.5f };  // 0.0-1.0 para Bezier/Exp/Log
    
    AutomationPoint() = default;
    AutomationPoint(double time, float val, AutomationCurveType curve = AutomationCurveType::Linear)
        : timeInBeats(time), value(val), curveType(curve) {}
    
    juce::var toVar() const;
    static AutomationPoint fromVar(const juce::var& v);
};

//==============================================================================
/** Lane de automatización - contiene todos los puntos para un parámetro */
class AutomationLane {
public:
    AutomationLane(const juce::String& paramId, float defaultValue = 0.0f);
    ~AutomationLane() = default;
    
    // Parameter info
    juce::String getParameterID() const { return parameterID; }
    void setParameterName(const juce::String& name) { parameterName = name; }
    juce::String getParameterName() const { return parameterName; }
    
    float getDefaultValue() const { return defaultValue; }
    void setDefaultValue(float val) { defaultValue = val; }
    
    // Mode
    AutomationMode getMode() const { return mode; }
    void setMode(AutomationMode m) { mode = m; }
    
    // Points management
    void addPoint(const AutomationPoint& point);
    void addPoint(double timeInBeats, float value, AutomationCurveType curve = AutomationCurveType::Linear);
    void removePoint(int index);
    void removePointsInRange(double startBeat, double endBeat);
    void clearAllPoints();
    
    int getNumPoints() const { return static_cast<int>(points.size()); }
    AutomationPoint& getPoint(int index) { return points[index]; }
    const AutomationPoint& getPoint(int index) const { return points[index]; }
    const std::vector<AutomationPoint>& getPoints() const { return points; }
    
    // Value interpolation
    float getValueAtTime(double timeInBeats) const;
    
    // Editing
    void movePoint(int index, double newTime, float newValue);
    void setCurveType(int index, AutomationCurveType type);
    void setCurvature(int index, float curvature);
    
    // Range operations
    void scaleValues(float multiplier);
    void offsetValues(float offset);
    void quantizeToGrid(double gridSize);
    
    // Serialization
    juce::var toVar() const;
    static AutomationLane fromVar(const juce::var& v);
    
private:
    juce::String parameterID;
    juce::String parameterName;
    float defaultValue { 0.0f };
    AutomationMode mode { AutomationMode::Read };
    
    std::vector<AutomationPoint> points;
    
    void sortPoints();
    int findPointIndexAtTime(double time, double tolerance = 0.001) const;
    
    float interpolateLinear(const AutomationPoint& p1, const AutomationPoint& p2, double time) const;
    float interpolateBezier(const AutomationPoint& p1, const AutomationPoint& p2, double time) const;
    float interpolateExponential(const AutomationPoint& p1, const AutomationPoint& p2, double time) const;
    float interpolateLogarithmic(const AutomationPoint& p1, const AutomationPoint& p2, double time) const;
};

//==============================================================================
/** Manager de automatización para una pista */
class TrackAutomation {
public:
    TrackAutomation();
    ~TrackAutomation() = default;
    
    // Lanes management
    AutomationLane* addLane(const juce::String& paramID, float defaultValue = 0.0f);
    void removeLane(const juce::String& paramID);
    AutomationLane* getLane(const juce::String& paramID);
    const AutomationLane* getLane(const juce::String& paramID) const;
    
    int getNumLanes() const { return static_cast<int>(lanes.size()); }
    std::vector<juce::String> getLaneIDs() const;
    
    // Automation reading
    std::map<juce::String, float> getValuesAtTime(double timeInBeats) const;
    float getValueAtTime(const juce::String& paramID, double timeInBeats) const;
    
    // Recording
    void startRecording(const juce::String& paramID);
    void stopRecording(const juce::String& paramID);
    void recordPoint(const juce::String& paramID, double timeInBeats, float value);
    bool isRecording(const juce::String& paramID) const;
    
    // Clear all
    void clearAllAutomation();
    
    // Serialization
    juce::var toVar() const;
    void loadFromVar(const juce::var& v);
    
    // Direct access for stats
    const std::map<juce::String, std::unique_ptr<AutomationLane>>& getLanes() const { return lanes; }
    
private:
    std::map<juce::String, std::unique_ptr<AutomationLane>> lanes;
    std::set<juce::String> recordingLanes;
};

//==============================================================================
/** Global automation manager para el proyecto */
class AutomationManager {
public:
    AutomationManager();
    ~AutomationManager() = default;
    
    // Track automation
    TrackAutomation* getTrackAutomation(int trackIndex);
    const TrackAutomation* getTrackAutomation(int trackIndex) const;
    void ensureTrackAutomation(int trackIndex);
    void removeTrackAutomation(int trackIndex);
    
    // Global automation mode
    void setGlobalMode(AutomationMode mode);
    AutomationMode getGlobalMode() const { return globalMode; }
    
    // Recording
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording; }
    
    // Playback
    void setPlaybackPosition(double timeInBeats);
    double getPlaybackPosition() const { return playbackPosition; }
    
    // Apply automation to parameters
    using ParameterCallback = std::function<void(int trackIndex, const juce::String& paramID, float value)>;
    void setParameterCallback(ParameterCallback callback) { parameterCallback = callback; }
    void applyAutomationAtCurrentTime();
    
    // Undo/Redo
    void pushUndoState();
    void undo();
    void redo();
    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    
    // Serialization
    juce::var toVar() const;
    void loadFromVar(const juce::var& v);
    
    // Stats
    int getParameterCount() const { return static_cast<int>(trackAutomations.size()); }
    
    int getTotalPointCount() const {
        int total = 0;
        for (const auto& [id, track] : trackAutomations)
            for (const auto& [paramID, lane] : track->getLanes())
                total += static_cast<int>(lane->getPoints().size());
        return total;
    }
    
private:
    std::map<int, std::unique_ptr<TrackAutomation>> trackAutomations;
    
    AutomationMode globalMode { AutomationMode::Read };
    bool recording { false };
    double playbackPosition { 0.0 };
    
    ParameterCallback parameterCallback;
    
    // Undo/Redo
    struct AutomationState {
        juce::var data;
    };
    std::vector<AutomationState> undoStack;
    std::vector<AutomationState> redoStack;
    int maxUndoLevels { 100 };
};

//==============================================================================
/** Pattern automation para MIDI CC */
class PatternAutomation {
public:
    PatternAutomation();
    ~PatternAutomation() = default;
    
    // CC Events
    struct CCEvent {
        double timeInBeats;
        int ccNumber;
        int value;  // 0-127
        
        juce::var toVar() const;
        static CCEvent fromVar(const juce::var& v);
    };
    
    void addCCEvent(double time, int ccNum, int value);
    void removeCCEvent(int index);
    void clearAllEvents();
    
    int getNumEvents() const { return static_cast<int>(events.size()); }
    const CCEvent& getEvent(int index) const { return events[index]; }
    const std::vector<CCEvent>& getEvents() const { return events; }
    
    // Get events in range
    std::vector<CCEvent> getEventsInRange(double startBeat, double endBeat) const;
    
    // Editing
    void quantizeToGrid(double gridSize);
    void scaleValues(float multiplier);
    
    // Serialization
    juce::var toVar() const;
    static PatternAutomation fromVar(const juce::var& v);
    
private:
    std::vector<CCEvent> events;
    void sortEvents();
};

} // namespace OmegaStudio
