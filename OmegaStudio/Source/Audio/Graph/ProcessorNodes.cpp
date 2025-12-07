//==============================================================================
// ProcessorNodes.cpp
//==============================================================================

#include "ProcessorNodes.h"

namespace Omega::Audio {

//==============================================================================
// InputNode
//==============================================================================
void InputNode::prepare(double sampleRate, int maxBlockSize) {
	juce::ignoreUnused(sampleRate, maxBlockSize);
	externalInput_ = nullptr;
	externalSamples_ = 0;
}

void InputNode::setExternalInput(const float* const* input, int numChannels, int numSamples) noexcept {
	externalInput_ = input;
	numChannels_ = numChannels;
	externalSamples_ = numSamples;
}

void InputNode::process(juce::AudioBuffer<float>& buffer) {
	buffer.clear();
	if (!externalInput_ || numChannels_ <= 0 || externalSamples_ <= 0) {
		return;
	}

	const int copyChannels = juce::jmin(numChannels_, buffer.getNumChannels());
	const int copySamples = juce::jmin(externalSamples_, buffer.getNumSamples());
	for (int ch = 0; ch < copyChannels; ++ch) {
		buffer.copyFrom(ch, 0, externalInput_[ch], copySamples);
	}
}

void InputNode::reset() {
	externalInput_ = nullptr;
	externalSamples_ = 0;
}

//==============================================================================
// OutputNode
//==============================================================================
void OutputNode::prepare(double sampleRate, int maxBlockSize) {
	juce::ignoreUnused(sampleRate, maxBlockSize);
	externalOutput_ = nullptr;
	externalSamples_ = 0;
}

void OutputNode::setExternalOutput(float* const* output, int numChannels, int numSamples) noexcept {
	externalOutput_ = output;
	numChannels_ = numChannels;
	externalSamples_ = numSamples;
}

void OutputNode::process(juce::AudioBuffer<float>& buffer) {
	if (!externalOutput_ || numChannels_ <= 0 || externalSamples_ <= 0) {
		buffer.clear();
		return;
	}

	const int copyChannels = juce::jmin(numChannels_, buffer.getNumChannels());
	const int copySamples = juce::jmin(externalSamples_, buffer.getNumSamples());
	for (int ch = 0; ch < copyChannels; ++ch) {
		if (externalOutput_[ch]) {
			juce::FloatVectorOperations::copy(externalOutput_[ch], buffer.getReadPointer(ch), copySamples);
		}
	}
}

void OutputNode::reset() {
	externalOutput_ = nullptr;
	externalSamples_ = 0;
}

//==============================================================================
// PluginNode
//==============================================================================
void PluginNode::prepare(double sampleRate, int maxBlockSize) {
	pluginChain_.prepareToPlay(sampleRate, maxBlockSize);
}

void PluginNode::process(juce::AudioBuffer<float>& buffer) {
	static juce::MidiBuffer emptyMidi;
	juce::MidiBuffer& midi = midi_ ? *midi_ : emptyMidi;
	pluginChain_.process(buffer, midi);
}

void PluginNode::reset() {
	pluginChain_.releaseResources();
}

int PluginNode::getLatencySamples() const noexcept {
	return pluginChain_.getTotalLatency();
}

//==============================================================================
// MixerNode
//==============================================================================
void MixerNode::prepare(double sampleRate, int maxBlockSize) {
	sampleRate_ = sampleRate;
	blockSize_ = maxBlockSize;
	masterBuffer_.setSize(2, blockSize_);
	mixer_.prepareToPlay(sampleRate_, blockSize_);
}

void MixerNode::process(juce::AudioBuffer<float>& buffer) {
	// Ensure master buffer matches block size
	masterBuffer_.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
	masterBuffer_.clear();

	// If channel buffers provided, feed first buffer with incoming audio as default
	std::vector<juce::AudioBuffer<float>*> channelBufs;
	std::vector<juce::MidiBuffer*> midiBufs;

	if (channelBuffers_ && !channelBuffers_->empty()) {
		channelBufs = *channelBuffers_;
		auto* ch0 = channelBufs[0];
		if (ch0) {
			ch0->setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
			for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), ch0->getNumChannels()); ++ch) {
				ch0->copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
			}
		}
	} else {
		// Fallback: use incoming buffer as single channel input
		channelBufs.push_back(&buffer);
	}

	if (midiBuffers_ && !midiBuffers_->empty()) {
		midiBufs = *midiBuffers_;
	} else {
		midiBufs.push_back(&emptyMidi_);
	}

	mixer_.process(channelBufs, midiBufs, masterBuffer_);

	// Copy master to provided buffer
	const int copyChannels = juce::jmin(buffer.getNumChannels(), masterBuffer_.getNumChannels());
	const int copySamples = juce::jmin(buffer.getNumSamples(), masterBuffer_.getNumSamples());
	for (int ch = 0; ch < copyChannels; ++ch) {
		buffer.copyFrom(ch, 0, masterBuffer_, ch, 0, copySamples);
	}
}

void MixerNode::reset() {
	mixer_.releaseResources();
	masterBuffer_.clear();
}

} // namespace Omega::Audio
