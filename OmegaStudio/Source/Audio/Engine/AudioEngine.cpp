//==============================================================================
// AudioEngine.cpp
// Implementation of the real-time audio engine
//==============================================================================

#include "AudioEngine.h"
#include "../Graph/AudioGraph.h"
#include "../Graph/ProcessorNodes.h"
#include "../Recording/AudioRecorder.h"
#include "../Plugins/PluginManager.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace Omega::Audio {

//==============================================================================
AudioEngine::AudioEngine()
    : deviceManager_(std::make_unique<juce::AudioDeviceManager>())
    , audioGraph_(nullptr)
    , audioMemoryPool_(nullptr)
{
    juce::Logger::writeToLog("OmegaStudio AudioEngine initialized");
}

//==============================================================================
AudioEngine::~AudioEngine() {
    shutdown();
    juce::Logger::writeToLog("OmegaStudio AudioEngine destroyed");
}

//==============================================================================
bool AudioEngine::initialize(const AudioEngineConfig& config) {
    if (!validateConfig(config)) {
        setError("Invalid audio configuration");
        return false;
    }
    
    config_ = config;
    
    // Initialize memory pool for audio thread
    // 16 MB pool with 64-byte blocks
    audioMemoryPool_ = std::make_unique<Memory::MemoryPool>(
        Memory::AUDIO_POOL_SIZE, 
        Memory::POOL_BLOCK_SIZE
    );
    
    // Initialize audio graph and core nodes
    audioGraph_ = std::make_unique<AudioGraph>();
    mixerEngine_ = std::make_unique<OmegaStudio::MixerEngine>();

    inputNodeId_ = audioGraph_->addNode(std::make_unique<InputNode>(config_.numInputChannels));
    pluginNodeId_ = audioGraph_->addNode(std::make_unique<PluginNode>());
    mixerNodeId_ = audioGraph_->addNode(std::make_unique<MixerNode>(*mixerEngine_));
    outputNodeId_ = audioGraph_->addNode(std::make_unique<OutputNode>(config_.numOutputChannels));
    audioGraph_->setInputNodeId(inputNodeId_);
    audioGraph_->setOutputNodeId(outputNodeId_);
    audioGraph_->connect(inputNodeId_, 0, pluginNodeId_, 0);
    audioGraph_->connect(pluginNodeId_, 0, mixerNodeId_, 0);
    audioGraph_->connect(mixerNodeId_, 0, outputNodeId_, 0);

    // Initialize recorder
    recorder_ = std::make_unique<omega::AudioRecorder>();
    
    // Initialize JUCE audio device manager
    juce::String error = deviceManager_->initialise(
        config_.numInputChannels,   // Input channels
        config_.numOutputChannels,  // Output channels
        nullptr,                    // Saved state (XML)
        true,                       // Select default device
        config_.deviceName,         // Preferred device name
        nullptr                     // Preferred setup
    );
    
    if (error.isNotEmpty()) {
        setError(error);
        state_.store(EngineState::Error, std::memory_order_release);
        return false;
    }
    
    // Get current device
    auto* device = deviceManager_->getCurrentAudioDevice();
    if (device == nullptr) {
        setError("Failed to open audio device");
        state_.store(EngineState::Error, std::memory_order_release);
        return false;
    }
    
    // Configure buffer size via AudioDeviceManager setup
    if (config_.bufferSize > 0) {
        auto setup = deviceManager_->getAudioDeviceSetup();
        setup.bufferSize = config_.bufferSize;
        deviceManager_->setAudioDeviceSetup(setup, true);
    }
    
    // Store actual device parameters
    const double deviceSampleRate = device->getCurrentSampleRate();
    const int deviceBufferSize = device->getCurrentBufferSizeSamples();
    currentSampleRate_.store(deviceSampleRate);
    currentBufferSize_.store(deviceBufferSize);
    
    juce::Logger::writeToLog(
        juce::String::formatted(
            "Audio device opened: %s @ %.1f Hz, buffer: %d samples",
            device->getName().toRawUTF8(),
            device->getCurrentSampleRate(),
            device->getCurrentBufferSizeSamples()
        )
    );
    
    // Add this as the audio callback
    deviceManager_->addAudioCallback(this);
    
    // Initialize recorder with actual sample rate
    if (recorder_) {
        recorder_->initialize(deviceSampleRate);
    }

    state_.store(EngineState::Initialized, std::memory_order_release);
    return true;
}

