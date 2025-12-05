#include "ChannelRack.h"

namespace OmegaStudio {
namespace Sequencer {

//==============================================================================
// ChannelRackEngine Implementation
//==============================================================================

ChannelRackEngine::ChannelRackEngine() {
    initializePatterns();
}

ChannelRackEngine::~ChannelRackEngine() = default;

void ChannelRackEngine::initializePatterns() {
    // Four-on-the-floor (kick drum)
    stepPatterns_["Four On Floor"] = {
        true, false, false, false,
        true, false, false, false,
        true, false, false, false,
        true, false, false, false
    };
    
    // Backbeat (snare on 2 and 4)
    stepPatterns_["Backbeat"] = {
        false, false, false, false,
        true, false, false, false,
        false, false, false, false,
        true, false, false, false
    };
    
    // Hi-hat pattern
    stepPatterns_["Hi-Hat 8ths"] = {
        true, false, true, false,
        true, false, true, false,
        true, false, true, false,
        true, false, true, false
    };
    
    // Hi-hat 16ths
    stepPatterns_["Hi-Hat 16ths"] = {
        true, true, true, true,
        true, true, true, true,
        true, true, true, true,
        true, true, true, true
    };
    
    // Offbeat
    stepPatterns_["Offbeat"] = {
        false, true, false, true,
        false, true, false, true,
        false, true, false, true,
        false, true, false, true
    };
}

//==============================================================================
// Channel Management
//==============================================================================

int ChannelRackEngine::addChannel(const juce::String& name, Channel::Type type) {
    auto channel = std::make_shared<Channel>();
    channel->id = nextChannelId_++;
    channel->name = name;
    channel->type = type;
    channel->colour = juce::Colour::fromHSV(
        juce::Random::getSystemRandom().nextFloat(),
        0.7f, 0.9f, 1.0f
    );
    
    channels_.push_back(channel);
    return channel->id;
}

void ChannelRackEngine::removeChannel(int channelId) {
    channels_.erase(
        std::remove_if(channels_.begin(), channels_.end(),
            [channelId](const auto& ch) { return ch->id == channelId; }),
        channels_.end()
    );
}

void ChannelRackEngine::moveChannel(int channelId, int newPosition) {
    auto it = std::find_if(channels_.begin(), channels_.end(),
        [channelId](const auto& ch) { return ch->id == channelId; });
    
    if (it != channels_.end() && newPosition >= 0 && newPosition < channels_.size()) {
        auto channel = *it;
        channels_.erase(it);
        channels_.insert(channels_.begin() + newPosition, channel);
    }
}

Channel* ChannelRackEngine::getChannel(int channelId) {
    for (auto& channel : channels_) {
        if (channel->id == channelId) {
            return channel.get();
        }
    }
    return nullptr;
}

//==============================================================================
// Layer/Split Management
//==============================================================================

int ChannelRackEngine::createLayer(const juce::String& name, const std::vector<int>& childIds) {
    int layerId = addChannel(name, Channel::Type::Layer);
    auto* layer = getChannel(layerId);
    
    if (layer) {
        layer->childChannelIds = childIds;
    }
    
    return layerId;
}

int ChannelRackEngine::createSplit(const juce::String& name) {
    return addChannel(name, Channel::Type::Split);
}

void ChannelRackEngine::addToLayer(int layerId, int channelId) {
    auto* layer = getChannel(layerId);
    if (layer && layer->type == Channel::Type::Layer) {
        layer->childChannelIds.push_back(channelId);
    }
}

void ChannelRackEngine::removeFromLayer(int layerId, int channelId) {
    auto* layer = getChannel(layerId);
    if (layer && layer->type == Channel::Type::Layer) {
        layer->childChannelIds.erase(
            std::remove(layer->childChannelIds.begin(), layer->childChannelIds.end(), channelId),
            layer->childChannelIds.end()
        );
    }
}

void ChannelRackEngine::setSplitZone(int channelId, int lowKey, int highKey) {
    auto* channel = getChannel(channelId);
    if (channel) {
        channel->keyZoneLow = juce::jlimit(0, 127, lowKey);
        channel->keyZoneHigh = juce::jlimit(0, 127, highKey);
    }
}

//==============================================================================
// Step Sequencer
//==============================================================================

void ChannelRackEngine::setStep(int channelId, int stepIndex, bool active) {
    auto* channel = getChannel(channelId);
    if (channel && stepIndex >= 0 && stepIndex < channel->steps.size()) {
        channel->steps[stepIndex] = active;
    }
}

void ChannelRackEngine::setStepVelocity(int channelId, int stepIndex, int velocity) {
    auto* channel = getChannel(channelId);
    if (channel && stepIndex >= 0 && stepIndex < channel->velocities.size()) {
        channel->velocities[stepIndex] = juce::jlimit(1, 127, velocity);
    }
}

void ChannelRackEngine::clearSteps(int channelId) {
    auto* channel = getChannel(channelId);
    if (channel) {
        std::fill(channel->steps.begin(), channel->steps.end(), false);
    }
}

void ChannelRackEngine::fillSteps(int channelId) {
    auto* channel = getChannel(channelId);
    if (channel) {
        std::fill(channel->steps.begin(), channel->steps.end(), true);
    }
}

void ChannelRackEngine::randomizeSteps(int channelId, float probability) {
    auto* channel = getChannel(channelId);
    if (channel) {
        for (size_t i = 0; i < channel->steps.size(); ++i) {
            channel->steps[i] = juce::Random::getSystemRandom().nextFloat() < probability;
        }
    }
}

void ChannelRackEngine::loadStepPattern(int channelId, const juce::String& patternName) {
    auto* channel = getChannel(channelId);
    auto it = stepPatterns_.find(patternName);
    
    if (channel && it != stepPatterns_.end()) {
        channel->steps = it->second;
        
        // Resize if needed
        if (channel->steps.size() < stepLength_) {
            channel->steps.resize(stepLength_, false);
        }
    }
}

std::vector<juce::String> ChannelRackEngine::getAvailablePatterns() const {
    std::vector<juce::String> names;
    for (const auto& pair : stepPatterns_) {
        names.push_back(pair.first);
    }
    return names;
}

//==============================================================================
// Playback
//==============================================================================

void ChannelRackEngine::prepareToPlay(double sampleRate, int blockSize) {
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;
}

void ChannelRackEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    // Process MIDI for each channel
    for (auto& channel : channels_) {
        if (channel->isMuted) continue;
        
        // Check if current step is active
        if (currentStep_ < channel->steps.size() && channel->steps[currentStep_]) {
            int velocity = channel->velocities[currentStep_];
            triggerChannelNote(channel.get(), velocity, midi, 0);
        }
    }
}

void ChannelRackEngine::advanceStep() {
    currentStep_ = (currentStep_ + 1) % stepLength_;
}

void ChannelRackEngine::routeMidiToChannel(int channelId, const juce::MidiMessage& message) {
    auto* channel = getChannel(channelId);
    if (!channel) return;
    
    // Check if note is in key zone (for splits)
    if (message.isNoteOn()) {
        int note = message.getNoteNumber();
        if (note < channel->keyZoneLow || note > channel->keyZoneHigh) {
            return;
        }
    }
    
    // Route to layer children
    if (channel->type == Channel::Type::Layer) {
        for (int childId : channel->childChannelIds) {
            routeMidiToChannel(childId, message);
        }
    }
    
    // TODO: Send to instrument/sampler
}

void ChannelRackEngine::triggerChannelNote(Channel* channel, int velocity, 
                                           juce::MidiBuffer& midi, int sampleOffset) {
    if (!channel) return;
    
    // Layer handling
    if (channel->type == Channel::Type::Layer) {
        for (int childId : channel->childChannelIds) {
            auto* child = getChannel(childId);
            if (child) {
                triggerChannelNote(child, velocity, midi, sampleOffset);
            }
        }
        return;
    }
    
    // Create MIDI note
    juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
        channel->midiChannel + 1,
        channel->rootNote,
        static_cast<uint8>(velocity)
    );
    
