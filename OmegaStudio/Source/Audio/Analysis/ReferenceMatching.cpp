/*
  ==============================================================================

    ReferenceMatching.cpp
    Implementation of reference track matching engine

  ==============================================================================
*/

#include "ReferenceMatching.h"
#include <cmath>
#include <algorithm>

namespace omega {
namespace Analysis {

ReferenceTrackMatcher::ReferenceTrackMatcher() 
    : fft_(12), window_(4096, juce::dsp::WindowingFunction<float>::hann),
      windowType_(juce::dsp::WindowingFunction<float>::hann) {
    initialize(48000.0, 4096);
}

void ReferenceTrackMatcher::initialize(double sampleRate, int fftSize) {
    sampleRate_ = sampleRate;
    setFFTSize(fftSize);
}

void ReferenceTrackMatcher::setSampleRate(double newSampleRate) {
    sampleRate_ = newSampleRate;
    if (hasReference_) {
        analyzeReferenceTrack();
    }
}

void ReferenceTrackMatcher::setFFTSize(int size) {
    fftSize_ = size;
    int order = static_cast<int>(std::log2(size));
    fft_ = juce::dsp::FFT(order);
    window_.~WindowingFunction();
    new (&window_) juce::dsp::WindowingFunction<float>(size, windowType_);
    fftData_.resize(size * 2, 0.0f);
    
    referenceSpectrum_ = SpectrumData(fftSize_, sampleRate_);
}

void ReferenceTrackMatcher::loadReferenceTrack(const juce::File& audioFile) {
    if (!audioFile.existsAsFile()) return;
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto* reader = formatManager.createReaderFor(audioFile);
    if (reader == nullptr) return;
    
    referenceBuffer_.setSize(static_cast<int>(reader->numChannels), 
                            static_cast<int>(reader->lengthInSamples));
    reader->read(&referenceBuffer_, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    
    delete reader;
    
    hasReference_ = true;
    analyzeReferenceTrack();
}

void ReferenceTrackMatcher::setReferenceBuffer(const juce::AudioBuffer<float>& buffer) {
    referenceBuffer_ = buffer;
    hasReference_ = true;
    analyzeReferenceTrack();
}

void ReferenceTrackMatcher::clearReference() {
    referenceBuffer_.setSize(0, 0);
    hasReference_ = false;
    referenceSpectrum_.clear();
}

void ReferenceTrackMatcher::analyzeReferenceTrack() {
    if (!hasReference_ || referenceBuffer_.getNumSamples() == 0) return;
    
    // Analyze spectrum (use middle section of track)
    int startSample = referenceBuffer_.getNumSamples() / 2;
    int numSamples = std::min(fftSize_, referenceBuffer_.getNumSamples() - startSample);
    referenceSpectrum_ = analyzeSpectrum(referenceBuffer_, startSample, numSamples);
    
    // Analyze loudness (full track)
    referenceLoudness_ = analyzeLoudness(referenceBuffer_);
}

SpectrumData ReferenceTrackMatcher::analyzeSpectrum(const juce::AudioBuffer<float>& buffer, 
                                                    int startSample, int numSamples) {
    SpectrumData result(fftSize_, sampleRate_);
    
    if (numSamples < 0) {
        numSamples = buffer.getNumSamples() - startSample;
    }
    
    numSamples = std::min(numSamples, fftSize_);
    
    performFFT(buffer, result, startSample, numSamples);
    
    return result;
}

void ReferenceTrackMatcher::performFFT(const juce::AudioBuffer<float>& buffer, SpectrumData& result,
                                      int startSample, int numSamples) {
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    
    // Mix to mono and copy to FFT buffer
    const int numChannels = buffer.getNumChannels();
    for (int i = 0; i < numSamples && i < fftSize_; ++i) {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            sample += buffer.getSample(ch, startSample + i);
        }
        fftData_[i] = sample / static_cast<float>(numChannels);
    }
    
    // Apply window
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize_);
    