//==============================================================================
void AudioEngine::shutdown() {
    if (state_.load(std::memory_order_acquire) == EngineState::Uninitialized) {
        return;
    }
    
    stop();
    
    // Remove callback and close device
    deviceManager_->removeAudioCallback(this);
    deviceManager_->closeAudioDevice();
    
    // Cleanup
    audioGraph_.reset();
    audioMemoryPool_.reset();
    recorder_.reset();
    
    state_.store(EngineState::Uninitialized, std::memory_order_release);
    juce::Logger::writeToLog("AudioEngine shutdown complete");
}

//==============================================================================
void AudioEngine::start() {
    auto currentState = state_.load(std::memory_order_acquire);
    if (currentState == EngineState::Initialized || 
        currentState == EngineState::Stopped) {
        state_.store(EngineState::Running, std::memory_order_release);
        juce::Logger::writeToLog("AudioEngine started");
    }
}

//==============================================================================
void AudioEngine::stop() {
    if (state_.load(std::memory_order_acquire) == EngineState::Running) {
        state_.store(EngineState::Stopped, std::memory_order_release);
        juce::Logger::writeToLog("AudioEngine stopped");
    }
}

//==============================================================================
void AudioEngine::reset() {
    // Reset audio graph and clear buffers
    if (audioGraph_) {
        // audioGraph_->reset(); // Will implement when AudioGraph is ready
    }
    
    totalCallbacks_.store(0, std::memory_order_relaxed);
    totalSamplesProcessed_.store(0, std::memory_order_relaxed);
    cpuLoad_.store(0.0);
}

//==============================================================================
bool AudioEngine::isRunning() const noexcept {
    return state_.load(std::memory_order_acquire) == EngineState::Running;
}