    midi.addEvent(noteOn, sampleOffset);
    
    // Schedule note off (e.g., 1/16th note later)
    int noteLength = static_cast<int>(sampleRate_ * 0.1);  // 100ms
    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
        channel->midiChannel + 1,
        channel->rootNote
    );
    midi.addEvent(noteOff, sampleOffset + noteLength);
}

//==============================================================================
// Cloning & Presets
//==============================================================================

int ChannelRackEngine::cloneChannel(int sourceId) {
    auto* source = getChannel(sourceId);
    if (!source) return -1;
    
    auto clone = std::make_shared<Channel>(*source);
    clone->id = nextChannelId_++;
    clone->name = source->name + " (clone)";
    
    channels_.push_back(clone);
    return clone->id;
}

void ChannelRackEngine::saveChannelPreset(int channelId, const juce::File& file) {
    auto* channel = getChannel(channelId);
    if (!channel) return;
    
    juce::ValueTree tree = toValueTree();
    auto xml = tree.createXml();
    
    if (xml) {
        xml->writeTo(file);
    }
}

int ChannelRackEngine::loadChannelPreset(const juce::File& file) {
    auto xml = juce::parseXML(file);
    if (!xml) return -1;
    
    juce::ValueTree tree = juce::ValueTree::fromXml(*xml);
    // TODO: Load single channel from tree
    
    return -1;
}

