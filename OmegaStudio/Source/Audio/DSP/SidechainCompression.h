#pragma once

#include <JuceHeader.h>
#include "../Graph/AudioNode.h"
#include <memory>
#include <vector>

namespace OmegaStudio {
namespace Audio {
namespace DSP {

/**
 * @class SidechainCompressor
 * @brief Compresor con entrada de sidechain externa
 */
class SidechainCompressor {
public:
    SidechainCompressor() = default;
    
    struct Parameters {
        float threshold { -20.0f };   // dB
        float ratio { 4.0f };         // 1:1 to inf:1
        float attackMs { 5.0f };
        float releaseMs { 50.0f };
        float knee { 3.0f };          // dB
        float makeupGain { 0.0f };    // dB
        bool autoMakeup { true };
        
        // Sidechain processing
        float sidechainHPF { 80.0f }; // Hz - filter out low freqs
        float sidechainLPF { 12000.0f }; // Hz
        bool externalSidechain { false };
    };
    
    void prepare(double sampleRate, int maxBlockSize) {
        sampleRate_ = sampleRate;
        
        // Ballistics coefficients
        updateCoefficients();
        
        // Sidechain filters
        sidechainHPF_.setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate, 80.0f));
        sidechainLPF_.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, 12000.0f));
        
        envelope_ = 0.0f;
        gainReduction_ = 0.0f;
    }
    
    void setParameters(const Parameters& params) {
        params_ = params;
        updateCoefficients();
        
        sidechainHPF_.setCoefficients(juce::IIRCoefficients::makeHighPass(sampleRate_, params_.sidechainHPF));
        sidechainLPF_.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate_, params_.sidechainLPF));
    }
    
    /**
     * Procesar audio con sidechain
     * @param mainBuffer Buffer principal (entrada/salida)
     * @param sidechainBuffer Buffer de sidechain (puede ser nullptr para usar el main como sidechain)
     */
    void process(juce::AudioBuffer<float>& mainBuffer, 
                const juce::AudioBuffer<float>* sidechainBuffer = nullptr) {
        
        const int numSamples = mainBuffer.getNumSamples();
        const int numChannels = mainBuffer.getNumChannels();
        
        // Determinar fuente de sidechain
        const juce::AudioBuffer<float>* scSource = 
            (params_.externalSidechain && sidechainBuffer != nullptr) 
            ? sidechainBuffer 
            : &mainBuffer;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // 1. Calcular nivel del sidechain
            float scLevel = calculateSidechainLevel(*scSource, sample);
            
            // 2. Aplicar filtros al sidechain
            scLevel = sidechainHPF_.processSingleSampleRaw(scLevel);
            scLevel = sidechainLPF_.processSingleSampleRaw(scLevel);
            
            // 3. Detector de envolvente
            float scLevelDB = juce::Decibels::gainToDecibels(std::abs(scLevel) + 1e-6f);
            
            if (scLevelDB > envelope_) {
                envelope_ += attackCoeff_ * (scLevelDB - envelope_);
            } else {
                envelope_ += releaseCoeff_ * (scLevelDB - envelope_);
            }
            
            // 4. Calcular gain reduction
            float gainReductionDB = 0.0f;
            
            if (envelope_ > params_.threshold) {
                float overshoot = envelope_ - params_.threshold;
                
                // Soft knee
                if (overshoot < params_.knee) {
                    float kneeScale = overshoot / params_.knee;
                    gainReductionDB = -overshoot * kneeScale * (1.0f - 1.0f / params_.ratio);
                } else {
                    gainReductionDB = -(overshoot - params_.knee * 0.5f) * (1.0f - 1.0f / params_.ratio);
                }
            }
            
            // Smooth gain reduction
            gainReduction_ += (gainReductionDB - gainReduction_) * 0.1f;
            
            // 5. Calcular gain total
            float makeupDB = params_.autoMakeup 
                ? -gainReduction_ * 0.5f 
                : params_.makeupGain;
            
            float totalGainDB = gainReduction_ + makeupDB;
            float gain = juce::Decibels::decibelsToGain(totalGainDB);
            
            // 6. Aplicar ganancia a todas las canales
            for (int ch = 0; ch < numChannels; ++ch) {
                float* channelData = mainBuffer.getWritePointer(ch);
                channelData[sample] *= gain;
            }
        }
    }
    
    float getCurrentGainReduction() const { 
        return gainReduction_; 
    }
    
    float getCurrentEnvelope() const { 
        return envelope_; 
    }
    
