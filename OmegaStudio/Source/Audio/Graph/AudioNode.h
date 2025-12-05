//==============================================================================
// AudioNode.h
// Base class for all audio processing nodes (instruments, effects, etc.)
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <string>
#include <vector>

namespace Omega::Audio {

//==============================================================================
// Node Type
//==============================================================================
enum class NodeType {
    Unknown,
    Instrument,
    Effect,
    Mixer,
    Send,
    Return,
    Master
};

//==============================================================================
// AudioNode - Base class for all processing nodes
//==============================================================================
class AudioNode {
public:
    explicit AudioNode(NodeType type, const std::string& name = "Node");
    virtual ~AudioNode() = default;
    
    // Non-copyable
    AudioNode(const AudioNode&) = delete;
    AudioNode& operator=(const AudioNode&) = delete;
    
    //==========================================================================
    // Processing (must be implemented by derived classes)
    //==========================================================================
    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    
    //==========================================================================
    // Node Info
    //==========================================================================
    [[nodiscard]] NodeType getType() const noexcept { return type_; }
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    //==========================================================================
    // Latency
    //==========================================================================
    [[nodiscard]] virtual int getLatencySamples() const noexcept { return 0; }
    
    //==========================================================================
    // Bypass
    //==========================================================================
    void setBypassed(bool shouldBeBypassed) noexcept { bypassed_ = shouldBeBypassed; }
    [[nodiscard]] bool isBypassed() const noexcept { return bypassed_; }
    
protected:
    NodeType type_;
    std::string name_;
    bool bypassed_{false};
};

} // namespace Omega::Audio
