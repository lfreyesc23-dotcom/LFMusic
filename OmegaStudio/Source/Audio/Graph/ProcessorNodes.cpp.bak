/**
 * @file ProcessorNodes.cpp
 * @brief Implementation of processor nodes
 */

#include "ProcessorNodes.h"

namespace omega {

// ============================================================================
// PitchCorrectionNode Implementation
// ============================================================================

PitchCorrectionNode::PitchCorrectionNode()
    : AudioNode("PitchCorrection") {
    m_processor = std::make_unique<PitchCorrection>();
}

void PitchCorrectionNode::prepare(double sampleRate, int maxBlockSize) {
    m_processor->initialize(sampleRate, maxBlockSize);
}

void PitchCorrectionNode::process(float** buffers, int numChannels, int numSamples) {
    if (numChannels == 0 || !buffers) {
        return;
    }

    // Process first channel
    m_processor->process(buffers[0], numSamples);

    // If stereo, process second channel
    if (numChannels > 1) {
        m_processor->processStereo(buffers[0], buffers[1], numSamples);
    }
}

void PitchCorrectionNode::reset() {
    m_processor->reset();
}

// ============================================================================
// VocalEnhancerNode Implementation
// ============================================================================

VocalEnhancerNode::VocalEnhancerNode()
    : AudioNode("VocalEnhancer") {
    m_processor = std::make_unique<VocalEnhancer>();
}

void VocalEnhancerNode::prepare(double sampleRate, int maxBlockSize) {
    m_processor->initialize(sampleRate, maxBlockSize);
}

void VocalEnhancerNode::process(float** buffers, int numChannels, int numSamples) {
    if (numChannels == 0 || !buffers) {
        return;
    }

    // Process first channel
    m_processor->process(buffers[0], numSamples);

    // If stereo, process second channel
    if (numChannels > 1) {
        m_processor->processStereo(buffers[0], buffers[1], numSamples);
    }
}

void VocalEnhancerNode::reset() {
    m_processor->reset();
}

// ============================================================================
// RecorderNode Implementation
// ============================================================================

RecorderNode::RecorderNode()
    : AudioNode("Recorder") {
    m_recorder = std::make_unique<AudioRecorder>();
}

void RecorderNode::prepare(double sampleRate, int maxBlockSize) {
    m_recorder->initialize(sampleRate);
}

void RecorderNode::process(float** buffers, int numChannels, int numSamples) {
    if (!buffers || numChannels == 0) {
        return;
    }

    // Pass audio to recorder
    m_recorder->processAudio(buffers, numChannels, numSamples);

    // Pass through audio unchanged (recorder doesn't modify signal)
    // Audio is just captured for recording
}

void RecorderNode::reset() {
    m_recorder->stopRecording();
    m_recorder->clearAllTracks();
}

// ============================================================================
// SamplePlayerNode Implementation
// ============================================================================

SamplePlayerNode::SamplePlayerNode()
    : AudioNode("SamplePlayer") {
}

void SamplePlayerNode::prepare(double sampleRate, int maxBlockSize) {
    // Reset playback state
    reset();
}

void SamplePlayerNode::process(float** buffers, int numChannels, int numSamples) {
    if (!m_playing || m_buffer.getNumSamples() == 0) {
        // Fill with silence
        for (int ch = 0; ch < numChannels; ++ch) {
            if (buffers[ch]) {
                std::fill(buffers[ch], buffers[ch] + numSamples, 0.0f);
            }
        }
        return;
    }

    int bufferChannels = m_buffer.getNumChannels();
    int bufferSamples = m_buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        if (m_playPosition >= bufferSamples) {
            if (m_loop) {
                m_playPosition = 0;
            } else {
                m_playing = false;
                // Fill remainder with silence
                for (int ch = 0; ch < numChannels; ++ch) {
                    if (buffers[ch]) {
                        std::fill(buffers[ch] + i, buffers[ch] + numSamples, 0.0f);
                    }
                }
                return;
            }
        }

        // Copy sample data with gain
        for (int ch = 0; ch < numChannels; ++ch) {
            if (buffers[ch]) {
                int sourceChannel = std::min(ch, bufferChannels - 1);
                buffers[ch][i] = m_buffer.getSample(sourceChannel, m_playPosition) * m_gain;
            }
        }

        m_playPosition++;
    }
}

void SamplePlayerNode::reset() {
    m_playPosition = 0;
    m_playing = false;
}

void SamplePlayerNode::loadSample(const juce::String& uuid) {
    // This would integrate with SampleManager to load the sample
    // For now, just reset state
    reset();
    
    // TODO: Get sample from SampleManager and copy to m_buffer
    // auto sample = sampleManager->getSample(uuid);
    // if (sample && sample->isLoaded()) {
    //     m_buffer = *sample->getBuffer();
    // }
}

void SamplePlayerNode::play() {
    if (m_buffer.getNumSamples() > 0) {
        m_playPosition = 0;
        m_playing = true;
    }
}

void SamplePlayerNode::stop() {
    m_playing = false;
    m_playPosition = 0;
}

} // namespace omega