//==============================================================================
// Serialization
//==============================================================================

juce::ValueTree ChannelRackEngine::toValueTree() const {
    juce::ValueTree tree("ChannelRack");
    
    for (const auto& channel : channels_) {
        juce::ValueTree channelTree("Channel");
        channelTree.setProperty("id", channel->id, nullptr);
        channelTree.setProperty("name", channel->name, nullptr);
        channelTree.setProperty("type", static_cast<int>(channel->type), nullptr);
        channelTree.setProperty("colour", channel->colour.toString(), nullptr);
        channelTree.setProperty("mixerTrack", channel->mixerTrack, nullptr);
        channelTree.setProperty("rootNote", channel->rootNote, nullptr);
        channelTree.setProperty("muted", channel->isMuted, nullptr);
        
        // Save steps
        juce::String stepsStr;
        for (bool step : channel->steps) {
            stepsStr += step ? "1" : "0";
        }
        channelTree.setProperty("steps", stepsStr, nullptr);
        
        tree.appendChild(channelTree, nullptr);
    }
    
    return tree;
}

void ChannelRackEngine::fromValueTree(const juce::ValueTree& tree) {
    // TODO: Implement deserialization
}

//==============================================================================
// ChannelRackComponent Implementation
//==============================================================================

ChannelRackComponent::ChannelRackComponent(ChannelRackEngine& engine)
    : engine_(engine) {
    setSize(800, 600);
}

ChannelRackComponent::~ChannelRackComponent() = default;

void ChannelRackComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    drawChannelList(g);
    drawStepGrid(g);
    drawSteps(g);
    drawPlayhead(g);
}

void ChannelRackComponent::resized() {
    // TODO: Layout controls
}

void ChannelRackComponent::mouseDown(const juce::MouseEvent& e) {
    int channelIdx = getChannelAtY(e.y);
    int stepIdx = getStepAtX(e.x);
    
    if (channelIdx >= 0 && stepIdx >= 0) {
        const auto& channels = engine_.getChannels();
        if (channelIdx < channels.size()) {
            int channelId = channels[channelIdx]->id;
            bool currentState = channels[channelIdx]->steps[stepIdx];
            
            engine_.setStep(channelId, stepIdx, !currentState);
            repaint();
        }
    }
}

