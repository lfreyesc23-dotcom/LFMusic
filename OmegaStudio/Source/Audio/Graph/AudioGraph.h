//==============================================================================
// AudioGraph.h
// Node-based audio processing graph (like FL Studio's mixer routing)
//
// ARCHITECTURE:
// - Nodes represent instruments, effects, or utility processors
// - Edges represent audio connections
// - Topological sorting ensures correct processing order
// - Automatic latency compensation (PDC)
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../Engine/AudioEngine.h"

namespace Omega::Audio {

//==============================================================================
// Node ID type
//==============================================================================
using NodeID = uint32_t;
constexpr NodeID INVALID_NODE_ID = 0;

//==============================================================================
// Forward declarations
//==============================================================================
class AudioNode;

//==============================================================================
// AudioConnection - Represents a connection between two nodes
//==============================================================================
struct AudioConnection {
    NodeID sourceNodeId;
    int sourceChannel;
    NodeID destNodeId;
    int destChannel;
    float gain = 1.0f;
    
    AudioConnection(NodeID src, int srcCh, NodeID dst, int dstCh)
        : sourceNodeId(src), sourceChannel(srcCh), destNodeId(dst), destChannel(dstCh) {}
};

//==============================================================================
// AudioGraph - Manages the audio processing graph
//==============================================================================
class AudioGraph {
public:
    AudioGraph();
    ~AudioGraph();
    
    // Non-copyable
    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;
    
    //==========================================================================
    // Node Management
    //==========================================================================
    [[nodiscard]] NodeID addNode(std::unique_ptr<AudioNode> node);
    [[nodiscard]] bool removeNode(NodeID nodeId);
    [[nodiscard]] AudioNode* getNode(NodeID nodeId) noexcept;
    [[nodiscard]] const AudioNode* getNode(NodeID nodeId) const noexcept;
    
    //==========================================================================
    // Connection Management
    //==========================================================================
    [[nodiscard]] bool connect(NodeID sourceId, int sourceChannel,
                               NodeID destId, int destChannel);
    [[nodiscard]] bool disconnect(NodeID sourceId, int sourceChannel,
                                  NodeID destId, int destChannel);
    [[nodiscard]] bool isConnected(NodeID sourceId, NodeID destId) const;
    
    //==========================================================================
    // Graph Processing (called from audio callback)
    //==========================================================================
    void process(const float* const* inputs, int numInputs,
                float* const* outputs, int numOutputs,
                int numSamples);
    
    //==========================================================================
    // Graph State
    //==========================================================================
    void reset();
    void clear();
    [[nodiscard]] size_t getNumNodes() const noexcept;
    [[nodiscard]] size_t getNumConnections() const noexcept;
    
    //==========================================================================
    // Latency Compensation (PDC - Plugin Delay Compensation)
    //==========================================================================
    void updateLatencyCompensation();
    [[nodiscard]] int getTotalLatency() const noexcept;
    
private:
    //==========================================================================
    // Internal State
    //==========================================================================
    std::unordered_map<NodeID, std::unique_ptr<AudioNode>> nodes_;
    std::vector<std::unique_ptr<AudioConnection>> connections_;
    std::vector<NodeID> processingOrder_;  // Topologically sorted
    
    NodeID nextNodeId_{1};
    int totalLatency_{0};
    
    //==========================================================================
    // Internal Methods
    //==========================================================================
    void rebuildProcessingOrder();
    [[nodiscard]] bool detectCycle(NodeID startNode) const;
};

} // namespace Omega::Audio
