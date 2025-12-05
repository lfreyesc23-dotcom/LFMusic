//==============================================================================
// AudioNode.cpp
//==============================================================================

#include "AudioNode.h"

namespace Omega::Audio {

AudioNode::AudioNode(NodeType type, const std::string& name)
    : type_(type), name_(name)
{
}

} // namespace Omega::Audio