    // Perform FFT
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Convert to dB magnitudes
    const int halfSize = fftSize_ / 2;
    for (int i = 0; i < halfSize; ++i) {
        float magnitude = fftData_[i];
        float dB = magnitude > 0.0f ? 20.0f * std::log10(magnitude) : -100.0f;
        
        // Apply smoothing if this is not the first analysis
        if (result.magnitudes[i] > -99.0f) {
            result.magnitudes[i] = smoothingFactor_ * result.magnitudes[i] + (1.0f - smoothingFactor_) * dB;
        } else {
            result.magnitudes[i] = dB;
        }
    }
}

LoudnessData ReferenceTrackMatcher::analyzeLoudness(const juce::AudioBuffer<float>& buffer) {
    LoudnessData result;
    
    result.integratedLUFS = calculateLUFS(buffer);
    result.truePeak = calculateTruePeak(buffer);
    
    // Simplified loudness range calculation
    result.loudnessRange = 10.0f; // TODO: Implement proper LRA
    
    // Short-term and momentary use same as integrated for now
    result.shortTermLUFS = result.integratedLUFS;
    result.momentaryLUFS = result.integratedLUFS;
    
    return result;
}

float ReferenceTrackMatcher::calculateLUFS(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumSamples() == 0) return -70.0f;
    
    // Simplified LUFS calculation (ITU-R BS.1770)
    // Real implementation would use K-weighting filter
    
    double sumSquares = 0.0;
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            double sample = data[i];
            sumSquares += sample * sample;
        }
    }
    
    double meanSquare = sumSquares / (numChannels * numSamples);
    double rms = std::sqrt(meanSquare);
    
    // Convert to LUFS (approximately)
    float lufs = -23.0f + 20.0f * std::log10(static_cast<float>(rms) + 1e-10f);
    
    return juce::jlimit(-70.0f, 0.0f, lufs);
}

float ReferenceTrackMatcher::calculateTruePeak(const juce::AudioBuffer<float>& buffer) {
    float maxPeak = 0.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float absSample = std::abs(data[i]);
            if (absSample > maxPeak) {
                maxPeak = absSample;
            }
        }
    }
    
    // Convert to dBTP
    float dBTP = maxPeak > 0.0f ? 20.0f * std::log10(maxPeak) : -100.0f;
    return dBTP;
}

FrequencyMatchResult ReferenceTrackMatcher::compareToReference(const juce::AudioBuffer<float>& buffer) {
    FrequencyMatchResult result;
    
    if (!hasReference_) return result;
    
    // Analyze current buffer
    SpectrumData currentSpectrum = analyzeSpectrum(buffer);
    
    // Compare spectra
    const int numBands = static_cast<int>(referenceSpectrum_.magnitudes.size());
    result.differences.resize(numBands);
    result.targetEQ.resize(numBands);
    
    double sumSquaredDiff = 0.0;
    for (int i = 0; i < numBands; ++i) {
        float diff = referenceSpectrum_.magnitudes[i] - currentSpectrum.magnitudes[i];
        result.differences[i] = diff;
        result.targetEQ[i] = diff * 0.5f; // Suggest 50% correction
        
        sumSquaredDiff += diff * diff;
    }
    
    result.overallDifference = std::sqrt(sumSquaredDiff / numBands);
    result.isGoodMatch = result.overallDifference < 3.0f;
    
    return result;
}

// ============================================================================
// ReferenceMatchingProcessor Implementation
// ============================================================================

ReferenceMatchingProcessor::ReferenceMatchingProcessor() {
    initialize(48000.0);
}

void ReferenceMatchingProcessor::initialize(double sampleRate) {
    matcher_.initialize(sampleRate);
    currentSpectrum_ = SpectrumData(4096, sampleRate);
    
    // Initialize matching EQs
    for (auto& eq : matchingEQs_) {
        eq.reset();
    }
}

void ReferenceMatchingProcessor::setReferenceFile(const juce::File& file) {
    matcher_.loadReferenceTrack(file);
    
    if (matcher_.hasReference()) {
        targetLUFS_ = matcher_.getReferenceLoudness().integratedLUFS;
    }
}

void ReferenceMatchingProcessor::process(juce::AudioBuffer<float>& buffer) {
    // Periodic analysis
    analysisSampleCounter_ += buffer.getNumSamples();
    if (analysisSampleCounter_ >= analysisSampleInterval_) {
        analysisSampleCounter_ = 0;
        
        currentSpectrum_ = matcher_.analyzeSpectrum(buffer);
        currentLoudness_ = matcher_.analyzeLoudness(buffer);
        
        if (matcher_.hasReference()) {
            matchResult_ = matcher_.compareToReference(buffer);
        }
    }
    
    // Apply auto-matching if enabled
    if (autoMatchEnabled_ && matcher_.hasReference()) {
        applyAutoMatch(buffer);
    }
    
    // Apply LUFS matching if enabled
    if (lufsMatchEnabled_) {
        applyLUFSMatch(buffer);
    }
}

