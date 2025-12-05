/**
 * @file AudioRecorder.cpp
 * @brief Implementation of multi-track audio recorder
 */

#include "AudioRecorder.h"
#include <algorithm>

namespace omega {

// ============================================================================
// RecordingTrack Implementation
// ============================================================================

RecordingTrack::RecordingTrack(int trackIndex)
    : m_trackIndex(trackIndex) {
}

void RecordingTrack::initialize(double sampleRate, double maxLengthSeconds) {
    m_sampleRate = sampleRate;
    m_maxSamples = static_cast<int64_t>(sampleRate * maxLengthSeconds);
    
    // Pre-allocate buffer
    m_buffer.resize(m_maxSamples, 0.0f);
    
    resetWritePosition();
    m_recordedSamples.store(0, std::memory_order_release);
}

bool RecordingTrack::write(const float* samples, int numSamples) {
    if (!m_armed.load(std::memory_order_acquire)) {
        return false;
    }
    
    int64_t writePos = m_writePosition.load(std::memory_order_acquire);
    
    // Check if we have space
    if (writePos + numSamples > m_maxSamples) {
        return false; // Buffer full
    }
    
    // Write samples
    std::copy(samples, samples + numSamples, m_buffer.begin() + writePos);
    
    // Update positions
    m_writePosition.store(writePos + numSamples, std::memory_order_release);
    m_recordedSamples.store(writePos + numSamples, std::memory_order_release);
    
    return true;
}

void RecordingTrack::read(float* buffer, int64_t startSample, int numSamples) const {
    // Clamp to valid range
    if (startSample < 0 || startSample >= m_maxSamples) {
        std::fill(buffer, buffer + numSamples, 0.0f);
        return;
    }
    
    int64_t samplesToRead = std::min(static_cast<int64_t>(numSamples),
                                      m_maxSamples - startSample);
    
    std::copy(m_buffer.begin() + startSample,
             m_buffer.begin() + startSample + samplesToRead,
             buffer);
    
    // Fill remainder with silence
    if (samplesToRead < numSamples) {
        std::fill(buffer + samplesToRead, buffer + numSamples, 0.0f);
    }
}

void RecordingTrack::clear() {
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    resetWritePosition();
    m_recordedSamples.store(0, std::memory_order_release);
}

// ============================================================================
// AudioRecorder Implementation
// ============================================================================

AudioRecorder::AudioRecorder() {
    // Initialize all tracks
    for (int i = 0; i < kMaxTracks; ++i) {
        m_tracks[i] = std::make_unique<RecordingTrack>(i);
        m_trackLevels[i].store(0.0f, std::memory_order_release);
    }
}

AudioRecorder::~AudioRecorder() {
    stopRecording();
}

void AudioRecorder::initialize(double sampleRate, double maxLengthSeconds) {
    m_sampleRate = sampleRate;
    m_maxLengthSeconds = maxLengthSeconds;
    
    // Initialize all tracks
    for (auto& track : m_tracks) {
        if (track) {
            track->initialize(sampleRate, maxLengthSeconds);
        }
    }
    
    // Initialize monitor buffers
    const int bufferSize = static_cast<int>(sampleRate * 0.1); // 100ms
    for (auto& buffer : m_monitorBuffers) {
        buffer.resize(bufferSize, 0.0f);
    }
}

bool AudioRecorder::startRecording() {
    State expected = State::Stopped;
    if (!m_state.compare_exchange_strong(expected, State::Recording)) {
        return false;
    }
    
    // Reset recording time
    m_recordingTime.store(0.0, std::memory_order_release);
    
    // Reset write positions for armed tracks
    for (auto& track : m_tracks) {
        if (track && track->isArmed()) {
            track->resetWritePosition();
        }
    }
    
    return true;
}

void AudioRecorder::stopRecording() {
    m_state.store(State::Stopped, std::memory_order_release);
}

void AudioRecorder::pauseRecording() {
    State expected = State::Recording;
    m_state.compare_exchange_strong(expected, State::Paused);
}

void AudioRecorder::resumeRecording() {
    State expected = State::Paused;
    m_state.compare_exchange_strong(expected, State::Recording);
}

void AudioRecorder::processAudio(const float* const* inputBuffers, 
                                int numChannels, int numSamples) {
    if (!shouldRecord()) {
        return;
    }
    
    // Process each armed track
    for (int trackIdx = 0; trackIdx < std::min(numChannels, kMaxTracks); ++trackIdx) {
        auto* track = m_tracks[trackIdx].get();
        if (!track || !track->isArmed()) {
            continue;
        }
        
        const float* inputBuffer = inputBuffers[trackIdx];
        
        // Calculate peak level for metering
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            float absValue = std::abs(inputBuffer[i]);
            if (absValue > peak) {
                peak = absValue;
            }
        }
        m_trackLevels[trackIdx].store(peak, std::memory_order_release);
        
        // Apply track volume
        float volume = track->getVolume();
        std::vector<float> scaledBuffer(numSamples);
        for (int i = 0; i < numSamples; ++i) {
            scaledBuffer[i] = inputBuffer[i] * volume;
        }
        
        // Write to track buffer
        track->write(scaledBuffer.data(), numSamples);
        
        // Update monitor buffer if monitoring is enabled
        if (track->isMonitoring()) {
            auto& monitorBuffer = m_monitorBuffers[trackIdx];
            int copySize = std::min(numSamples, static_cast<int>(monitorBuffer.size()));
            std::copy(scaledBuffer.begin(), 
                     scaledBuffer.begin() + copySize,
                     monitorBuffer.begin());
        }
    }
    
