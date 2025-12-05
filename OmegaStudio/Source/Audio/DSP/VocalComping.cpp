/**
 * @file VocalComping.cpp
 * @brief Implementation of vocal comping system
 */

#include "VocalComping.h"
#include <algorithm>
#include <cmath>

namespace omega {

//==============================================================================
// VocalCompingEngine Implementation
//==============================================================================

VocalCompingEngine::VocalCompingEngine() {
}

void VocalCompingEngine::initialize(double sampleRate) {
    m_sampleRate = sampleRate;
    m_tempBuffer.setSize(2, static_cast<int>(sampleRate * 10)); // 10 second buffer
}

int VocalCompingEngine::addTake(const juce::AudioBuffer<float>& takeData, const juce::String& takeName) {
    auto take = std::make_unique<VocalTake>();
    take->name = takeName;
    take->audioData = takeData;
    take->sampleRate = m_sampleRate;
    
    // Assign color based on index
    const juce::Colour colors[] = {
        juce::Colours::blue, juce::Colours::green, juce::Colours::red,
        juce::Colours::purple, juce::Colours::orange, juce::Colours::cyan
    };
    take->color = colors[m_takes.size() % 6];
    
    // Analyze quality
    int takeIndex = static_cast<int>(m_takes.size());
    m_takes.push_back(std::move(take));
    analyzeTakeQuality(takeIndex);
    
    return takeIndex;
}

void VocalCompingEngine::removeTake(int takeIndex) {
    if (takeIndex >= 0 && takeIndex < static_cast<int>(m_takes.size())) {
        m_takes.erase(m_takes.begin() + takeIndex);
        
        // Remove segments referencing this take
        m_segments.erase(
            std::remove_if(m_segments.begin(), m_segments.end(),
                [takeIndex](const CompSegment& seg) { return seg.takeIndex == takeIndex; }),
            m_segments.end()
        );
    }
}

VocalTake* VocalCompingEngine::getTake(int takeIndex) {
    if (takeIndex >= 0 && takeIndex < static_cast<int>(m_takes.size()))
        return m_takes[takeIndex].get();
    return nullptr;
}

const VocalTake* VocalCompingEngine::getTake(int takeIndex) const {
    if (takeIndex >= 0 && takeIndex < static_cast<int>(m_takes.size()))
        return m_takes[takeIndex].get();
    return nullptr;
}

void VocalCompingEngine::addSegment(const CompSegment& segment) {
    if (segment.isValid() && segment.takeIndex < static_cast<int>(m_takes.size())) {
        m_segments.push_back(segment);
        
        // Sort segments by start time
        std::sort(m_segments.begin(), m_segments.end(),
            [](const CompSegment& a, const CompSegment& b) {
                return a.startTime < b.startTime;
            });
    }
}

void VocalCompingEngine::removeSegment(int segmentIndex) {
    if (segmentIndex >= 0 && segmentIndex < static_cast<int>(m_segments.size())) {
        m_segments.erase(m_segments.begin() + segmentIndex);
    }
}

CompSegment* VocalCompingEngine::getSegment(int segmentIndex) {
    if (segmentIndex >= 0 && segmentIndex < static_cast<int>(m_segments.size()))
        return &m_segments[segmentIndex];
    return nullptr;
}

const CompSegment* VocalCompingEngine::getSegment(int segmentIndex) const {
    if (segmentIndex >= 0 && segmentIndex < static_cast<int>(m_segments.size()))
        return &m_segments[segmentIndex];
    return nullptr;
}

void VocalCompingEngine::clearSegments() {
    m_segments.clear();
}

void VocalCompingEngine::compileToBuffer(juce::AudioBuffer<float>& outputBuffer, 
                                        int startSample, 
                                        int numSamples) {
    const double startTime = startSample / m_sampleRate;
    const double endTime = (startSample + numSamples) / m_sampleRate;
    
    // Clear output region
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
        outputBuffer.clear(ch, startSample, numSamples);
    }
    