//==============================================================================
// THE CRITICAL FUNCTION: Audio Callback
// This runs on a real-time thread with highest priority
// RULES:
// - NO malloc/free/new/delete
// - NO mutex locks (use lock-free structures only)
// - NO file I/O
// - NO system calls
// - Keep processing time < buffer duration
//==============================================================================
void AudioEngine::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // Measure CPU time (for load calculation)
    const auto startTime = juce::Time::getHighResolutionTicks();
    
    // Increment statistics
    totalCallbacks_.fetch_add(1, std::memory_order_relaxed);
    totalSamplesProcessed_.fetch_add(numSamples, std::memory_order_relaxed);
    
    // Check if engine is running
    if (state_.load(std::memory_order_acquire) != EngineState::Running) {
        // If stopped, output silence
        for (int ch = 0; ch < numOutputChannels; ++ch) {
            if (outputChannelData[ch] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
            }
        }
        return;
    }
    
    // Pull MIDI events from RT queue into buffer
    pumpMIDIInput(numSamples);

    // Set external buffers for IO nodes (if present)
    if (audioGraph_) {
        if (auto* inNode = dynamic_cast<InputNode*>(audioGraph_->getNode(inputNodeId_))) {
            inNode->setExternalInput(inputChannelData, numInputChannels, numSamples);
        }
        if (auto* outNode = dynamic_cast<OutputNode*>(audioGraph_->getNode(outputNodeId_))) {
            outNode->setExternalOutput(outputChannelData, numOutputChannels, numSamples);
        }
        if (auto* pNode = dynamic_cast<PluginNode*>(audioGraph_->getNode(pluginNodeId_))) {
            pNode->setMidiBuffer(&audioThreadMidi_);
        }
        if (auto* mNode = dynamic_cast<MixerNode*>(audioGraph_->getNode(mixerNodeId_))) {
            // Ensure one channel buffer and midi buffer for the mixer
            if (channelBuffersStorage_.size() < 1) channelBuffersStorage_.resize(1);
            channelBuffersStorage_[0].setSize(numOutputChannels, numSamples, false, false, true);
            channelBufferPtrs_.clear();
            channelBufferPtrs_.push_back(&channelBuffersStorage_[0]);

            midiBufferPtrs_.clear();
            midiBufferPtrs_.push_back(&audioThreadMidi_);

            mNode->setChannelBuffers(&channelBufferPtrs_);
            mNode->setMidiBuffers(&midiBufferPtrs_);
        }

        audioGraph_->process(inputChannelData, numInputChannels,
                             outputChannelData, numOutputChannels, numSamples);
    }

    // Record incoming audio if armed/recording
    if (recorder_ && recorder_->isRecording()) {
        recorder_->processAudio(inputChannelData, numInputChannels, numSamples);
    }
    
    // For now, pass through input to output (or generate silence)
    if (numInputChannels > 0 && inputChannelData[0] != nullptr) {
        // Pass-through mode
        for (int ch = 0; ch < juce::jmin(numInputChannels, numOutputChannels); ++ch) {
            if (outputChannelData[ch] != nullptr && inputChannelData[ch] != nullptr) {
                juce::FloatVectorOperations::copy(
                    outputChannelData[ch], 
                    inputChannelData[ch], 
                    numSamples
                );
            }
        }
    } else {
        // Generate silence
        for (int ch = 0; ch < numOutputChannels; ++ch) {
            if (outputChannelData[ch] != nullptr) {
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
            }
        }
    }
    
    // Calculate CPU load
    const auto endTime = juce::Time::getHighResolutionTicks();
    const double elapsedTime = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    const double bufferDuration = numSamples / currentSampleRate_.load();
    const double load = (bufferDuration > 0.0) ? (elapsedTime / bufferDuration) : 0.0;
    
    updateCpuLoad(load);
    
    // Send meter levels to GUI (example)
    // This is lock-free and real-time safe
    if (numOutputChannels > 0 && outputChannelData[0] != nullptr) {
        float peakLevel = juce::FloatVectorOperations::findMaximum(
            outputChannelData[0], numSamples
        );
        
        Memory::AudioMessage msg{Memory::MessageType::MeterLevel, 0, peakLevel};
        messageQueue_.push(msg);  // Lock-free push
    }
}

//==============================================================================
void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    currentSampleRate_.store(device->getCurrentSampleRate());
    currentBufferSize_.store(device->getCurrentBufferSizeSamples());
    
    juce::Logger::writeToLog(
        juce::String::formatted(
            "Audio device about to start: %.1f Hz, %d samples",
            device->getCurrentSampleRate(),
            device->getCurrentBufferSizeSamples()
        )
    );
    
    reset();

    prepareGraph(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());

    if (recorder_) {
        recorder_->initialize(device->getCurrentSampleRate());
    }
}

//==============================================================================
void AudioEngine::audioDeviceStopped() {
    juce::Logger::writeToLog("Audio device stopped");
}

//==============================================================================
// Recording control
//==============================================================================
bool AudioEngine::startRecording() {
    if (!recorder_) return false;
    // Arm default track 0 if none armed
    recorder_->armTrack(0);
    return recorder_->startRecording();
}

void AudioEngine::stopRecording() {
    if (!recorder_) return;
    recorder_->stopRecording();
}

bool AudioEngine::isRecording() const noexcept {
    return recorder_ && recorder_->isRecording();
}

void AudioEngine::armTrack(int trackIndex) {
    if (recorder_) recorder_->armTrack(trackIndex);
}

void AudioEngine::disarmTrack(int trackIndex) {
    if (recorder_) recorder_->disarmTrack(trackIndex);
}

