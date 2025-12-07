//==============================================================================
// AudioGraph.cpp
// Implementation of the audio processing graph
//==============================================================================

#include "AudioGraph.h"
#include "AudioNode.h"
#include "ProcessorNodes.h"

#include <algorithm>
#include <functional>

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
    adjacency_[newId] = {};
    
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
                return conn->sourceNodeId == nodeId || conn->destNodeId == nodeId;
            }),
        connections_.end()
    );

    adjacency_.erase(nodeId);
    for (auto& [id, edges] : adjacency_) {
        edges.erase(
            std::remove_if(edges.begin(), edges.end(),
                [nodeId](const AudioConnection& c) { return c.destNodeId == nodeId; }),
            edges.end());
    }
    
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
bool AudioGraph::connect(NodeID sourceId, int sourceChannel,
                         NodeID destId, int destChannel) {
    if (!hasNode(sourceId) || !hasNode(destId)) {
        return false;
    }

    if (connectionExists(sourceId, sourceChannel, destId, destChannel)) {
        return true; // already connected
    }

    auto connection = std::make_unique<AudioConnection>(sourceId, sourceChannel, destId, destChannel);
    connections_.push_back(std::move(connection));
    adjacency_[sourceId].push_back(*connections_.back());

    if (detectCycle(sourceId)) {
        // Revert connection on cycle
        connections_.pop_back();
        auto& edges = adjacency_[sourceId];
        edges.pop_back();
        return false;
    }

    rebuildProcessingOrder();
    return true;
}

//==============================================================================
bool AudioGraph::disconnect(NodeID sourceId, int sourceChannel,
                             NodeID destId, int destChannel) {
    bool removed = false;
    connections_.erase(
        std::remove_if(connections_.begin(), connections_.end(),
            [&](const auto& conn) {
                const bool match = conn->sourceNodeId == sourceId &&
                                   conn->sourceChannel == sourceChannel &&
                                   conn->destNodeId == destId &&
                                   conn->destChannel == destChannel;
                removed |= match;
                return match;
            }),
        connections_.end());

    auto& edges = adjacency_[sourceId];
    edges.erase(
        std::remove_if(edges.begin(), edges.end(),
            [&](const AudioConnection& c) {
                return c.sourceNodeId == sourceId && c.sourceChannel == sourceChannel &&
                       c.destNodeId == destId && c.destChannel == destChannel;
            }),
        edges.end());

    if (removed) {
        rebuildProcessingOrder();
    }
    return removed;
}

//==============================================================================
bool AudioGraph::isConnected(NodeID sourceId, NodeID destId) const {
    return std::any_of(connections_.begin(), connections_.end(),
        [&](const auto& conn) {
            return conn->sourceNodeId == sourceId && conn->destNodeId == destId;
        });
}

//==============================================================================
void AudioGraph::process(const float* const* inputs, int numInputs,
                        float* const* outputs, int numOutputs,
                        int numSamples)
{
    const int channels = juce::jmax(numInputs, numOutputs);
    ensureScratchBuffer(channels, numSamples);

    // Preload input buffer for InputNode if present
    if (inputNodeId_ != INVALID_NODE_ID) {
        auto* inputNode = dynamic_cast<InputNode*>(getNode(inputNodeId_));
        if (inputNode) {
            auto& buf = ensureNodeBuffer(inputNodeId_, channels, numSamples);
            buf.clear();
            if (inputs && numInputs > 0) {
                const int copyCh = juce::jmin(numInputs, buf.getNumChannels());
                for (int ch = 0; ch < copyCh; ++ch) {
                    buf.copyFrom(ch, 0, inputs[ch], numSamples);
                }
            }
        }
    }

    // Process nodes in topological order
    for (NodeID nodeId : processingOrder_) {
        auto* node = getNode(nodeId);
        if (!node) continue;

        auto& nodeBuffer = ensureNodeBuffer(nodeId, channels, numSamples);

        // For InputNode buffer already filled; others start cleared
        if (!dynamic_cast<InputNode*>(node)) {
            nodeBuffer.clear();
        }

        // Special handling: OutputNode will copy to external buffers
        node->process(nodeBuffer);

        // Apply node latency (basic per-block delay)
        const int nodeLatency = node->getLatencySamples();
        if (nodeLatency > 0) {
            applyLatency(nodeId, nodeBuffer, nodeLatency);
        }

        // Route to downstream nodes
        auto adjIt = adjacency_.find(nodeId);
        if (adjIt != adjacency_.end()) {
            for (const auto& edge : adjIt->second) {
                auto& destBuffer = ensureNodeBuffer(edge.destNodeId, channels, numSamples);
                const int copyChannels = juce::jmin(nodeBuffer.getNumChannels(), destBuffer.getNumChannels());
                for (int ch = 0; ch < copyChannels; ++ch) {
                    destBuffer.addFrom(ch, 0, nodeBuffer, ch, 0, numSamples, edge.gain);
                }
            }
        }
    }

    // Fallback: if no nodes, clear outputs
    if (processingOrder_.empty()) {
        for (int ch = 0; ch < numOutputs; ++ch) {
            if (outputs[ch]) {
                juce::FloatVectorOperations::clear(outputs[ch], numSamples);
            }
        }
    }
}

//==============================================================================
void AudioGraph::reset() {
    for (auto& [id, node] : nodes_) {
        // node->reset(); // To be implemented
    }
    for (auto& [id, buf] : nodeBuffers_) {
        juce::ignoreUnused(id);
        buf.clear();
    }
}

