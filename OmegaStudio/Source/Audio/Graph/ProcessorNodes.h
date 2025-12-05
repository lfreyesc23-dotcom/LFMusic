/**
 * @file ProcessorNodes.h
 * @brief AudioGraph nodes for new processors
 * 
 * Integrates PitchCorrection, VocalEnhancer, AudioRecorder into AudioGraph
 */

#pragma once

#include "../Graph/AudioNode.h"
#include "../DSP/PitchCorrection.h"
#include "../AI/VocalEnhancer.h"
#include "../Recording/AudioRecorder.h"
#include <memory>

namespace omega {

/**
 * @class PitchCorrectionNode
 * @brief AudioGraph node for auto-tune processing
 */
class PitchCorrectionNode : public AudioNode {
public:
    PitchCorrectionNode();
    ~PitchCorrectionNode() override = default;

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** buffers, int numChannels, int numSamples) override;
    void reset() override;

    /**
     * Get the pitch correction processor
     */
    PitchCorrection* getProcessor() { return m_processor.get(); }

private:
    std::unique_ptr<PitchCorrection> m_processor;
};

/**
 * @class VocalEnhancerNode
 * @brief AudioGraph node for AI vocal enhancement
 */
class VocalEnhancerNode : public AudioNode {
public:
    VocalEnhancerNode();
    ~VocalEnhancerNode() override = default;

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** buffers, int numChannels, int numSamples) override;
    void reset() override;

    /**
     * Get the vocal enhancer processor
     */
    VocalEnhancer* getProcessor() { return m_processor.get(); }

private:
    std::unique_ptr<VocalEnhancer> m_processor;
};

/**
 * @class RecorderNode
 * @brief AudioGraph node for multi-track recording
 */
class RecorderNode : public AudioNode {
public:
    RecorderNode();
    ~RecorderNode() override = default;

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** buffers, int numChannels, int numSamples) override;
    void reset() override;

    /**
     * Get the audio recorder
     */
    AudioRecorder* getRecorder() { return m_recorder.get(); }

private:
    std::unique_ptr<AudioRecorder> m_recorder;
};

/**
 * @class SamplePlayerNode
 * @brief AudioGraph node for playing samples from library
 */
class SamplePlayerNode : public AudioNode {
public:
    SamplePlayerNode();
    ~SamplePlayerNode() override = default;

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** buffers, int numChannels, int numSamples) override;
    void reset() override;

    /**
     * Load sample by UUID
     */
    void loadSample(const juce::String& uuid);

    /**
     * Start playback
     */
    void play();

    /**
     * Stop playback
     */
    void stop();

    /**
     * Set playback loop
     */
    void setLoop(bool loop) { m_loop = loop; }

    /**
     * Set playback gain
     */
    void setGain(float gain) { m_gain = gain; }

    /**
     * Check if playing
     */
    bool isPlaying() const { return m_playing; }

private:
    juce::AudioBuffer<float> m_buffer;
    int m_playPosition = 0;
    bool m_playing = false;
    bool m_loop = false;
    float m_gain = 1.0f;
};

} // namespace omega