private:
    void updateCoefficients() {
        attackCoeff_ = 1.0f - std::exp(-1.0f / (params_.attackMs * 0.001f * sampleRate_));
        releaseCoeff_ = 1.0f - std::exp(-1.0f / (params_.releaseMs * 0.001f * sampleRate_));
    }
    
    float calculateSidechainLevel(const juce::AudioBuffer<float>& buffer, int sample) {
        float level = 0.0f;
        int numChannels = buffer.getNumChannels();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            level += std::abs(buffer.getSample(ch, sample));
        }
        
        return level / numChannels;
    }
    
    Parameters params_;
    double sampleRate_ { 44100.0 };
    
    float attackCoeff_ { 0.0f };
    float releaseCoeff_ { 0.0f };
    float envelope_ { 0.0f };
    float gainReduction_ { 0.0f };
    
    juce::IIRFilter sidechainHPF_;
    juce::IIRFilter sidechainLPF_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainCompressor)
};

/**
 * @class SidechainRouter
 * @brief Sistema de routing para sidechains
 */
class SidechainRouter {
public:
    struct Connection {
        int sourceNodeId { -1 };      // Nodo que envía señal
        int destinationNodeId { -1 }; // Nodo que recibe sidechain
        float amount { 1.0f };        // 0.0 - 1.0
        bool enabled { true };
        bool preFader { true };       // Pre o post-fader
        
        Connection() = default;
        Connection(int src, int dst) 
            : sourceNodeId(src), destinationNodeId(dst) {}
    };
    
    void addConnection(int sourceNode, int destinationNode) {
        connections_.push_back(Connection(sourceNode, destinationNode));
    }
    
    void removeConnection(int sourceNode, int destinationNode) {
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [=](const Connection& c) {
                    return c.sourceNodeId == sourceNode && 
                           c.destinationNodeId == destinationNode;
                }),
            connections_.end()
        );
    }
    
    std::vector<Connection> getConnectionsForDestination(int nodeId) const {
        std::vector<Connection> result;
        for (const auto& conn : connections_) {
            if (conn.destinationNodeId == nodeId && conn.enabled) {
                result.push_back(conn);
            }
        }
        return result;
    }
    
    std::vector<Connection> getConnectionsFromSource(int nodeId) const {
        std::vector<Connection> result;
        for (const auto& conn : connections_) {
            if (conn.sourceNodeId == nodeId && conn.enabled) {
                result.push_back(conn);
            }
        }
        return result;
    }
    
    bool hasConnection(int sourceNode, int destinationNode) const {
        for (const auto& conn : connections_) {
            if (conn.sourceNodeId == sourceNode && 
                conn.destinationNodeId == destinationNode) {
                return true;
            }
        }
        return false;
    }
    
    void setConnectionAmount(int sourceNode, int destinationNode, float amount) {
        for (auto& conn : connections_) {
            if (conn.sourceNodeId == sourceNode && 
                conn.destinationNodeId == destinationNode) {
                conn.amount = juce::jlimit(0.0f, 1.0f, amount);
            }
        }
    }
    
    void setConnectionEnabled(int sourceNode, int destinationNode, bool enabled) {
        for (auto& conn : connections_) {
            if (conn.sourceNodeId == sourceNode && 
                conn.destinationNodeId == destinationNode) {
                conn.enabled = enabled;
            }
        }
    }
    
    const std::vector<Connection>& getAllConnections() const {
        return connections_;
    }
    
    void clear() {
        connections_.clear();
    }
    
private:
    std::vector<Connection> connections_;
};

/**
 * @class SidechainCompressorNode
 * @brief Nodo de audio con sidechain support
 */
class SidechainCompressorNode : public Graph::AudioNode {
public:
    SidechainCompressorNode(int nodeId) 
        : AudioNode(nodeId, Graph::NodeType::Effect) {
    }
    
    void prepare(double sampleRate, int maxBlockSize) override {
        compressor_.prepare(sampleRate, maxBlockSize);
        sidechainBuffer_.setSize(2, maxBlockSize);
    }
    
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        if (hasSidechainInput_ && sidechainBuffer_.getNumSamples() > 0) {
            compressor_.process(buffer, &sidechainBuffer_);
        } else {
            compressor_.process(buffer, nullptr);
        }
        
