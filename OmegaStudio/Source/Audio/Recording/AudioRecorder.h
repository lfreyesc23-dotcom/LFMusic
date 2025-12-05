/**
 * @file AudioRecorder.h
 * @brief Professional multi-track audio recorder with real-time monitoring
 * 
 * Features:
 * - Multi-track recording with independent arm/disarm
 * - Lock-free circular buffer for RT-safe operation
 * - WAV/AIFF export with metadata support
 * - Real-time input monitoring with zero-latency
 * - Punch-in/punch-out recording
 * - Auto-punch with pre-roll
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <atomic>
#include <array>
#include <vector>
#include "../../Memory/LockFreeFIFO.h"
#include "../../Utils/Constants.h"

namespace omega {

/**
 * @class RecordingTrack
 * @brief Represents a single recording track
 */
class RecordingTrack {
public:
    RecordingTrack(int trackIndex);
    ~RecordingTrack() = default;

    /**
     * Initialize track with sample rate and max recording length
     * @param sampleRate Audio sample rate
     * @param maxLengthSeconds Maximum recording length in seconds
     */
    void initialize(double sampleRate, double maxLengthSeconds);

    /**
     * Write audio samples to track (RT-safe)
     * @param samples Audio samples to write
     * @param numSamples Number of samples
     * @return True if written successfully
     */
    bool write(const float* samples, int numSamples);

    /**
     * Read audio samples from track
     * @param buffer Destination buffer
     * @param startSample Start position in samples
     * @param numSamples Number of samples to read
     */
    void read(float* buffer, int64_t startSample, int numSamples) const;

    /**
     * Clear track buffer
     */
    void clear();

    /**
     * Arm track for recording
     */
    void arm() noexcept { m_armed.store(true, std::memory_order_release); }

    /**
     * Disarm track
     */
    void disarm() noexcept { m_armed.store(false, std::memory_order_release); }

    /**
     * Check if track is armed
     */
    bool isArmed() const noexcept { return m_armed.load(std::memory_order_acquire); }

    /**
     * Enable/disable monitoring
     */
    void setMonitoring(bool enabled) noexcept {
        m_monitoring.store(enabled, std::memory_order_release);
    }

    /**
     * Check if monitoring is enabled
     */
    bool isMonitoring() const noexcept {
        return m_monitoring.load(std::memory_order_acquire);
    }

    /**
     * Set track volume (0.0 to 1.0)
     */
    void setVolume(float volume) noexcept {
        m_volume.store(volume, std::memory_order_release);
    }

    /**
     * Get track volume
     */
    float getVolume() const noexcept {
        return m_volume.load(std::memory_order_acquire);
    }

    /**
     * Get current write position
     */
    int64_t getWritePosition() const noexcept {
        return m_writePosition.load(std::memory_order_acquire);
    }

    /**
     * Reset write position
     */
    void resetWritePosition() noexcept {
        m_writePosition.store(0, std::memory_order_release);
    }

    /**
     * Get total recorded samples
     */
    int64_t getRecordedSamples() const noexcept {
        return m_recordedSamples.load(std::memory_order_acquire);
    }

    /**
     * Get track buffer for export
     */
    const std::vector<float>& getBuffer() const { return m_buffer; }

    int getTrackIndex() const noexcept { return m_trackIndex; }

private:
    int m_trackIndex;
    std::vector<float> m_buffer;
    double m_sampleRate = 48000.0;
    
    std::atomic<bool> m_armed{false};
    std::atomic<bool> m_monitoring{false};
    std::atomic<float> m_volume{1.0f};
    std::atomic<int64_t> m_writePosition{0};
    std::atomic<int64_t> m_recordedSamples{0};
    
    int64_t m_maxSamples = 0;
};

/**
 * @class AudioRecorder
 * @brief Professional multi-track audio recorder
 * 
 * Thread-safe multi-track recorder with features:
 * - Up to 64 simultaneous recording tracks
 * - Lock-free operation for real-time safety
 * - Automatic file management
 * - Punch recording with pre-roll
 * - Input monitoring with latency compensation
 */
class AudioRecorder {
public:
    /**
     * Recording state
     */
    enum class State {
        Stopped,
        Recording,
        Paused
    };

    /**
     * File format for export
     */
    enum class FileFormat {
        WAV_16bit,
        WAV_24bit,
        WAV_32bit,
        AIFF_16bit,
        AIFF_24bit,
        AIFF_32bit
    };

    /**
     * Recording mode
     */
    enum class RecordMode {
        Normal,         // Standard recording
        PunchIn,        // Punch-in at specific time
        Loop,           // Loop recording with takes
        AutoPunch       // Automatic punch in/out
    };

    static constexpr int kMaxTracks = 64;
    static constexpr int kDefaultMaxLength = 600; // 10 minutes

    AudioRecorder();
    ~AudioRecorder();

    /**
     * Initialize recorder
     * @param sampleRate Audio sample rate
     * @param maxLengthSeconds Maximum recording length per track
     */
    void initialize(double sampleRate, double maxLengthSeconds = kDefaultMaxLength);

    /**
     * Start recording
     * @return True if started successfully
     */
    bool startRecording();

    /**
     * Stop recording
     */
    void stopRecording();

