//==============================================================================
// AudioEngine.h
// The core real-time audio processing engine
//
// ARCHITECTURE:
// - Initializes audio devices (ASIO/CoreAudio via JUCE)
// - Manages the audio callback (THE most critical function)
// - Thread-safe communication with GUI via lock-free FIFOs
// - Owns the audio graph and DSP processors
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <atomic>
#include "../../Memory/MemoryPool.h"
#include "../../Memory/LockFreeFIFO.h"
#include "../../Utils/Constants.h"
#include "../../Utils/Atomic.h"

namespace Omega::Audio {

//==============================================================================
// Forward declarations
//==============================================================================
class AudioGraph;
class AudioCallback;

//==============================================================================
// Audio Engine State
//==============================================================================
enum class EngineState {
    Uninitialized,
    Initialized,
    Running,
    Stopped,
    Error
};

//==============================================================================
// Audio Engine Configuration
//==============================================================================
struct AudioEngineConfig {
    double sampleRate = Audio::DEFAULT_SAMPLE_RATE;
    int bufferSize = Audio::DEFAULT_BUFFER_SIZE;
    int numInputChannels = Audio::DEFAULT_INPUT_CHANNELS;
    int numOutputChannels = Audio::DEFAULT_OUTPUT_CHANNELS;
    juce::String deviceName;  // Empty = use default device
    
    [[nodiscard]] bool isValid() const noexcept {
        return sampleRate > 0.0 && bufferSize > 0 && 
               bufferSize <= Audio::MAX_BUFFER_SIZE &&
               numOutputChannels > 0;
    }
};

//==============================================================================
// AudioEngine - Main audio processing system
//==============================================================================
class AudioEngine : public juce::AudioIODeviceCallback {
public:
    //==========================================================================
    // Constructor & Destructor
    //==========================================================================
    AudioEngine();
    ~AudioEngine() override;
    
    // Non-copyable, non-movable (singleton-like behavior)
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;
    
    //==========================================================================
    // Initialization & Shutdown
    //==========================================================================
    [[nodiscard]] bool initialize(const AudioEngineConfig& config);
    void shutdown();
    
    //==========================================================================
    // Device Management
    //==========================================================================
    [[nodiscard]] bool openAudioDevice(const juce::String& deviceName);
    void closeAudioDevice();
    [[nodiscard]] juce::StringArray getAvailableDevices() const;
    [[nodiscard]] juce::String getCurrentDeviceName() const;
    
    //==========================================================================
    // Transport Control
    //==========================================================================
    void start();
    void stop();
    void reset();
    [[nodiscard]] bool isRunning() const noexcept;
    
    //==========================================================================
    // Audio Configuration
    //==========================================================================
    void setBufferSize(int newBufferSize);
    void setSampleRate(double newSampleRate);
    [[nodiscard]] int getBufferSize() const noexcept;
    [[nodiscard]] double getSampleRate() const noexcept;
    [[nodiscard]] int getNumInputChannels() const noexcept;
    [[nodiscard]] int getNumOutputChannels() const noexcept;
    
    //==========================================================================
    // State Query
    //==========================================================================
    [[nodiscard]] EngineState getState() const noexcept;
    [[nodiscard]] juce::String getLastError() const;
    [[nodiscard]] double getCpuLoad() const noexcept;
    
    //==========================================================================
    // Audio Graph Access (for adding instruments/effects)
    //==========================================================================
    [[nodiscard]] AudioGraph* getAudioGraph() noexcept;
    [[nodiscard]] const AudioGraph* getAudioGraph() const noexcept;
    
    //==========================================================================
    // Message Communication (GUI ← Audio Thread)
    //==========================================================================
    [[nodiscard]] Memory::MessageFIFO& getMessageFIFO() noexcept;
    
    //==========================================================================
    // JUCE AudioIODeviceCallback Interface
    // These are called by JUCE on the real-time audio thread
    //==========================================================================
    void audioDeviceIOCallbackWithContext(
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;
    
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& errorMessage) override;
    
private:
    //==========================================================================
    // Internal State
    //==========================================================================
    std::unique_ptr<juce::AudioDeviceManager> deviceManager_;
    std::unique_ptr<AudioGraph> audioGraph_;
    std::unique_ptr<Memory::MemoryPool> audioMemoryPool_;
    
    //==========================================================================
    // Configuration
    //==========================================================================
    AudioEngineConfig config_;
    
    //==========================================================================
    // Atomic State (thread-safe access)
    //==========================================================================
    std::atomic<EngineState> state_{EngineState::Uninitialized};
    Utils::RelaxedAtomic<double> cpuLoad_{0.0};
    Utils::RelaxedAtomic<double> currentSampleRate_{0.0};
    Utils::RelaxedAtomic<int> currentBufferSize_{0};
    
    //==========================================================================
    // Error Handling
    //==========================================================================
    mutable juce::CriticalSection errorLock_;
    juce::String lastError_;
    
    //==========================================================================
    // Communication Channels
    //==========================================================================
    Memory::MessageFIFO messageQueue_;  // Audio → GUI messages
    
    //==========================================================================
    // Performance Monitoring
    //==========================================================================
    std::atomic<uint64_t> totalCallbacks_{0};
    std::atomic<uint64_t> totalSamplesProcessed_{0};
    juce::Time lastCallbackTime_;
    
    //==========================================================================
    // Internal Helpers
    //==========================================================================
    void setError(const juce::String& error);
    void updateCpuLoad(double load);
    [[nodiscard]] bool validateConfig(const AudioEngineConfig& config) const;
};

} // namespace Omega::Audio
