//==============================================================================
// AutomationClip.cpp - Implementation
//==============================================================================

#include "AutomationClip.h"
#include <algorithm>
#include <cmath>

namespace OmegaStudio {

//==============================================================================
// AutomationClip Implementation
//==============================================================================

AutomationClip::AutomationClip(const juce::String& name)
    : name_(name) {
    // Agregar puntos por defecto
    points_.push_back({0.0, 0.5f, 0.0f, InterpolationType::Linear});
    points_.push_back({4.0, 0.5f, 0.0f, InterpolationType::Linear});
}

void AutomationClip::setLength(double lengthInBeats) {
    lengthInBeats_ = std::max(0.25, lengthInBeats);
}

void AutomationClip::addPoint(const AutomationPoint& point) {
    points_.push_back(point);
    sortPoints();
}

void AutomationClip::addPoint(double time, float value) {
    AutomationPoint point;
    point.time = time;
    point.value = juce::jlimit(0.0f, 1.0f, value);
    point.tension = 0.0f;
    point.type = InterpolationType::Linear;
    
    addPoint(point);
}

void AutomationClip::removePoint(int index) {
    if (index >= 0 && index < static_cast<int>(points_.size())) {
        points_.erase(points_.begin() + index);
    }
}

void AutomationClip::removePointAt(double time) {
    int index = findPointIndexAt(time);
    if (index >= 0) {
        removePoint(index);
    }
}

void AutomationClip::clearPoints() {
    points_.clear();
}

void AutomationClip::setPointValue(int index, float value) {
    if (index >= 0 && index < static_cast<int>(points_.size())) {
        points_[index].value = juce::jlimit(0.0f, 1.0f, value);
    }
}

void AutomationClip::setPointTime(int index, double time) {
    if (index >= 0 && index < static_cast<int>(points_.size())) {
        points_[index].time = time;
        sortPoints();
    }
}

void AutomationClip::setPointTension(int index, float tension) {
    if (index >= 0 && index < static_cast<int>(points_.size())) {
        points_[index].tension = juce::jlimit(-1.0f, 1.0f, tension);
    }
}

void AutomationClip::setPointType(int index, InterpolationType type) {
    if (index >= 0 && index < static_cast<int>(points_.size())) {
        points_[index].type = type;
    }
}

//==============================================================================
float AutomationClip::getValueAtTime(double time) const {
    return getValueAtBeat(time);
}

float AutomationClip::getValueAtBeat(double beat) const {
    if (points_.empty()) return 0.5f;
    if (points_.size() == 1) return points_[0].value;
    
    // Wrapping (loop)
    while (beat < 0.0) beat += lengthInBeats_;
    while (beat >= lengthInBeats_) beat -= lengthInBeats_;
    
    // Encontrar puntos adyacentes
    int nextIndex = -1;
    for (int i = 0; i < static_cast<int>(points_.size()); ++i) {
        if (points_[i].time > beat) {
            nextIndex = i;
            break;
        }
    }
    
    if (nextIndex == -1) {
        // Después del último punto
        return points_.back().value;
    }
    
    if (nextIndex == 0) {
        // Antes del primer punto
        return points_[0].value;
    }
    
    const auto& p0 = points_[nextIndex - 1];
    const auto& p1 = points_[nextIndex];
    
    // Calcular factor de interpolación
    const double duration = p1.time - p0.time;
    if (duration <= 0.0) return p0.value;
    
    const float t = static_cast<float>((beat - p0.time) / duration);
    
    // Aplicar interpolación según tipo
    switch (p0.type) {
        case InterpolationType::Linear:
            return linearInterpolate(p0.value, p1.value, t);
            
        case InterpolationType::Curve:
            return curveInterpolate(p0.value, p1.value, p0.tension, t);
            
        case InterpolationType::Step:
            return p0.value;
            
        case InterpolationType::Hold:
            return p0.value;
            
        case InterpolationType::Exponential:
            return exponentialInterpolate(p0.value, p1.value, t);
            
        default:
            return linearInterpolate(p0.value, p1.value, t);
    }
}

//==============================================================================
void AutomationClip::setValueRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
}

double AutomationClip::snapTime(double time) const {
    if (!snapEnabled_) return time;
    
    return std::round(time / snapResolution_) * snapResolution_;
}

//==============================================================================
void AutomationClip::scaleTime(double factor) {
    for (auto& point : points_) {
        point.time *= factor;
    }
    lengthInBeats_ *= factor;
}

void AutomationClip::scaleValues(float factor) {
    for (auto& point : points_) {
        point.value = juce::jlimit(0.0f, 1.0f, point.value * factor);
    }
}

void AutomationClip::offsetValues(float offset) {
    for (auto& point : points_) {
        point.value = juce::jlimit(0.0f, 1.0f, point.value + offset);
    }
}

void AutomationClip::invertValues() {
    for (auto& point : points_) {
        point.value = 1.0f - point.value;
    }
}