//==============================================================================
void AudioGraph::clear() {
    nodes_.clear();
    connections_.clear();
    adjacency_.clear();
    processingOrder_.clear();
    nodeBuffers_.clear();
    delayLines_.clear();
    delayIndices_.clear();
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
    processingOrder_.clear();

    // Kahn's algorithm for topological sort
    std::unordered_map<NodeID, int> inDegree;
    for (const auto& [id, node] : nodes_) {
        juce::ignoreUnused(node);
        inDegree[id] = 0;
    }

    for (const auto& conn : connections_) {
        ++inDegree[conn->destNodeId];
    }

    std::queue<NodeID> q;
    for (const auto& [id, deg] : inDegree) {
        if (deg == 0) {
            q.push(id);
        }
    }

    while (!q.empty()) {
        NodeID n = q.front();
        q.pop();
        processingOrder_.push_back(n);

        for (const auto& edge : adjacency_[n]) {
            auto it = inDegree.find(edge.destNodeId);
            if (it != inDegree.end() && --(it->second) == 0) {
                q.push(edge.destNodeId);
            }
        }
    }

    // If cycle, fallback to insertion order (already added) to avoid empty order
    if (processingOrder_.size() != nodes_.size()) {
        processingOrder_.clear();
        for (const auto& [id, node] : nodes_) {
            juce::ignoreUnused(node);
            processingOrder_.push_back(id);
        }
    }
}

//==============================================================================
void AudioGraph::updateLatencyCompensation() {
    totalLatency_ = 0;
    // Longest-path latency accumulation across the DAG
    std::unordered_map<NodeID, int> latencyByNode;
    for (NodeID id : processingOrder_) {
        const auto* node = getNode(id);
        const int nodeLatency = node ? node->getLatencySamples() : 0;
        int current = nodeLatency;

        // Check predecessors to accumulate longest path to this node
        for (const auto& [predId, edges] : adjacency_) {
            for (const auto& e : edges) {
                if (e.destNodeId == id) {
                    current = std::max(current, nodeLatency + latencyByNode[predId]);
                }
            }
        }

        latencyByNode[id] = current;
        totalLatency_ = std::max(totalLatency_, current);
    }
}

//==============================================================================
int AudioGraph::getTotalLatency() const noexcept {
    return totalLatency_;
}

//==============================================================================
bool AudioGraph::detectCycle(NodeID startNode) const {
    std::unordered_map<NodeID, int> visitState; // 0=unvisited,1=visiting,2=visited

    std::function<bool(NodeID)> dfs = [&](NodeID n) -> bool {
        auto stateIt = visitState.find(n);
        if (stateIt != visitState.end()) {
            if (stateIt->second == 1) return true;   // back-edge => cycle
            if (stateIt->second == 2) return false;  // already processed
        }

        visitState[n] = 1;
        auto adjIt = adjacency_.find(n);
        if (adjIt != adjacency_.end()) {
            for (const auto& edge : adjIt->second) {
                if (dfs(edge.destNodeId)) {
                    return true;
                }
            }
        }
        visitState[n] = 2;
        return false;
    };

    return dfs(startNode);
}

//==============================================================================
bool AudioGraph::hasNode(NodeID nodeId) const noexcept {
    return nodes_.find(nodeId) != nodes_.end();
}

//==============================================================================
bool AudioGraph::connectionExists(NodeID sourceId, int sourceChannel,
                                  NodeID destId, int destChannel) const {
    return std::any_of(connections_.begin(), connections_.end(),
        [&](const auto& conn) {
            return conn->sourceNodeId == sourceId && conn->sourceChannel == sourceChannel &&
                   conn->destNodeId == destId && conn->destChannel == destChannel;
        });
}

//==============================================================================
void AudioGraph::ensureScratchBuffer(int channels, int numSamples) {
    if (channels <= 0 || numSamples <= 0) {
        scratchBuffer_.setSize(0, 0, false, false, true);
        scratchChannels_ = 0;
        scratchBlockSize_ = 0;
        return;
    }

    if (channels != scratchChannels_ || numSamples > scratchBlockSize_) {
        scratchChannels_ = channels;
        scratchBlockSize_ = numSamples;
        scratchBuffer_.setSize(scratchChannels_, scratchBlockSize_, false, false, true);
    }

    scratchBuffer_.clear();
}

//==============================================================================
juce::AudioBuffer<float>& AudioGraph::ensureNodeBuffer(NodeID id, int channels, int numSamples) {
    auto& buf = nodeBuffers_[id];
    if (buf.getNumChannels() != channels || buf.getNumSamples() < numSamples) {
        buf.setSize(channels, numSamples, false, false, true);
    }
    return buf;
}

//==============================================================================
void AudioGraph::applyLatency(NodeID id, juce::AudioBuffer<float>& buffer, int latencySamples) {
    if (latencySamples <= 0) return;
    auto& dl = delayLines_[id];
    auto& idx = delayIndices_[id];

    const int channels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Ensure delay line size: latency per channel
    const size_t needed = static_cast<size_t>(latencySamples * channels);
    if (dl.size() != needed) {
        dl.assign(needed, 0.0f);
        idx = 0;
    }

    // Simple circular buffer per channel interleaved
    for (int sample = 0; sample < numSamples; ++sample) {
        for (int ch = 0; ch < channels; ++ch) {
            const size_t writePos = (idx + static_cast<size_t>(ch)) % dl.size();
            float& delaySlot = dl[writePos];
            const float current = buffer.getSample(ch, sample);
            buffer.setSample(ch, sample, delaySlot);
            delaySlot = current;
        }
        idx = (idx + static_cast<size_t>(channels)) % dl.size();
    }
}

} // namespace Omega::Audio