    // Update recording time
    updateRecordingTime(numSamples);
    
    // Check for auto-punch out
    if (m_recordMode == RecordMode::AutoPunch) {
        double currentTime = m_recordingTime.load(std::memory_order_acquire);
        if (currentTime >= m_punchOutTime) {
            stopRecording();
        }
    }
}

void AudioRecorder::getMonitorAudio(float** outputBuffers, 
                                   int numChannels, int numSamples) {
    // Copy monitor buffers to output for zero-latency monitoring
    for (int ch = 0; ch < std::min(numChannels, kMaxTracks); ++ch) {
        auto* track = m_tracks[ch].get();
        if (!track || !track->isMonitoring()) {
            std::fill(outputBuffers[ch], outputBuffers[ch] + numSamples, 0.0f);
            continue;
        }
        
        const auto& monitorBuffer = m_monitorBuffers[ch];
        int copySize = std::min(numSamples, static_cast<int>(monitorBuffer.size()));
        
        std::copy(monitorBuffer.begin(),
                 monitorBuffer.begin() + copySize,
                 outputBuffers[ch]);
        
        // Fill remainder with silence
        if (copySize < numSamples) {
            std::fill(outputBuffers[ch] + copySize,
                     outputBuffers[ch] + numSamples, 0.0f);
        }
    }
}

bool AudioRecorder::exportTrack(int trackIndex, const juce::File& outputFile, 
                               FileFormat format) {
    if (!isValidTrackIndex(trackIndex)) {
        return false;
    }
    
    juce::ScopedLock lock(m_exportLock);
    
    auto* track = getTrack(trackIndex);
    if (!track) {
        return false;
    }
    
    int64_t numSamples = track->getRecordedSamples();
    if (numSamples == 0) {
        return false;
    }
    
    // Create writer
    std::unique_ptr<juce::AudioFormatWriter> writer(createWriter(outputFile, format));
    if (!writer) {
        return false;
    }
    
    // Write audio data
    const auto& buffer = track->getBuffer();
    const float* channelData[1] = { buffer.data() };
    
    return writer->writeFromFloatArrays(channelData, 1, static_cast<int>(numSamples));
}

