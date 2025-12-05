//==============================================================================
// AudioGraph.cpp
// Implementation of the audio processing graph
//==============================================================================

#include "AudioGraph.h"
#include "AudioNode.h"

namespace Omega::Audio {

//==============================================================================
AudioGraph::AudioGraph() = default;
AudioGraph::~AudioGraph() = default;

//==============================================================================
NodeID AudioGraph::addNode(std::unique_ptr<AudioNode> node) {
    if (!node) {
        return INVALID_NODE_ID;
    }
    
    const NodeID newId = nextNodeId_++;
    nodes_[newId] = std::move(node);
    
    rebuildProcessingOrder();
    return newId;
}

//==============================================================================
bool AudioGraph::removeNode(NodeID nodeId) {
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    
    // Remove all connections involving this node
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
            [nodeId](const auto& conn) {
                // Connection structure to be defined
                return false;  // Placeholder
            }),
        connections_.end()
    );
    
    nodes_.erase(it);
    rebuildProcessingOrder();
    return true;
}

//==============================================================================
AudioNode* AudioGraph::getNode(NodeID nodeId) noexcept {
    auto it = nodes_.find(nodeId);
    return (it != nodes_.end()) ? it->second.get() : nullptr;
}

//==============================================================================
const AudioNode* AudioGraph::getNode(NodeID nodeId) const noexcept {
    auto it = nodes_.find(nodeId);
    return (it != nodes_.end()) ? it->second.get() : nullptr;
}

//==============================================================================
void AudioGraph::process(const float* const* inputs, int numInputs,
                        float* const* outputs, int numOutputs,
                        int numSamples)
{
    // Process nodes in topological order
    for (NodeID nodeId : processingOrder_) {
        auto* node = getNode(nodeId);
        if (node) {
            // node->process(numSamples); // To be implemented
        }
    }
    
    // For now, clear outputs
    for (int ch = 0; ch < numOutputs; ++ch) {
        if (outputs[ch]) {
            juce::FloatVectorOperations::clear(outputs[ch], numSamples);
        }
    }
}

//==============================================================================
void AudioGraph::reset() {
    for (auto& [id, node] : nodes_) {
        // node->reset(); // To be implemented
    }
}

//==============================================================================
void AudioGraph::clear() {
    nodes_.clear();
    connections_.clear();
    processingOrder_.clear();
    nextNodeId_ = 1;
}

//==============================================================================
size_t AudioGraph::getNumNodes() const noexcept {
    return nodes_.size();
}

//==============================================================================
size_t AudioGraph::getNumConnections() const noexcept {
    return connections_.size();
}

//==============================================================================
void AudioGraph::rebuildProcessingOrder() {
    // Placeholder for topological sort
    processingOrder_.clear();
    for (const auto& [id, node] : nodes_) {
        processingOrder_.push_back(id);
    }
}

//==============================================================================
void AudioGraph::updateLatencyCompensation() {
    // Placeholder for PDC calculation
    totalLatency_ = 0;
}

//==============================================================================
int AudioGraph::getTotalLatency() const noexcept {
    return totalLatency_;
}

} // namespace Omega::Audio