void ChannelRackComponent::mouseDrag(const juce::MouseEvent& e) {
    mouseDown(e);  // Paint mode
}

void ChannelRackComponent::mouseUp(const juce::MouseEvent& e) {
    // Nothing for now
}

void ChannelRackComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    int channelIdx = getChannelAtY(e.y);
    if (channelIdx >= 0) {
        // TODO: Open channel settings
    }
}

bool ChannelRackComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& details) {
    return details.description.toString().startsWith("sample:") ||
           details.description.toString().startsWith("plugin:");
}

void ChannelRackComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails& details) {
    // TODO: Create channel from dropped sample/plugin
}

void ChannelRackComponent::setStepLength(int steps) {
    engine_.setStepLength(steps);
    repaint();
}

void ChannelRackComponent::drawChannelList(juce::Graphics& g) {
    const auto& channels = engine_.getChannels();
    int y = 0;
    
    for (size_t i = 0; i < channels.size(); ++i) {
        const auto& channel = channels[i];
        
        // Background
        g.setColour(channel->colour.withAlpha(0.1f));
        g.fillRect(0, y, 200, rowHeight_);
        
        // Name
        g.setColour(juce::Colours::white);
        g.drawText(channel->name, 5, y, 190, rowHeight_, juce::Justification::centredLeft);
        
        // Separator
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawHorizontalLine(y + rowHeight_, 0.0f, static_cast<float>(getWidth()));
        
        y += rowHeight_;
    }
}

void ChannelRackComponent::drawStepGrid(juce::Graphics& g) {
    const int channelCount = static_cast<int>(engine_.getChannels().size());
    const int gridX = 200;
    
    g.setColour(juce::Colour(0xff2a2a2a));
    
    // Vertical lines (steps)
    for (int step = 0; step <= 16; ++step) {
        int x = gridX + step * stepSize_;
        
        // Emphasize every 4 steps
        if (step % 4 == 0) {
            g.setColour(juce::Colour(0xff404040));
        } else {
            g.setColour(juce::Colour(0xff2a2a2a));
        }
        
        g.drawVerticalLine(x, 0.0f, static_cast<float>(channelCount * rowHeight_));
    }
    
    // Horizontal lines (channels)
    g.setColour(juce::Colour(0xff2a2a2a));
    for (int i = 0; i <= channelCount; ++i) {
        int y = i * rowHeight_;
        g.drawHorizontalLine(y, static_cast<float>(gridX), 
                            static_cast<float>(gridX + 16 * stepSize_));
    }
}

void ChannelRackComponent::drawSteps(juce::Graphics& g) {
    const auto& channels = engine_.getChannels();
    const int gridX = 200;
    int y = 0;
    
    for (const auto& channel : channels) {
        for (size_t step = 0; step < channel->steps.size() && step < 16; ++step) {
            if (channel->steps[step]) {
                int x = gridX + static_cast<int>(step) * stepSize_;
                
                // Draw active step
                g.setColour(channel->colour);
                g.fillRoundedRectangle(
                    static_cast<float>(x + 2),
                    static_cast<float>(y + 5),
                    static_cast<float>(stepSize_ - 4),
                    static_cast<float>(rowHeight_ - 10),
                    3.0f
                );
                
                // Draw velocity bar if enabled
                if (showVelocity_) {
                    float velHeight = (channel->velocities[step] / 127.0f) * (rowHeight_ - 10);
                    g.setColour(juce::Colours::white.withAlpha(0.3f));
                    g.fillRect(
                        static_cast<float>(x + 2),
                        static_cast<float>(y + 5 + (rowHeight_ - 10 - velHeight)),
                        static_cast<float>(stepSize_ - 4),
                        velHeight
                    );
                }
            }
        }
        y += rowHeight_;
    }
}