int AudioRecorder::exportAllTracks(const juce::File& outputDirectory,
                                  const juce::String& fileNamePrefix,
                                  FileFormat format) {
    if (!outputDirectory.exists()) {
        outputDirectory.createDirectory();
    }
    
    int exportedCount = 0;
    
    for (int i = 0; i < kMaxTracks; ++i) {
        auto* track = getTrack(i);
        if (!track || track->getRecordedSamples() == 0) {
            continue;
        }
        
        juce::String fileName = fileNamePrefix + "_Track_" + juce::String(i + 1);
        fileName += (format == FileFormat::WAV_16bit ||
                     format == FileFormat::WAV_24bit ||
                     format == FileFormat::WAV_32bit) ? ".wav" : ".aiff";
        
        juce::File outputFile = outputDirectory.getChildFile(fileName);
        
        if (exportTrack(i, outputFile, format)) {
            exportedCount++;
        }
    }
    
    return exportedCount;
}

bool AudioRecorder::exportMix(const juce::File& outputFile, FileFormat format) {
    juce::ScopedLock lock(m_exportLock);
    
    // Find longest track
    int64_t maxSamples = 0;
    for (const auto& track : m_tracks) {
        if (track) {
            maxSamples = std::max(maxSamples, track->getRecordedSamples());
        }
    }
    
    if (maxSamples == 0) {
        return false;
    }
    
    // Create mix buffer
    std::vector<float> mixBuffer(maxSamples, 0.0f);
    
    // Mix all tracks
    for (const auto& track : m_tracks) {
        if (!track || track->getRecordedSamples() == 0) {
            continue;
        }
        
        const auto& trackBuffer = track->getBuffer();
        int64_t trackSamples = track->getRecordedSamples();
        float volume = track->getVolume();
        
        for (int64_t i = 0; i < trackSamples; ++i) {
            mixBuffer[i] += trackBuffer[i] * volume;
        }
    }
    
    // Normalize if clipping
    float maxLevel = *std::max_element(mixBuffer.begin(), mixBuffer.end(),
        [](float a, float b) { return std::abs(a) < std::abs(b); });
    
    if (std::abs(maxLevel) > 1.0f) {
        float normFactor = 0.99f / std::abs(maxLevel);
        for (auto& sample : mixBuffer) {
            sample *= normFactor;
        }
    }
    
    // Create writer and export
    std::unique_ptr<juce::AudioFormatWriter> writer(createWriter(outputFile, format));
    if (!writer) {
        return false;
    }
    
    const float* channelData[1] = { mixBuffer.data() };
    return writer->writeFromFloatArrays(channelData, 1, static_cast<int>(maxSamples));
}

void AudioRecorder::clearAllTracks() {
    for (auto& track : m_tracks) {
        if (track) {
            track->clear();
        }
    }
}

void AudioRecorder::clearTrack(int trackIndex) {
    if (auto* track = getTrack(trackIndex)) {
        track->clear();
    }
}

void AudioRecorder::armTrack(int trackIndex) {
    if (auto* track = getTrack(trackIndex)) {
        track->arm();
    }
}

void AudioRecorder::disarmTrack(int trackIndex) {
    if (auto* track = getTrack(trackIndex)) {
        track->disarm();
    }
}

bool AudioRecorder::isTrackArmed(int trackIndex) const {
    if (auto* track = getTrack(trackIndex)) {
        return track->isArmed();
    }
    return false;
}

void AudioRecorder::setTrackMonitoring(int trackIndex, bool enabled) {
    if (auto* track = getTrack(trackIndex)) {
        track->setMonitoring(enabled);
    }
}

void AudioRecorder::setTrackVolume(int trackIndex, float volume) {
    if (auto* track = getTrack(trackIndex)) {
        track->setVolume(volume);
    }
}

float AudioRecorder::getTrackVolume(int trackIndex) const {
    if (auto* track = getTrack(trackIndex)) {
        return track->getVolume();
    }
    return 1.0f;
}

void AudioRecorder::setPunchInTime(double timeInSeconds) {
    m_punchInTime = std::max(0.0, timeInSeconds);
}