        // Clear sidechain buffer for next iteration
        sidechainBuffer_.clear();
    }
    
    void setSidechainInput(const juce::AudioBuffer<float>& scBuffer) {
        hasSidechainInput_ = true;
        
        int numSamples = std::min(scBuffer.getNumSamples(), sidechainBuffer_.getNumSamples());
        int numChannels = std::min(scBuffer.getNumChannels(), sidechainBuffer_.getNumChannels());
        
        for (int ch = 0; ch < numChannels; ++ch) {
            sidechainBuffer_.copyFrom(ch, 0, scBuffer, ch, 0, numSamples);
        }
    }
    
    void clearSidechainInput() {
        hasSidechainInput_ = false;
        sidechainBuffer_.clear();
    }
    
    void setCompressorParameters(const SidechainCompressor::Parameters& params) {
        compressor_.setParameters(params);
    }
    
    SidechainCompressor& getCompressor() { return compressor_; }
    
    juce::String getName() const override { 
        return "Sidechain Compressor"; 
    }
    
private:
    SidechainCompressor compressor_;
    juce::AudioBuffer<float> sidechainBuffer_;
    bool hasSidechainInput_ { false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainCompressorNode)
};

/**
 * @class DuckingPreset
 * @brief Presets comunes de ducking
 */
class DuckingPreset {
public:
    enum class Type {
        KickBass,        // Kick ducks bass
        KickPad,         // Kick ducks pad/synth
        VocalMusic,      // Vocal ducks music
        RadioVoiceOver,  // Aggressive ducking for VO
        SubtleRhythmic,  // Sutil para ritmo
        Custom
    };
    
    static SidechainCompressor::Parameters getPreset(Type type) {
        SidechainCompressor::Parameters params;
        
        switch (type) {
            case Type::KickBass:
                params.threshold = -24.0f;
                params.ratio = 8.0f;
                params.attackMs = 1.0f;
                params.releaseMs = 100.0f;
                params.knee = 6.0f;
                params.sidechainHPF = 40.0f;
                params.sidechainLPF = 200.0f;  // Solo graves
                params.externalSidechain = true;
                break;
                
            case Type::KickPad:
                params.threshold = -20.0f;
                params.ratio = 6.0f;
                params.attackMs = 5.0f;
                params.releaseMs = 200.0f;
                params.knee = 4.0f;
                params.sidechainHPF = 60.0f;
                params.sidechainLPF = 8000.0f;
                params.externalSidechain = true;
                break;
                
            case Type::VocalMusic:
                params.threshold = -18.0f;
                params.ratio = 4.0f;
                params.attackMs = 10.0f;
                params.releaseMs = 300.0f;
                params.knee = 8.0f;
                params.sidechainHPF = 200.0f;  // Freq de voz
                params.sidechainLPF = 5000.0f;
                params.externalSidechain = true;
                break;
                
            case Type::RadioVoiceOver:
                params.threshold = -30.0f;
                params.ratio = 10.0f;
                params.attackMs = 2.0f;
                params.releaseMs = 50.0f;
                params.knee = 2.0f;
                params.sidechainHPF = 300.0f;
                params.sidechainLPF = 3000.0f;
                params.externalSidechain = true;
                break;
                
            case Type::SubtleRhythmic:
                params.threshold = -12.0f;
                params.ratio = 3.0f;
                params.attackMs = 20.0f;
                params.releaseMs = 400.0f;
                params.knee = 10.0f;
                params.sidechainHPF = 80.0f;
                params.sidechainLPF = 12000.0f;
                params.externalSidechain = true;
                break;
                
            case Type::Custom:
            default:
                // Defaults
                break;
        }
        
        params.autoMakeup = true;
        return params;
    }
    
    static juce::String getPresetName(Type type) {
        switch (type) {
            case Type::KickBass: return "Kick → Bass";
            case Type::KickPad: return "Kick → Pad/Synth";
            case Type::VocalMusic: return "Vocal → Music";
            case Type::RadioVoiceOver: return "Radio Voice Over";
            case Type::SubtleRhythmic: return "Subtle Rhythmic";
            case Type::Custom: return "Custom";
            default: return "Unknown";
        }
    }
};

} // namespace DSP
} // namespace Audio
} // namespace OmegaStudio