void AutomationClip::smoothValues(int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        if (points_.size() < 3) return;
        
        std::vector<float> newValues(points_.size());
        
        // Mantener primer y último punto
        newValues[0] = points_[0].value;
        newValues[points_.size() - 1] = points_.back().value;
        
        // Suavizar puntos intermedios (promedio con vecinos)
        for (size_t i = 1; i < points_.size() - 1; ++i) {
            newValues[i] = (points_[i - 1].value + points_[i].value * 2.0f + points_[i + 1].value) / 4.0f;
        }
        
        // Aplicar nuevos valores
        for (size_t i = 0; i < points_.size(); ++i) {
            points_[i].value = newValues[i];
        }
    }
}

//==============================================================================
void AutomationClip::generateLFO(double startTime, double endTime, float frequency, 
                                  float depth, float phase, const juce::String& waveform) {
    clearPoints();
    
    const double duration = endTime - startTime;
    const int numPoints = static_cast<int>(std::ceil(frequency * duration * 4)); // 4 puntos por ciclo
    
    for (int i = 0; i <= numPoints; ++i) {
        const double t = startTime + (duration * i) / numPoints;
        const float normalizedTime = static_cast<float>((t - startTime) / duration);
        const float angle = juce::MathConstants<float>::twoPi * frequency * normalizedTime + phase;
        
        float value = 0.5f;
        
        if (waveform == "sine") {
            value = 0.5f + 0.5f * std::sin(angle) * depth;
        } else if (waveform == "triangle") {
            float triangleValue = 2.0f * std::abs(2.0f * (angle / juce::MathConstants<float>::twoPi - 
                                   std::floor(angle / juce::MathConstants<float>::twoPi + 0.5f))) - 1.0f;
            value = 0.5f + 0.5f * triangleValue * depth;
        } else if (waveform == "square") {
            value = std::sin(angle) >= 0.0f ? (0.5f + 0.5f * depth) : (0.5f - 0.5f * depth);
        } else if (waveform == "sawtooth") {
            float sawValue = 2.0f * (angle / juce::MathConstants<float>::twoPi - 
                             std::floor(angle / juce::MathConstants<float>::twoPi + 0.5f));
            value = 0.5f + 0.5f * sawValue * depth;
        }
        
        addPoint(t, value);
    }
}

//==============================================================================
void AutomationClip::loadPreset(const juce::String& presetName) {
    AutomationPresets::applyPreset(*this, presetName);
}

juce::StringArray AutomationClip::getPresetList() const {
    return {
        "Fade In",
        "Fade Out",
        "Sidechain Pump",
        "Wobble",
        "Stutter",
        "Filter Sweep",
        "Random Walk"
    };
}

//==============================================================================
float AutomationClip::linearInterpolate(float v0, float v1, float t) const {
    return v0 + t * (v1 - v0);
}

float AutomationClip::curveInterpolate(float v0, float v1, float tension, float t) const {
    // Hermite interpolation con tension
    const float t2 = t * t;
    const float t3 = t2 * t;
    
    const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    const float h10 = t3 - 2.0f * t2 + t;
    const float h01 = -2.0f * t3 + 3.0f * t2;
    const float h11 = t3 - t2;
    
    const float m0 = (1.0f - tension) * (v1 - v0);
    const float m1 = (1.0f - tension) * (v1 - v0);
    
    return h00 * v0 + h10 * m0 + h01 * v1 + h11 * m1;
}

float AutomationClip::exponentialInterpolate(float v0, float v1, float t) const {
    // Para frecuencias (exponencial)
    if (v0 <= 0.0f || v1 <= 0.0f) {
        return linearInterpolate(v0, v1, t);
    }
    
    const float logV0 = std::log(v0);
    const float logV1 = std::log(v1);
    const float logResult = logV0 + t * (logV1 - logV0);
    
    return std::exp(logResult);
}

void AutomationClip::sortPoints() {
    std::sort(points_.begin(), points_.end());
}

int AutomationClip::findPointIndexAt(double time) const {
    const double threshold = 0.01;  // 10ms tolerance
    
    for (int i = 0; i < static_cast<int>(points_.size()); ++i) {
        if (std::abs(points_[i].time - time) < threshold) {
            return i;
        }
    }
    
    return -1;
}

//==============================================================================
juce::ValueTree AutomationClip::toValueTree() const {
    juce::ValueTree tree("AutomationClip");
    tree.setProperty("name", name_, nullptr);
    tree.setProperty("parameterName", parameterName_, nullptr);
    tree.setProperty("length", lengthInBeats_, nullptr);
    tree.setProperty("minValue", minValue_, nullptr);
    tree.setProperty("maxValue", maxValue_, nullptr);
    
    juce::ValueTree pointsTree("Points");
    for (const auto& point : points_) {
        juce::ValueTree pointTree("Point");
        pointTree.setProperty("time", point.time, nullptr);
        pointTree.setProperty("value", point.value, nullptr);
        pointTree.setProperty("tension", point.tension, nullptr);
        pointTree.setProperty("type", static_cast<int>(point.type), nullptr);
        pointsTree.appendChild(pointTree, nullptr);
    }
    tree.appendChild(pointsTree, nullptr);
    
    return tree;
}