//==============================================================================
void AudioEngine::audioDeviceError(const juce::String& errorMessage) {
    setError(errorMessage);
    state_.store(EngineState::Error, std::memory_order_release);
    
    juce::Logger::writeToLog("Audio device error: " + errorMessage);
}

//==============================================================================
// Getters
//==============================================================================
EngineState AudioEngine::getState() const noexcept {
    return state_.load(std::memory_order_acquire);
}

double AudioEngine::getCpuLoad() const noexcept {
    return cpuLoad_.load();
}

double AudioEngine::getSampleRate() const noexcept {
    return currentSampleRate_.load();
}

int AudioEngine::getBufferSize() const noexcept {
    return currentBufferSize_.load();
}

AudioGraph* AudioEngine::getAudioGraph() noexcept {
    return audioGraph_.get();
}

const AudioGraph* AudioEngine::getAudioGraph() const noexcept {
    return audioGraph_.get();
}

Memory::MessageFIFO& AudioEngine::getMessageFIFO() noexcept {
    return messageQueue_;
}

juce::String AudioEngine::getLastError() const {
    const juce::ScopedLock lock(errorLock_);
    return lastError_;
}

juce::String AudioEngine::getCurrentDeviceName() const {
    auto* device = deviceManager_->getCurrentAudioDevice();
    return device ? device->getName() : juce::String();
}

//==============================================================================
// Internal Helpers
//==============================================================================
void AudioEngine::setError(const juce::String& error) {
    const juce::ScopedLock lock(errorLock_);
    lastError_ = error;
}

void AudioEngine::updateCpuLoad(double load) {
    // Simple exponential smoothing
    constexpr double alpha = 0.1;
    double currentLoad = cpuLoad_.load();
    double newLoad = alpha * load + (1.0 - alpha) * currentLoad;
    cpuLoad_.store(newLoad);
}

bool AudioEngine::validateConfig(const AudioEngineConfig& config) const {
    return config.isValid();
}

void AudioEngine::pumpMIDIInput(int numSamples) {
    audioThreadMidi_.clear();
    if (!midiManager_) return;

    auto& inQueue = midiManager_->getInputQueue();
    auto& outQueue = midiManager_->getOutputQueue();

    while (true) {
        auto evOpt = inQueue.pop();
        if (!evOpt.has_value()) break;

        auto ev = evOpt.value();
        const int pos = juce::jlimit(0, juce::jmax(0, numSamples - 1), ev.samplePosition);
        audioThreadMidi_.addEvent(ev.toMessage(), pos);

        // Echo to output queue for GUI/hardware feedback (non-blocking)
        outQueue.push(ev);
    }
}

void AudioEngine::prepareGraph(double sampleRate, int blockSize) {
    if (!audioGraph_) return;

    for (NodeID id : {inputNodeId_, pluginNodeId_, mixerNodeId_, outputNodeId_}) {
        if (auto* node = audioGraph_->getNode(id)) {
            node->prepare(sampleRate, blockSize);
        }
    }

    if (mixerEngine_) {
        mixerEngine_->prepareToPlay(sampleRate, blockSize);
    }
}

bool AudioEngine::addPluginToGraph(const juce::String& pluginUID) {
    if (!audioGraph_) return false;
    auto* pNode = dynamic_cast<PluginNode*>(audioGraph_->getNode(pluginNodeId_));
    if (!pNode) return false;

    auto plugin = OmegaStudio::PluginManager::getInstance().loadPlugin(pluginUID);
    if (!plugin) return false;

    pNode->chain().clearPlugins();
    pNode->chain().addPlugin(std::move(plugin));
    // Recompute latency on graph
    audioGraph_->updateLatencyCompensation();
    return true;
}

bool AudioEngine::clearGraphPlugins() {
    if (!audioGraph_) return false;
    auto* pNode = dynamic_cast<PluginNode*>(audioGraph_->getNode(pluginNodeId_));
    if (!pNode) return false;
    pNode->chain().clearPlugins();
    audioGraph_->updateLatencyCompensation();
    return true;
}

} // namespace Omega::Audio