void ReferenceMatchingProcessor::applyAutoMatch(juce::AudioBuffer<float>& buffer) {
    // Apply EQ based on match result
    // Simplified implementation - real version would use proper filter design
    
    if (matchResult_.targetEQ.empty()) return;
    
    // Apply gain adjustment based on average difference
    float avgDiff = 0.0f;
    for (float diff : matchResult_.targetEQ) {
        avgDiff += diff;
    }
    avgDiff /= matchResult_.targetEQ.size();
    
    float gain = std::pow(10.0f, avgDiff * autoMatchAmount_ / 20.0f);
    buffer.applyGain(gain);
}

void ReferenceMatchingProcessor::applyLUFSMatch(juce::AudioBuffer<float>& buffer) {
    float currentLUFS = currentLoudness_.integratedLUFS;
    float diff = targetLUFS_ - currentLUFS;
    
    // Smooth gain adjustment
    float targetGain = std::pow(10.0f, diff / 20.0f);
    currentGain_ = 0.95f * currentGain_ + 0.05f * targetGain;
    
    buffer.applyGain(currentGain_);
}

// ============================================================================
// ReferenceLibrary Implementation
// ============================================================================

void ReferenceLibrary::addReference(const ReferenceTrack& ref) {
    references_.push_back(ref);
}

void ReferenceLibrary::removeReference(int index) {
    if (index >= 0 && index < static_cast<int>(references_.size())) {
        references_.erase(references_.begin() + index);
    }
}

void ReferenceLibrary::clear() {
    references_.clear();
}

std::vector<int> ReferenceLibrary::findByGenre(const juce::String& genre) const {
    std::vector<int> results;
    for (size_t i = 0; i < references_.size(); ++i) {
        if (references_[i].genre.equalsIgnoreCase(genre)) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

std::vector<int> ReferenceLibrary::findByArtist(const juce::String& artist) const {
    std::vector<int> results;
    for (size_t i = 0; i < references_.size(); ++i) {
        if (references_[i].artist.containsIgnoreCase(artist)) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

std::vector<int> ReferenceLibrary::findByLoudness(float minLUFS, float maxLUFS) const {
    std::vector<int> results;
    for (size_t i = 0; i < references_.size(); ++i) {
        float lufs = references_[i].integratedLUFS;
        if (lufs >= minLUFS && lufs <= maxLUFS) {
            results.push_back(static_cast<int>(i));
        }
    }
    return results;
}

void ReferenceLibrary::saveToFile(const juce::File& file) {
    juce::XmlElement root("ReferenceLibrary");
    
    for (const auto& ref : references_) {
        auto* refElement = root.createNewChildElement("Reference");
        refElement->setAttribute("name", ref.name);
        refElement->setAttribute("artist", ref.artist);
        refElement->setAttribute("genre", ref.genre);
        refElement->setAttribute("file", ref.file.getFullPathName());
        refElement->setAttribute("lufs", ref.integratedLUFS);
        refElement->setAttribute("truePeak", ref.truePeak);
        refElement->setAttribute("notes", ref.notes);
    }
    
    root.writeTo(file);
}

void ReferenceLibrary::loadFromFile(const juce::File& file) {
    auto xml = juce::parseXML(file);
    if (xml == nullptr) return;
    
    references_.clear();
    
    for (auto* refElement : xml->getChildIterator()) {
        if (refElement->hasTagName("Reference")) {
            ReferenceTrack ref;
            ref.name = refElement->getStringAttribute("name");
            ref.artist = refElement->getStringAttribute("artist");
            ref.genre = refElement->getStringAttribute("genre");
            ref.file = juce::File(refElement->getStringAttribute("file"));
            ref.integratedLUFS = static_cast<float>(refElement->getDoubleAttribute("lufs", -14.0));
            ref.truePeak = static_cast<float>(refElement->getDoubleAttribute("truePeak", -1.0));
            ref.notes = refElement->getStringAttribute("notes");
            
            references_.push_back(ref);
        }
    }
}

} // namespace Analysis
} // namespace omega
