#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace Audio {
namespace Recording {

/**
 * @struct Take
 * @brief Representa una toma de grabación individual
 */
struct Take {
    juce::String name;
    juce::AudioBuffer<float> audioData;
    double startTime { 0.0 };     // En beats/segundos
    double duration { 0.0 };
    bool isMuted { false };
    juce::Colour colour;
    juce::Uuid id;
    
    Take() : id(juce::Uuid()), colour(juce::Colours::cyan) {}
};

/**
 * @struct CompSegment
 * @brief Segmento seleccionado de una take para el comp final
 */
struct CompSegment {
    juce::Uuid takeId;
    double startTime { 0.0 };     // Dentro de la take
    double endTime { 0.0 };
    double fadeInLength { 0.01 };  // Segundos
    double fadeOutLength { 0.01 };
    
    bool contains(double time) const {
        return time >= startTime && time <= endTime;
    }
    
    double getLength() const { return endTime - startTime; }
};

/**
 * @class CompLane
 * @brief Lane de comping con múltiples takes
 */
class CompLane {
public:
    CompLane(const juce::String& name = "Comp Lane")
        : name_(name) {}
    
    void addTake(std::unique_ptr<Take> take) {
        takes_.push_back(std::move(take));
    }
    
    void removeTake(const juce::Uuid& takeId) {
        takes_.erase(
            std::remove_if(takes_.begin(), takes_.end(),
                [&takeId](const auto& take) { return take->id == takeId; }),
            takes_.end()
        );
    }
    
    Take* findTake(const juce::Uuid& takeId) {
        auto it = std::find_if(takes_.begin(), takes_.end(),
            [&takeId](const auto& take) { return take->id == takeId; });
        return it != takes_.end() ? it->get() : nullptr;
    }
    
    const std::vector<std::unique_ptr<Take>>& getTakes() const { return takes_; }
    
    // Comp segments management
    void addSegment(const CompSegment& segment) {
        segments_.push_back(segment);
        sortSegments();
    }
    
    void removeSegment(int index) {
        if (index >= 0 && index < segments_.size()) {
            segments_.erase(segments_.begin() + index);
        }
    }
    
    void clearSegments() {
        segments_.clear();
    }
    
    const std::vector<CompSegment>& getSegments() const { return segments_; }
    
    /**
     * Renderiza el comp final mezclando segmentos seleccionados
     */
    juce::AudioBuffer<float> renderComp(double sampleRate, int numChannels) {
        if (segments_.empty()) {
            return juce::AudioBuffer<float>(numChannels, 0);
        }
        
        // Calculate total length needed
        double maxTime = 0.0;
        for (const auto& seg : segments_) {
            maxTime = std::max(maxTime, seg.endTime);
        }
        
        int totalSamples = std::ceil(maxTime * sampleRate);
        juce::AudioBuffer<float> output(numChannels, totalSamples);
        output.clear();
        
        // Render each segment with crossfades
        for (const auto& segment : segments_) {
            auto* take = findTake(segment.takeId);
            if (take == nullptr) continue;
            
            int startSample = segment.startTime * sampleRate;
            int endSample = segment.endTime * sampleRate;
            int length = endSample - startSample;
            
            // Apply crossfade
            for (int ch = 0; ch < numChannels && ch < take->audioData.getNumChannels(); ++ch) {
                const float* src = take->audioData.getReadPointer(ch);
                float* dst = output.getWritePointer(ch);
                
                int fadeInSamples = segment.fadeInLength * sampleRate;
                int fadeOutSamples = segment.fadeOutLength * sampleRate;
                
                for (int i = 0; i < length; ++i) {
                    int srcIdx = startSample + i;
                    int dstIdx = startSample + i;
                    
                    if (srcIdx >= 0 && srcIdx < take->audioData.getNumSamples() &&
                        dstIdx >= 0 && dstIdx < output.getNumSamples()) {
                        
                        float sample = src[srcIdx];
                        
                        // Fade in
                        if (i < fadeInSamples) {
                            sample *= (float)i / fadeInSamples;
                        }
                        
                        // Fade out
                        if (i > length - fadeOutSamples) {
                            sample *= (float)(length - i) / fadeOutSamples;
                        }
                        
                        dst[dstIdx] += sample;  // Mix with existing
                    }
                }
            }
        }
        
        return output;
    }
    
    const juce::String& getName() const { return name_; }
    void setName(const juce::String& name) { name_ = name; }
    
private:
    void sortSegments() {
        std::sort(segments_.begin(), segments_.end(),
            [](const CompSegment& a, const CompSegment& b) {
                return a.startTime < b.startTime;
            });
    }
    
    juce::String name_;
    std::vector<std::unique_ptr<Take>> takes_;
    std::vector<CompSegment> segments_;
};

/**
 * @class CompingSystem
 * @brief Sistema completo de comping multi-take
 */
class CompingSystem {
public:
    CompingSystem() = default;
    
    void addLane(std::unique_ptr<CompLane> lane) {
        lanes_.push_back(std::move(lane));
    }
    
    CompLane* getLane(int index) {
        if (index >= 0 && index < lanes_.size()) {
            return lanes_[index].get();
        }
        return nullptr;
    }
    
    int getNumLanes() const { return lanes_.size(); }
    
    /**
     * Auto-comp: selecciona automáticamente los mejores segmentos
     * basado en RMS energy (criterio simple)
     */
    void autoComp(int laneIndex, double segmentLength = 4.0) {
        auto* lane = getLane(laneIndex);
        if (lane == nullptr) return;
        
        lane->clearSegments();
        
        const auto& takes = lane->getTakes();
        if (takes.empty()) return;
        
        // Find loudest segments
        double currentTime = 0.0;
        double endTime = takes[0]->duration;
        
        while (currentTime < endTime) {
            double segEnd = std::min(currentTime + segmentLength, endTime);
            
            // Find best take for this time range
            int bestTakeIdx = 0;
            float maxRMS = 0.0f;
            
            for (int t = 0; t < takes.size(); ++t) {
                float rms = calculateRMS(takes[t]->audioData, 
                                        currentTime, segEnd, 
                                        takes[t]->audioData.getNumSamples() / takes[t]->duration);
                if (rms > maxRMS) {
                    maxRMS = rms;
                    bestTakeIdx = t;
                }
            }
            
            // Add segment
            CompSegment seg;
            seg.takeId = takes[bestTakeIdx]->id;
            seg.startTime = currentTime;
            seg.endTime = segEnd;
            seg.fadeInLength = 0.01;
            seg.fadeOutLength = 0.01;
            lane->addSegment(seg);
            
            currentTime = segEnd;
        }
    }
    
private:
    float calculateRMS(const juce::AudioBuffer<float>& buffer, 
                      double startTime, double endTime, double sampleRate) {
        int startSample = startTime * sampleRate;
        int endSample = endTime * sampleRate;
        int length = endSample - startSample;
        
        if (length <= 0) return 0.0f;
        
        float sumSquares = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = startSample; i < endSample && i < buffer.getNumSamples(); ++i) {
                sumSquares += data[i] * data[i];
            }
        }
        
        return std::sqrt(sumSquares / (length * buffer.getNumChannels()));
    }
    
    std::vector<std::unique_ptr<CompLane>> lanes_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompingSystem)
};

} // namespace Recording
} // namespace Audio
} // namespace OmegaStudio