void ChannelRackComponent::drawPlayhead(juce::Graphics& g) {
    const int gridX = 200;
    int currentStep = engine_.getCurrentStep();
    int x = gridX + currentStep * stepSize_ + stepSize_ / 2;
    
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(x, 0.0f, static_cast<float>(engine_.getChannels().size() * rowHeight_));
}

int ChannelRackComponent::getChannelAtY(int y) {
    return y / rowHeight_;
}

int ChannelRackComponent::getStepAtX(int x) {
    const int gridX = 200;
    if (x < gridX) return -1;
    return (x - gridX) / stepSize_;
}

void ChannelRackComponent::showChannelMenu(int channelId) {
    juce::PopupMenu menu;
    menu.addItem(1, "Clone Channel");
    menu.addItem(2, "Delete Channel");
    menu.addSeparator();
    menu.addItem(3, "Clear Steps");
    menu.addItem(4, "Fill Steps");
    menu.addItem(5, "Randomize Steps");
    
    // TODO: Handle menu result
}

void ChannelRackComponent::showStepMenu(int channelId, int stepIndex) {
    juce::PopupMenu menu;
    menu.addItem(1, "Set Velocity...");
    menu.addItem(2, "Clear Step");
    
    // TODO: Handle menu result
}

//==============================================================================
// StepButton Implementation
//==============================================================================

StepButton::StepButton(int channelId, int stepIndex)
    : channelId_(channelId), stepIndex_(stepIndex) {
    setSize(40, 40);
}

void StepButton::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().reduced(2).toFloat();
    
    if (isActive_) {
        g.setColour(juce::Colours::orange);
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Velocity indicator
        float velHeight = (velocity_ / 127.0f) * bounds.getHeight();
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRect(bounds.getX(), bounds.getBottom() - velHeight, 
                  bounds.getWidth(), velHeight);
    } else if (isHovered_) {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(bounds, 3.0f);
    }
    
    if (isAccent_) {
        g.setColour(juce::Colours::red);
        g.drawRoundedRectangle(bounds, 3.0f, 2.0f);
    }
}

void StepButton::mouseDown(const juce::MouseEvent& e) {
    setActive(!isActive_);
    
    if (onStepChanged) {
        onStepChanged(channelId_, stepIndex_, isActive_);
    }
}

void StepButton::setActive(bool active) {
    isActive_ = active;
    repaint();
}

void StepButton::setVelocity(int velocity) {
    velocity_ = juce::jlimit(1, 127, velocity);
    repaint();
}

void StepButton::setAccent(bool accent) {
    isAccent_ = accent;
    repaint();
}

//==============================================================================
// ChannelStripComponent Implementation
//==============================================================================

ChannelStripComponent::ChannelStripComponent(Channel& channel)
    : channel_(channel) {
    
    nameLabel_ = std::make_unique<juce::Label>();
    nameLabel_->setText(channel_.name, juce::dontSendNotification);
    addAndMakeVisible(nameLabel_.get());
    
    volumeSlider_ = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, 
                                                     juce::Slider::NoTextBox);
    addAndMakeVisible(volumeSlider_.get());
    
    muteButton_ = std::make_unique<juce::TextButton>("M");
    addAndMakeVisible(muteButton_.get());
    
    soloButton_ = std::make_unique<juce::TextButton>("S");
    addAndMakeVisible(soloButton_.get());
}

void ChannelStripComponent::paint(juce::Graphics& g) {
    g.fillAll(channel_.colour.withAlpha(0.1f));
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRect(getLocalBounds());
}

void ChannelStripComponent::resized() {
    auto bounds = getLocalBounds();
    
    nameLabel_->setBounds(bounds.removeFromTop(30));
    muteButton_->setBounds(bounds.removeFromBottom(25).removeFromLeft(getWidth() / 2));
    soloButton_->setBounds(bounds.removeFromBottom(25));
    volumeSlider_->setBounds(bounds);
}

} // namespace Sequencer
} // namespace OmegaStudio
