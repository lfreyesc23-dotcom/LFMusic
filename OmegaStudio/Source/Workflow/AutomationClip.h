//==============================================================================
// AutomationClip.h - Sistema de Automation Clips (FL Studio style)
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>

namespace OmegaStudio {

//==============================================================================
/** Automation Clip - Control de parámetros en el tiempo
 *  - Curvas de automatización dibujables
 *  - Múltiples tipos de interpolación (Linear, Curve, Step, Hold)
 *  - Snap to grid
 *  - Copy/paste de clips
 *  - Automation de cualquier parámetro
 *  - Link a múltiples parámetros
 */
class AutomationClip {
public:
    //==========================================================================
    enum class InterpolationType {
        Linear,         // Interpolación lineal
        Curve,          // Curva suave (Bezier)
        Step,           // Pasos (sin interpolación)
        Hold,           // Mantener valor hasta siguiente punto
        Exponential     // Exponencial (para frecuencias)
    };
    
    //==========================================================================
    struct AutomationPoint {
        double time = 0.0;              // Tiempo en beats
        float value = 0.0f;             // Valor normalizado (0.0 - 1.0)
        float tension = 0.0f;           // Tensión de curva (-1.0 a 1.0)
        InterpolationType type = InterpolationType::Linear;
        
        bool operator<(const AutomationPoint& other) const {
            return time < other.time;
        }
    };
    
    //==========================================================================
    AutomationClip(const juce::String& name = "Automation");
    ~AutomationClip() = default;
    
    // Identification
    void setName(const juce::String& name) { name_ = name; }
    juce::String getName() const { return name_; }
    
    void setParameterName(const juce::String& paramName) { parameterName_ = paramName; }
    juce::String getParameterName() const { return parameterName_; }
    
    // Length
    void setLength(double lengthInBeats);
    double getLength() const { return lengthInBeats_; }
    
    // Points management
    void addPoint(const AutomationPoint& point);
    void addPoint(double time, float value);
    void removePoint(int index);
    void removePointAt(double time);
    void clearPoints();
    
    void setPointValue(int index, float value);
    void setPointTime(int index, double time);
    void setPointTension(int index, float tension);
    void setPointType(int index, InterpolationType type);
    
    int getNumPoints() const { return static_cast<int>(points_.size()); }
    const AutomationPoint& getPoint(int index) const { return points_[index]; }
    const std::vector<AutomationPoint>& getAllPoints() const { return points_; }
    
    // Value queries
    float getValueAtTime(double time) const;
    float getValueAtBeat(double beat) const;
    
    // Range
    void setValueRange(float min, float max);
    float getMinValue() const { return minValue_; }
    float getMaxValue() const { return maxValue_; }
    
    // Grid snap
    void setSnapEnabled(bool enabled) { snapEnabled_ = enabled; }
    bool isSnapEnabled() const { return snapEnabled_; }
    
    void setSnapResolution(double beats) { snapResolution_ = beats; }
    double getSnapResolution() const { return snapResolution_; }
    
    double snapTime(double time) const;
    
    // Editing operations
    void scaleTime(double factor);
    void scaleValues(float factor);
    void offsetValues(float offset);
    void invertValues();
    void smoothValues(int iterations = 1);
    
    // LFO Generator
    void generateLFO(double startTime, double endTime, float frequency, 
                     float depth, float phase, const juce::String& waveform);
    
    // Presets
    void loadPreset(const juce::String& presetName);
    juce::StringArray getPresetList() const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    juce::String name_;
    juce::String parameterName_;
    
    double lengthInBeats_ = 4.0;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    
    std::vector<AutomationPoint> points_;
    
    bool snapEnabled_ = true;
    double snapResolution_ = 0.25;  // 1/16 note
    
    // Interpolation helpers
    float linearInterpolate(float v0, float v1, float t) const;
    float curveInterpolate(float v0, float v1, float tension, float t) const;
    float exponentialInterpolate(float v0, float v1, float t) const;
    
    void sortPoints();
    int findPointIndexAt(double time) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationClip)
};

//==============================================================================
/** Gestor de múltiples Automation Clips */
class AutomationManager {
public:
    AutomationManager() = default;
    ~AutomationManager() = default;
    
    // Clip management
    AutomationClip* createClip(const juce::String& name);
    void deleteClip(int index);
    void deleteClip(const juce::String& name);
    
    int getNumClips() const { return static_cast<int>(clips_.size()); }
    AutomationClip* getClip(int index);
    AutomationClip* getClip(const juce::String& name);
    
    // Parameter binding
    void bindClipToParameter(const juce::String& clipName, const juce::String& parameterPath);
    void unbindClip(const juce::String& clipName);
    
    juce::String getParameterForClip(const juce::String& clipName) const;
    std::vector<juce::String> getClipsForParameter(const juce::String& parameterPath) const;
    
    // Playback
    void setPlayheadPosition(double timeInBeats);
    void updateAutomation();
    
    // Get current values for all automated parameters
    std::map<juce::String, float> getCurrentAutomationValues() const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    std::vector<std::unique_ptr<AutomationClip>> clips_;
    std::map<juce::String, juce::String> clipToParameter_;  // clip name -> parameter path
    
    double playheadPosition_ = 0.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationManager)
};

//==============================================================================
/** Presets de LFO comunes */
class AutomationPresets {
public:
    static void applyPreset(AutomationClip& clip, const juce::String& presetName) {
        clip.clearPoints();
        
        if (presetName == "Fade In") {
            clip.addPoint(0.0, 0.0f);
            clip.addPoint(clip.getLength(), 1.0f);
        }
        else if (presetName == "Fade Out") {
            clip.addPoint(0.0, 1.0f);
            clip.addPoint(clip.getLength(), 0.0f);
        }
        else if (presetName == "Sidechain Pump") {
            const double length = clip.getLength();
            for (int i = 0; i < 4; ++i) {
                double t = (length / 4.0) * i;
                clip.addPoint(t, 0.0f);
                clip.addPoint(t + 0.01, 1.0f);
            }
        }
        else if (presetName == "Wobble") {
            clip.generateLFO(0.0, clip.getLength(), 4.0f, 1.0f, 0.0f, "sine");
        }
        else if (presetName == "Stutter") {
            const double length = clip.getLength();
            for (int i = 0; i < 16; ++i) {
                double t = (length / 16.0) * i;
                clip.addPoint(t, i % 2 == 0 ? 1.0f : 0.0f);
                auto& point = clip.getAllPoints().back();
                const_cast<AutomationClip::AutomationPoint&>(point).type = 
                    AutomationClip::InterpolationType::Step;
            }
        }
        else if (presetName == "Filter Sweep") {
            clip.addPoint(0.0, 0.0f);
            auto& p1 = clip.getAllPoints().back();
            const_cast<AutomationClip::AutomationPoint&>(p1).type = 
                AutomationClip::InterpolationType::Exponential;
            
            clip.addPoint(clip.getLength(), 1.0f);
        }
        else if (presetName == "Random Walk") {
            juce::Random random;
            const double length = clip.getLength();
            const int numPoints = 16;
            
            for (int i = 0; i < numPoints; ++i) {
                double t = (length / numPoints) * i;
                float value = random.nextFloat();
                clip.addPoint(t, value);
            }
            
            clip.smoothValues(2);
        }
    }
};

} // namespace OmegaStudio