void AutomationClip::fromValueTree(const juce::ValueTree& tree) {
    name_ = tree.getProperty("name", "Automation");
    parameterName_ = tree.getProperty("parameterName", "");
    lengthInBeats_ = tree.getProperty("length", 4.0);
    minValue_ = tree.getProperty("minValue", 0.0f);
    maxValue_ = tree.getProperty("maxValue", 1.0f);
    
    points_.clear();
    
    juce::ValueTree pointsTree = tree.getChildWithName("Points");
    for (int i = 0; i < pointsTree.getNumChildren(); ++i) {
        juce::ValueTree pointTree = pointsTree.getChild(i);
        
        AutomationPoint point;
        point.time = pointTree.getProperty("time", 0.0);
        point.value = pointTree.getProperty("value", 0.5f);
        point.tension = pointTree.getProperty("tension", 0.0f);
        point.type = static_cast<InterpolationType>(static_cast<int>(pointTree.getProperty("type", 0)));
        
        points_.push_back(point);
    }
}

//==============================================================================
// AutomationManager Implementation
//==============================================================================

AutomationClip* AutomationManager::createClip(const juce::String& name) {
    auto clip = std::make_unique<AutomationClip>(name);
    AutomationClip* ptr = clip.get();
    clips_.push_back(std::move(clip));
    return ptr;
}

void AutomationManager::deleteClip(int index) {
    if (index >= 0 && index < static_cast<int>(clips_.size())) {
        const juce::String name = clips_[index]->getName();
        clipToParameter_.erase(name);
        clips_.erase(clips_.begin() + index);
    }
}

void AutomationManager::deleteClip(const juce::String& name) {
    for (int i = 0; i < static_cast<int>(clips_.size()); ++i) {
        if (clips_[i]->getName() == name) {
            deleteClip(i);
            return;
        }
    }
}

AutomationClip* AutomationManager::getClip(int index) {
    if (index >= 0 && index < static_cast<int>(clips_.size())) {
        return clips_[index].get();
    }
    return nullptr;
}

AutomationClip* AutomationManager::getClip(const juce::String& name) {
    for (auto& clip : clips_) {
        if (clip->getName() == name) {
            return clip.get();
        }
    }
    return nullptr;
}

void AutomationManager::bindClipToParameter(const juce::String& clipName, const juce::String& parameterPath) {
    clipToParameter_[clipName] = parameterPath;
}

void AutomationManager::unbindClip(const juce::String& clipName) {
    clipToParameter_.erase(clipName);
}

juce::String AutomationManager::getParameterForClip(const juce::String& clipName) const {
    auto it = clipToParameter_.find(clipName);
    if (it != clipToParameter_.end()) {
        return it->second;
    }
    return {};
}

std::vector<juce::String> AutomationManager::getClipsForParameter(const juce::String& parameterPath) const {
    std::vector<juce::String> result;
    
    for (const auto& pair : clipToParameter_) {
        if (pair.second == parameterPath) {
            result.push_back(pair.first);
        }
    }
    
    return result;
}

void AutomationManager::setPlayheadPosition(double timeInBeats) {
    playheadPosition_ = timeInBeats;
}

void AutomationManager::updateAutomation() {
    // Este método sería llamado por el audio engine para actualizar parámetros
}

std::map<juce::String, float> AutomationManager::getCurrentAutomationValues() const {
    std::map<juce::String, float> values;
    
    for (const auto& pair : clipToParameter_) {
        AutomationClip* clip = const_cast<AutomationManager*>(this)->getClip(pair.first);
        if (clip) {
            values[pair.second] = clip->getValueAtBeat(playheadPosition_);
        }
    }
    
    return values;
}

juce::ValueTree AutomationManager::toValueTree() const {
    juce::ValueTree tree("AutomationManager");
    
    for (const auto& clip : clips_) {
        tree.appendChild(clip->toValueTree(), nullptr);
    }
    
    juce::ValueTree bindingsTree("Bindings");
    for (const auto& pair : clipToParameter_) {
        juce::ValueTree binding("Binding");
        binding.setProperty("clip", pair.first, nullptr);
        binding.setProperty("parameter", pair.second, nullptr);
        bindingsTree.appendChild(binding, nullptr);
    }
    tree.appendChild(bindingsTree, nullptr);
    
    return tree;
}

void AutomationManager::fromValueTree(const juce::ValueTree& tree) {
    clips_.clear();
    clipToParameter_.clear();
    
    for (int i = 0; i < tree.getNumChildren(); ++i) {
        juce::ValueTree child = tree.getChild(i);
        
        if (child.hasType("AutomationClip")) {
            auto clip = std::make_unique<AutomationClip>();
            clip->fromValueTree(child);
            clips_.push_back(std::move(clip));
        }
        else if (child.hasType("Bindings")) {
            for (int j = 0; j < child.getNumChildren(); ++j) {
                juce::ValueTree binding = child.getChild(j);
                juce::String clipName = binding.getProperty("clip", "");
                juce::String paramPath = binding.getProperty("parameter", "");
                clipToParameter_[clipName] = paramPath;
            }
        }
    }
}

} // namespace OmegaStudio