    // Process each segment that overlaps with the requested range
    for (const auto& segment : m_segments) {
        if (segment.endTime < startTime || segment.startTime > endTime)
            continue;  // Segment doesn't overlap
        
        const VocalTake* take = getTake(segment.takeIndex);
        if (!take || take->isMuted)
            continue;
        
        // Calculate segment sample range
        int segmentStartSample = static_cast<int>((segment.startTime - startTime) * m_sampleRate);
        int segmentEndSample = static_cast<int>((segment.endTime - startTime) * m_sampleRate);
        
        segmentStartSample = juce::jmax(0, segmentStartSample);
        segmentEndSample = juce::jmin(numSamples, segmentEndSample);
        
        if (segmentStartSample >= segmentEndSample)
            continue;
        
        // Calculate source sample position in take
        int takeStartSample = static_cast<int>(segment.startTime * take->sampleRate);
        int segmentLength = segmentEndSample - segmentStartSample;
        
        // Copy audio from take to output with crossfades
        for (int ch = 0; ch < juce::jmin(outputBuffer.getNumChannels(), 
                                         take->audioData.getNumChannels()); ++ch) {
            const float* srcData = take->audioData.getReadPointer(ch, takeStartSample);
            float* dstData = outputBuffer.getWritePointer(ch, startSample + segmentStartSample);
            
            // Calculate crossfade sample counts
            int fadeInSamples = static_cast<int>(segment.crossfadeIn * m_sampleRate);
            int fadeOutSamples = static_cast<int>(segment.crossfadeOut * m_sampleRate);
            
            for (int i = 0; i < segmentLength; ++i) {
                if (takeStartSample + i >= take->audioData.getNumSamples())
                    break;
                
                float gain = take->volume;
                
                // Apply fade in
                if (i < fadeInSamples) {
                    float fadePos = static_cast<float>(i) / fadeInSamples;
                    gain *= calculateCrossfadeGain(fadePos, segment.fadeType, true);
                }
                
                // Apply fade out
                if (i > segmentLength - fadeOutSamples) {
                    float fadePos = static_cast<float>(segmentLength - i) / fadeOutSamples;
                    gain *= calculateCrossfadeGain(fadePos, segment.fadeType, true);
                }
                
                dstData[i] += srcData[i] * gain;
            }
        }
    }
}

juce::AudioBuffer<float> VocalCompingEngine::exportCompiledComp() {
    if (m_segments.empty() || m_takes.empty()) {
        return juce::AudioBuffer<float>(2, 0);
    }
    
    // Find total duration
    double maxEndTime = 0.0;
    for (const auto& segment : m_segments) {
        maxEndTime = juce::jmax(maxEndTime, segment.endTime);
    }
    
    int totalSamples = static_cast<int>(maxEndTime * m_sampleRate);
    juce::AudioBuffer<float> result(2, totalSamples);
    result.clear();
    
    compileToBuffer(result, 0, totalSamples);
    
    return result;
}

void VocalCompingEngine::analyzeTakeQuality(int takeIndex) {
    VocalTake* take = getTake(takeIndex);
    if (!take || take->audioData.getNumSamples() == 0)
        return;
    
    const float* data = take->audioData.getReadPointer(0);
    int numSamples = take->audioData.getNumSamples();
    
    // Calculate RMS
    take->averageRMS = calculateRMS(data, numSamples);
    
    // Analyze pitch stability (simplified)
    take->pitchStability = analyzePitchStability(data, numSamples);
    
    // Analyze timing accuracy (simplified)
    take->timingAccuracy = analyzeTimingAccuracy(data, numSamples);
    
    // Calculate overall score (weighted average)
    take->overallScore = (take->averageRMS * 0.3f + 
                         take->pitchStability * 0.5f + 
                         take->timingAccuracy * 0.2f);
}

void VocalCompingEngine::autoSelectBestSegments(double segmentDuration) {
    if (m_takes.empty())
        return;
    
    clearSegments();
    
    // Find longest take
    double maxDuration = 0.0;
    for (const auto& take : m_takes) {
        maxDuration = juce::jmax(maxDuration, take->getDurationSeconds());
    }
    
    // Create segments
    double currentTime = 0.0;
    while (currentTime < maxDuration) {
        double endTime = juce::jmin(currentTime + segmentDuration, maxDuration);
        
        // Find best take for this time segment
        int bestTakeIndex = 0;
        float bestScore = -1.0f;
        
        for (int i = 0; i < static_cast<int>(m_takes.size()); ++i) {
            const auto& take = m_takes[i];
            if (take->getDurationSeconds() > currentTime) {
                float score = take->overallScore;
                if (score > bestScore) {
                    bestScore = score;
                    bestTakeIndex = i;
                }
            }
        }
        
        // Add segment
        CompSegment segment;
        segment.takeIndex = bestTakeIndex;
        segment.startTime = currentTime;
        segment.endTime = endTime;
        segment.crossfadeIn = m_defaultCrossfade;
        segment.crossfadeOut = m_defaultCrossfade;
        segment.fadeType = m_defaultFadeType;
        
        addSegment(segment);
        
        currentTime = endTime;
    }
}

void VocalCompingEngine::reset() {
    m_takes.clear();
    m_segments.clear();
}

float VocalCompingEngine::calculateCrossfadeGain(float position, 
                                                 CompSegment::CrossfadeType type, 
                                                 bool fadeIn) {
    position = juce::jlimit(0.0f, 1.0f, position);
    
    if (!fadeIn)
        position = 1.0f - position;
    
    switch (type) {
        case CompSegment::CrossfadeType::Linear:
            return position;
            
        case CompSegment::CrossfadeType::EqualPower:
            return std::sin(position * juce::MathConstants<float>::halfPi);
            
        case CompSegment::CrossfadeType::Logarithmic:
            return std::exp(position * 2.0f - 2.0f);
            
        case CompSegment::CrossfadeType::SCurve:
            return position * position * (3.0f - 2.0f * position);
            
        default:
            return position;
    }
}

