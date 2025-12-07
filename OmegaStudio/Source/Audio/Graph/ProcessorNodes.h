//==============================================================================
// ProcessorNodes.h
// Core graph node implementations (scaffolding, RT-safe friendly)
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "AudioNode.h"
#include "../Plugins/PluginManager.h"
#include "../../Mixer/MixerEngine.h"

namespace Omega::Audio {

//==============================================================================
// InputNode - entry point for hardware inputs
//==============================================================================
class InputNode : public AudioNode {
public:
	InputNode(int numChannels)
		: AudioNode(NodeType::Instrument, "Input"), numChannels_(numChannels) {}

	void prepare(double sampleRate, int maxBlockSize) override;
	void process(juce::AudioBuffer<float>& buffer) override;
	void reset() override;

	// Called per callback to point to the live input buffers (no ownership)
	void setExternalInput(const float* const* input, int numChannels, int numSamples) noexcept;

private:
	int numChannels_ { 0 };
	const float* const* externalInput_ { nullptr };
	int externalSamples_ { 0 };
};

//==============================================================================
// OutputNode - terminal node feeding hardware outputs
//==============================================================================
class OutputNode : public AudioNode {
public:
	OutputNode(int numChannels)
		: AudioNode(NodeType::Master, "Output"), numChannels_(numChannels) {}

	void prepare(double sampleRate, int maxBlockSize) override;
	void process(juce::AudioBuffer<float>& buffer) override;
	void reset() override;

	// Output buffer owned by callback; we just copy into it
	void setExternalOutput(float* const* output, int numChannels, int numSamples) noexcept;

private:
	int numChannels_ { 0 };
	float* const* externalOutput_ { nullptr };
	int externalSamples_ { 0 };
};

//==============================================================================
// PluginNode - wraps a PluginChain for effects/instruments
//==============================================================================
class PluginNode : public AudioNode {
public:
	PluginNode()
		: AudioNode(NodeType::Effect, "PluginChain") {}

	void prepare(double sampleRate, int maxBlockSize) override;
	void process(juce::AudioBuffer<float>& buffer) override;
	void reset() override;

	OmegaStudio::PluginChain& chain() noexcept { return pluginChain_; }
	int getLatencySamples() const noexcept override;

	void setMidiBuffer(juce::MidiBuffer* midi) noexcept { midi_ = midi; }

private:
	OmegaStudio::PluginChain pluginChain_;
	juce::MidiBuffer* midi_ { nullptr }; // non-owning
};

//==============================================================================
// MixerNode - bridges to MixerEngine (channel/bus processing)
//==============================================================================
class MixerNode : public AudioNode {
public:
	MixerNode(OmegaStudio::MixerEngine& mixer)
		: AudioNode(NodeType::Mixer, "Mixer"), mixer_(mixer) {}

	void prepare(double sampleRate, int maxBlockSize) override;
	void process(juce::AudioBuffer<float>& buffer) override;
	void reset() override;

	int getLatencySamples() const noexcept override { return 0; }

	// MIDI/messages for channels
	void setMidiBuffers(std::vector<juce::MidiBuffer*>* midi) noexcept { midiBuffers_ = midi; }
	void setChannelBuffers(std::vector<juce::AudioBuffer<float>*>* channelBuffers) noexcept { channelBuffers_ = channelBuffers; }

	juce::AudioBuffer<float>& masterBuffer() noexcept { return masterBuffer_; }

private:
	OmegaStudio::MixerEngine& mixer_;
	std::vector<juce::MidiBuffer*>* midiBuffers_ { nullptr }; // per channel midi (non-owning)
	std::vector<juce::AudioBuffer<float>*>* channelBuffers_ { nullptr }; // per channel audio (non-owning)
	juce::AudioBuffer<float> masterBuffer_;
	juce::MidiBuffer emptyMidi_;
	double sampleRate_ { 48000.0 };
	int blockSize_ { 512 };
};

} // namespace Omega::Audio