void AudioRecorder::setPunchOutTime(double timeInSeconds) {
    m_punchOutTime = std::max(0.0, timeInSeconds);
}

void AudioRecorder::setPreRoll(double timeInSeconds) {
    m_preRoll = std::max(0.0, timeInSeconds);
}

int64_t AudioRecorder::getRecordedSamples(int trackIndex) const {
    if (auto* track = getTrack(trackIndex)) {
        return track->getRecordedSamples();
    }
    return 0;
}

int AudioRecorder::getArmedTrackCount() const {
    int count = 0;
    for (const auto& track : m_tracks) {
        if (track && track->isArmed()) {
            ++count;
        }
    }
    return count;
}

float AudioRecorder::getTrackLevel(int trackIndex) const {
    if (isValidTrackIndex(trackIndex)) {
        return m_trackLevels[trackIndex].load(std::memory_order_acquire);
    }
    return 0.0f;
}

bool AudioRecorder::isValidTrackIndex(int index) const {
    return index >= 0 && index < kMaxTracks;
}

RecordingTrack* AudioRecorder::getTrack(int index) {
    if (!isValidTrackIndex(index)) {
        return nullptr;
    }
    return m_tracks[index].get();
}

const RecordingTrack* AudioRecorder::getTrack(int index) const {
    if (!isValidTrackIndex(index)) {
        return nullptr;
    }
    return m_tracks[index].get();
}

void AudioRecorder::updateRecordingTime(int numSamples) {
    double increment = numSamples / m_sampleRate;
    double currentTime = m_recordingTime.load(std::memory_order_acquire);
    m_recordingTime.store(currentTime + increment, std::memory_order_release);
}

bool AudioRecorder::shouldRecord() const {
    State state = m_state.load(std::memory_order_acquire);
    
    if (state != State::Recording) {
        return false;
    }
    
    // Check punch-in time for relevant modes
    if (m_recordMode == RecordMode::PunchIn || m_recordMode == RecordMode::AutoPunch) {
        double currentTime = m_recordingTime.load(std::memory_order_acquire);
        if (currentTime < m_punchInTime) {
            return false;
        }
    }
    
    return true;
}

juce::AudioFormatWriter* AudioRecorder::createWriter(const juce::File& file, 
                                                     FileFormat format) {
    juce::WavAudioFormat wavFormat;
    juce::AiffAudioFormat aiffFormat;
    
    juce::AudioFormat* audioFormat = nullptr;
    int bitsPerSample = 16;
    
    switch (format) {
        case FileFormat::WAV_16bit:
            audioFormat = &wavFormat;
            bitsPerSample = 16;
            break;
        case FileFormat::WAV_24bit:
            audioFormat = &wavFormat;
            bitsPerSample = 24;
            break;
        case FileFormat::WAV_32bit:
            audioFormat = &wavFormat;
            bitsPerSample = 32;
            break;
        case FileFormat::AIFF_16bit:
            audioFormat = &aiffFormat;
            bitsPerSample = 16;
            break;
        case FileFormat::AIFF_24bit:
            audioFormat = &aiffFormat;
            bitsPerSample = 24;
            break;
        case FileFormat::AIFF_32bit:
            audioFormat = &aiffFormat;
            bitsPerSample = 32;
            break;
    }
    
    if (!audioFormat) {
        return nullptr;
    }
    
    // Create output stream
    file.deleteFile(); // Remove existing file
    std::unique_ptr<juce::FileOutputStream> outputStream(file.createOutputStream());
    
    if (!outputStream) {
        return nullptr;
    }
    
    // Create writer
    juce::StringPairArray metadata;
    metadata.set("Software", "OmegaStudio");
    
    return audioFormat->createWriterFor(outputStream.release(),
                                       m_sampleRate,
                                       1, // Mono
                                       bitsPerSample,
                                       metadata,
                                       0);
}

} // namespace omega