float VocalCompingEngine::calculateRMS(const float* buffer, int numSamples) {
    if (numSamples == 0)
        return 0.0f;
    
    double sum = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        sum += buffer[i] * buffer[i];
    }
    
    return std::sqrt(static_cast<float>(sum / numSamples));
}

float VocalCompingEngine::analyzePitchStability(const float* buffer, int numSamples) {
    // Simplified pitch stability analysis
    // In production, use autocorrelation or YIN algorithm
    return 0.75f; // Placeholder
}

float VocalCompingEngine::analyzeTimingAccuracy(const float* buffer, int numSamples) {
    // Simplified timing analysis
    return 0.80f; // Placeholder
}

//==============================================================================
// VocalCompingRecorder Implementation
//==============================================================================

VocalCompingRecorder::VocalCompingRecorder() {
}

void VocalCompingRecorder::initialize(double sampleRate, double maxLengthSeconds) {
    m_sampleRate = sampleRate;
    m_maxSamples = static_cast<int>(sampleRate * maxLengthSeconds);
    m_recordBuffer.setSize(2, m_maxSamples);
    reset();
}

bool VocalCompingRecorder::startRecording(const juce::String& takeName) {
    if (m_isRecording)
        return false;
    
    m_currentTakeName = takeName;
    m_writePosition = 0;
    m_recordBuffer.clear();
    m_isRecording = true;
    
    return true;
}

void VocalCompingRecorder::stopRecording() {
    m_isRecording = false;
}

void VocalCompingRecorder::processAudioInput(const float* inputBuffer, int numSamples) {
    if (!m_isRecording)
        return;
    
    int samplesToWrite = juce::jmin(numSamples, m_maxSamples - m_writePosition);
    
    if (samplesToWrite > 0) {
        // Write to both channels (mono to stereo)
        for (int ch = 0; ch < 2; ++ch) {
            float* dst = m_recordBuffer.getWritePointer(ch, m_writePosition);
            for (int i = 0; i < samplesToWrite; ++i) {
                dst[i] = inputBuffer[i];
            }
        }
        
        m_writePosition += samplesToWrite;
        
        // Auto-stop if buffer full
        if (m_writePosition >= m_maxSamples) {
            m_isRecording = false;
        }
    }
}

void VocalCompingRecorder::reset() {
    m_isRecording = false;
    m_writePosition = 0;
    m_currentTakeName.clear();
}

//==============================================================================
// VocalCompingManager Implementation
//==============================================================================

VocalCompingManager::VocalCompingManager() {
}

void VocalCompingManager::initialize(double sampleRate) {
    m_sampleRate = sampleRate;
    m_engine.initialize(sampleRate);
    m_recorder.initialize(sampleRate);
}

bool VocalCompingManager::startNewTake(const juce::String& takeName) {
    return m_recorder.startRecording(takeName);
}

void VocalCompingManager::stopAndCommitTake() {
    if (!m_recorder.isRecording())
        return;
    
    m_recorder.stopRecording();
    
    // Get recorded buffer and add to engine
    const auto& recordedBuffer = m_recorder.getCurrentTakeBuffer();
    juce::String takeName = m_recorder.getCurrentTakeName();
    
    // Trim silence from end
    int actualLength = recordedBuffer.getNumSamples();
    // Could add silence detection here
    
    juce::AudioBuffer<float> trimmedBuffer(2, actualLength);
    for (int ch = 0; ch < 2; ++ch) {
        trimmedBuffer.copyFrom(ch, 0, recordedBuffer, ch, 0, actualLength);
    }
    
    m_engine.addTake(trimmedBuffer, takeName);
}

void VocalCompingManager::processRecordingInput(const float* inputBuffer, int numSamples) {
    m_recorder.processAudioInput(inputBuffer, numSamples);
}

bool VocalCompingManager::saveCompSession(const juce::File& file) {
    // Implementation would serialize takes and segments to JSON/XML
    return true; // Placeholder
}

bool VocalCompingManager::loadCompSession(const juce::File& file) {
    // Implementation would deserialize from file
    return true; // Placeholder
}

bool VocalCompingManager::exportCompiledAudio(const juce::File& outputFile) {
    auto compiledBuffer = m_engine.exportCompiledComp();
    
    if (compiledBuffer.getNumSamples() == 0)
        return false;
    
    // Create WAV file
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset(wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        m_sampleRate,
        compiledBuffer.getNumChannels(),
        24,  // 24-bit
        {},
        0
    ));
    
    if (writer != nullptr) {
        writer->writeFromAudioSampleBuffer(compiledBuffer, 0, compiledBuffer.getNumSamples());
        return true;
    }
    
    return false;
}

} // namespace omega