    /**
     * Pause recording
     */
    void pauseRecording();

    /**
     * Resume recording
     */
    void resumeRecording();

    /**
     * Process audio for recording (RT-safe, called from audio thread)
     * @param inputBuffers Array of input channel buffers
     * @param numChannels Number of input channels
     * @param numSamples Number of samples to process
     */
    void processAudio(const float* const* inputBuffers, int numChannels, int numSamples);

    /**
     * Get monitored audio for playback (RT-safe)
     * @param outputBuffers Array of output buffers
     * @param numChannels Number of output channels
     * @param numSamples Number of samples
     */
    void getMonitorAudio(float** outputBuffers, int numChannels, int numSamples);

    /**
     * Export track to file
     * @param trackIndex Track to export
     * @param outputFile Output file path
     * @param format File format
     * @return True if exported successfully
     */
    bool exportTrack(int trackIndex, const juce::File& outputFile, FileFormat format);

    /**
     * Export all armed tracks to separate files
     * @param outputDirectory Output directory
     * @param fileNamePrefix Prefix for generated filenames
     * @param format File format
     * @return Number of tracks exported
     */
    int exportAllTracks(const juce::File& outputDirectory, 
                       const juce::String& fileNamePrefix,
                       FileFormat format);

    /**
     * Export mix of all armed tracks to single file
     * @param outputFile Output file
     * @param format File format
     * @return True if exported successfully
     */
    bool exportMix(const juce::File& outputFile, FileFormat format);

    /**
     * Clear all tracks
     */
    void clearAllTracks();

    /**
     * Clear specific track
     * @param trackIndex Track to clear
     */
    void clearTrack(int trackIndex);

    // ============ Track Management ============

    /**
     * Arm track for recording
     * @param trackIndex Track index (0-63)
     */
    void armTrack(int trackIndex);

    /**
     * Disarm track
     * @param trackIndex Track index (0-63)
     */
    void disarmTrack(int trackIndex);

    /**
     * Check if track is armed
     */
    bool isTrackArmed(int trackIndex) const;

    /**
     * Enable/disable monitoring for track
     */
    void setTrackMonitoring(int trackIndex, bool enabled);

    /**
     * Set track input volume
     */
    void setTrackVolume(int trackIndex, float volume);

    /**
     * Get track volume
     */
    float getTrackVolume(int trackIndex) const;

    // ============ Recording Mode ============

    /**
     * Set recording mode
     */
    void setRecordMode(RecordMode mode) noexcept {
        m_recordMode = mode;
    }

    /**
     * Get current recording mode
     */
    RecordMode getRecordMode() const noexcept {
        return m_recordMode;
    }

    /**
     * Set punch-in time (for PunchIn and AutoPunch modes)
     * @param timeInSeconds Time in seconds
     */
    void setPunchInTime(double timeInSeconds);

    /**
     * Set punch-out time (for AutoPunch mode)
     * @param timeInSeconds Time in seconds
     */
    void setPunchOutTime(double timeInSeconds);

    /**
     * Set pre-roll time
     * @param timeInSeconds Time in seconds
     */
    void setPreRoll(double timeInSeconds);

    // ============ State Queries ============

    /**
     * Get current recording state
     */
    State getState() const noexcept {
        return m_state.load(std::memory_order_acquire);
    }

    /**
     * Check if currently recording
     */
    bool isRecording() const noexcept {
        return getState() == State::Recording;
    }

    /**
     * Get current recording time in seconds
     */
    double getRecordingTime() const noexcept {
        return m_recordingTime.load(std::memory_order_acquire);
    }

    /**
     * Get total recorded samples for track
     */
    int64_t getRecordedSamples(int trackIndex) const;

    /**
     * Get number of armed tracks
     */
    int getArmedTrackCount() const;

    /**
     * Get recording level for track (peak)
     * @param trackIndex Track index
     * @return Peak level (0.0 to 1.0+)
     */
    float getTrackLevel(int trackIndex) const;

private:
    // Helper functions
    bool isValidTrackIndex(int index) const;
    RecordingTrack* getTrack(int index);
    const RecordingTrack* getTrack(int index) const;
    void updateRecordingTime(int numSamples);
    bool shouldRecord() const;
    juce::AudioFormatWriter* createWriter(const juce::File& file, FileFormat format);

    // Tracks
    std::array<std::unique_ptr<RecordingTrack>, kMaxTracks> m_tracks;

    // State
    std::atomic<State> m_state{State::Stopped};
    std::atomic<double> m_recordingTime{0.0};
    double m_sampleRate = 48000.0;
    double m_maxLengthSeconds = kDefaultMaxLength;

    // Recording mode
    RecordMode m_recordMode = RecordMode::Normal;
    double m_punchInTime = 0.0;
    double m_punchOutTime = 0.0;
    double m_preRoll = 2.0; // 2 seconds default

    // Metering (for UI feedback)
    std::array<std::atomic<float>, kMaxTracks> m_trackLevels;

    // Monitoring buffers (for zero-latency monitoring)
    std::array<std::vector<float>, kMaxTracks> m_monitorBuffers;

    // Thread safety
    juce::CriticalSection m_exportLock; // Only for non-RT export operations
};

} // namespace omega
