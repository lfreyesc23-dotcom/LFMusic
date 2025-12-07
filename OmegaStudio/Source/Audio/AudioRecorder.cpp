#include "AudioRecorder.h"

namespace OmegaStudio {
namespace Audio {

AudioRecorder::AudioRecorder()
{
    recordBuffer.setSize(2, 44100 * 60); // 1 minute initial buffer
}

AudioRecorder::~AudioRecorder()
{
    stopRecording();
}

void AudioRecorder::startRecording(const RecordingSettings& settings)
{
    juce::ScopedLock lock(recordLock);
    
    currentSettings = settings;
    currentSamplePos = 0;
    peakLevelL = peakLevelR = 0.0f;
    
    recording = true;
    paused = false;
    
    currentInfo.isRecording = true;
    currentInfo.duration = 0.0;
    currentInfo.samplesCaptured = 0;
    currentInfo.peakLevel = 0.0;
}

void AudioRecorder::stopRecording()
{
    juce::ScopedLock lock(recordLock);
    
    if (!recording) return;
    
    recording = false;
    paused = false;
    
    currentInfo.isRecording = false;
    
    // Trim buffer to actual recorded size
    if (currentSamplePos > 0) {
        juce::AudioBuffer<float> trimmedBuffer(recordBuffer.getNumChannels(), currentSamplePos);
        for (int ch = 0; ch < recordBuffer.getNumChannels(); ++ch) {
            trimmedBuffer.copyFrom(ch, 0, recordBuffer, ch, 0, currentSamplePos);
        }
        recordBuffer = trimmedBuffer;
    }
    
    // Normalize if requested
    if (currentSettings.autoNormalize) {
        float maxLevel = recordBuffer.getMagnitude(0, recordBuffer.getNumSamples());
        if (maxLevel > 0.0f) {
            recordBuffer.applyGain(0.9f / maxLevel);
        }
    }
    
    if (onRecordingComplete) {
        onRecordingComplete(recordBuffer);
    }
}

void AudioRecorder::pauseRecording()
{
    juce::ScopedLock lock(recordLock);
    paused = true;
}

void AudioRecorder::resumeRecording()
{
    juce::ScopedLock lock(recordLock);
    paused = false;
}

void AudioRecorder::clearRecording()
{
    juce::ScopedLock lock(recordLock);
    currentSamplePos = 0;
    recordBuffer.clear();
}

juce::AudioBuffer<float> AudioRecorder::getRecordedAudio() const
{
    juce::ScopedLock lock(recordLock);
    return recordBuffer;
}

bool AudioRecorder::saveToFile(const juce::File& file)
{
    if (file.getFileExtension() == ".wav") {
        return saveToWav(file);
    } else if (file.getFileExtension() == ".flac") {
        return saveToFlac(file);
    }
    return false;
}

bool AudioRecorder::saveToWav(const juce::File& file)
{
    file.deleteFile();
    
    std::unique_ptr<juce::FileOutputStream> outputStream(file.createOutputStream());
    if (!outputStream) return false;
    
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    
    writer.reset(wavFormat.createWriterFor(outputStream.get(),
                                           sampleRate,
                                           recordBuffer.getNumChannels(),
                                           24, // 24-bit
                                           {},
                                           0));
    
    if (!writer) return false;
    
    outputStream.release(); // Writer takes ownership
    
    return writer->writeFromAudioSampleBuffer(recordBuffer, 0, recordBuffer.getNumSamples());
}

bool AudioRecorder::saveToFlac(const juce::File& file)
{
    file.deleteFile();
    
    std::unique_ptr<juce::FileOutputStream> outputStream(file.createOutputStream());
    if (!outputStream) return false;
    
    juce::FlacAudioFormat flacFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    
    writer.reset(flacFormat.createWriterFor(outputStream.get(),
                                            sampleRate,
                                            recordBuffer.getNumChannels(),
                                            24,
                                            {},
                                            5)); // Compression level
    
    if (!writer) return false;
    
    outputStream.release();
    
    return writer->writeFromAudioSampleBuffer(recordBuffer, 0, recordBuffer.getNumSamples());
}

AudioRecorder::RecordingInfo AudioRecorder::getRecordingInfo() const
{
    juce::ScopedLock lock(recordLock);
    return currentInfo;
}

void AudioRecorder::setPunchInOut(double punchIn, double punchOut)
{
    punchInTime = punchIn;
    punchOutTime = punchOut;
}

void AudioRecorder::enablePunchRecording(bool enable)
{
    punchRecordingEnabled = enable;
}

void AudioRecorder::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // Pass through output
    for (int ch = 0; ch < numOutputChannels; ++ch) {
        if (outputChannelData[ch]) {
            std::memset(outputChannelData[ch], 0, sizeof(float) * numSamples);
        }
    }
    
    if (!recording || paused) return;
    
    processInputBuffer(inputChannelData, numInputChannels, numSamples);
}

void AudioRecorder::processInputBuffer(const float* const* inputData, 
                                      int numChannels, 
                                      int numSamples)
{
    juce::ScopedLock lock(recordLock);
    
    // Ensure buffer is large enough
    if (currentSamplePos + numSamples > recordBuffer.getNumSamples()) {
        int newSize = (currentSamplePos + numSamples) * 2; // Double size
        juce::AudioBuffer<float> newBuffer(recordBuffer.getNumChannels(), newSize);
        
        for (int ch = 0; ch < recordBuffer.getNumChannels(); ++ch) {
            newBuffer.copyFrom(ch, 0, recordBuffer, ch, 0, currentSamplePos);
        }
        
        recordBuffer = newBuffer;
    }
    
    // Copy input data
    for (int ch = 0; ch < juce::jmin(numChannels, recordBuffer.getNumChannels()); ++ch) {
        if (inputData[ch]) {
            recordBuffer.copyFrom(ch, currentSamplePos, inputData[ch], numSamples);
            
            // Update peak levels
            float peak = juce::FloatVectorOperations::findMaximum(inputData[ch], numSamples);
            if (ch == 0) peakLevelL = juce::jmax(peakLevelL, peak);
            if (ch == 1) peakLevelR = juce::jmax(peakLevelR, peak);
        }
    }
    
    currentSamplePos += numSamples;
    
    updateRecordingInfo();
    
    if (onRecordingUpdate) {
        onRecordingUpdate(currentInfo);
    }
}

void AudioRecorder::updateRecordingInfo()
{
    currentInfo.samplesCaptured = currentSamplePos;
    currentInfo.duration = currentSamplePos / sampleRate;
    currentInfo.peakLevel = juce::jmax(peakLevelL, peakLevelR);
}

void AudioRecorder::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device) {
        sampleRate = device->getCurrentSampleRate();
    }
}

void AudioRecorder::audioDeviceStopped()
{
    stopRecording();
}

} // namespace Audio
} // namespace OmegaStudio
