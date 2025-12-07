#pragma once
#include <JuceHeader.h>
#include <functional>

namespace OmegaStudio {
namespace Audio {

/**
 * @brief Professional Audio Recording System (FL Studio 2025 Feature)
 * Records internal/external audio with automatic clip creation
 */
class AudioRecorder : public juce::AudioIODeviceCallback
{
public:
    enum class RecordingSource {
        ExternalInput,      // Microphone, line-in
        InternalOutput,     // Record from mixer output
        MixerTrack,        // Record specific mixer track
        MasterOutput       // Record final master
    };

    struct RecordingSettings {
        RecordingSource source = RecordingSource::ExternalInput;
        int inputChannel = 0;
        bool autoNormalize = false;
        bool createClipAutomatically = true;
        juce::String targetTrackName;
        double preRoll = 0.0;  // seconds
        double countIn = 0.0;  // seconds
    };

    struct RecordingInfo {
        double duration = 0.0;
        int samplesCaptured = 0;
        double peakLevel = 0.0;
        juce::String filePath;
        bool isRecording = false;
    };

    AudioRecorder();
    ~AudioRecorder() override;

    // Recording control
    void startRecording(const RecordingSettings& settings = RecordingSettings());
    void stopRecording();
    void pauseRecording();
    void resumeRecording();
    bool isRecording() const { return recording; }
    bool isPaused() const { return paused; }

    // Get recorded audio
    juce::AudioBuffer<float> getRecordedAudio() const;
    void clearRecording();

    // Save to file
    bool saveToFile(const juce::File& file);
    bool saveToWav(const juce::File& file);
    bool saveToFlac(const juce::File& file);

    // Monitoring
    RecordingInfo getRecordingInfo() const;
    std::function<void(const RecordingInfo&)> onRecordingUpdate;
    std::function<void(juce::AudioBuffer<float>&)> onRecordingComplete;

    // AudioIODeviceCallback
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                         int numInputChannels,
                                         float* const* outputChannelData,
                                         int numOutputChannels,
                                         int numSamples,
                                         const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // Punch-in/out recording
    void setPunchInOut(double punchIn, double punchOut);
    void enablePunchRecording(bool enable);

private:
    juce::AudioBuffer<float> recordBuffer;
    juce::CriticalSection recordLock;
    
    bool recording = false;
    bool paused = false;
    double sampleRate = 44100.0;
    int currentSamplePos = 0;
    
    RecordingSettings currentSettings;
    RecordingInfo currentInfo;

    // Punch recording
    bool punchRecordingEnabled = false;
    double punchInTime = 0.0;
    double punchOutTime = 0.0;

    // Level monitoring
    float peakLevelL = 0.0f;
    float peakLevelR = 0.0f;

    void processInputBuffer(const float* const* inputData, int numChannels, int numSamples);
    void updateRecordingInfo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRecorder)
};

} // namespace Audio
} // namespace OmegaStudio
