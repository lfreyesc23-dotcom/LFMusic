#include "SpectralAnalyzer.h"

namespace OmegaStudio {
// Stub implementations

FFTAnalyzer::FFTAnalyzer() { 
    magnitudeSpectrum.resize(numBins, 0.0f); 
    smoothedSpectrum.resize(numBins, 0.0f); 
    fifo.resize(fftSize, 0.0f); 
}
FFTAnalyzer::~FFTAnalyzer() = default;
void FFTAnalyzer::prepareToPlay(double sr) { sampleRate = sr; }
void FFTAnalyzer::pushSamples(const juce::AudioBuffer<float>& buffer) { for (int i = 0; i < buffer.getNumSamples(); ++i) { fifo[fifoIndex++] = buffer.getSample(0, i); if (fifoIndex >= fftSize) { fifoIndex = 0; performFFT(); newDataAvailable = true; } } }
void FFTAnalyzer::setWindowType(juce::dsp::WindowingFunction<float>::WindowingMethod method) { windowType = method; }
void FFTAnalyzer::setAveraging(float amount) { averaging = juce::jlimit(0.0f, 1.0f, amount); }
float FFTAnalyzer::getFrequencyForBin(int bin) const { return bin * static_cast<float>(sampleRate / fftSize); }
int FFTAnalyzer::getBinForFrequency(float frequency) const { return static_cast<int>(frequency / (sampleRate / fftSize)); }
void FFTAnalyzer::performFFT() { 
    for (int i = 0; i < fftSize; ++i) 
        fftData[i] = fifo[i]; 
    
    // Create window on demand
    juce::dsp::WindowingFunction<float> window(fftSize, windowType);
    window.multiplyWithWindowingTable(fftData.data(), fftSize); 
    
    fft.performFrequencyOnlyForwardTransform(fftData.data()); 
    
    for (int i = 0; i < numBins; ++i) { 
        float magnitude = fftData[i]; 
        float db = magnitude > 0.0f ? 20.0f * std::log10(magnitude) : minDecibels; 
        magnitudeSpectrum[i] = db; 
    } 
    
    smoothSpectrum(); 
}
void FFTAnalyzer::smoothSpectrum() { for (int i = 0; i < numBins; ++i) smoothedSpectrum[i] = smoothedSpectrum[i] * averaging + magnitudeSpectrum[i] * (1.0f - averaging); }

Spectrogram::Spectrogram(int w, int h) : width(w), height(h) { spectrogramImage = juce::Image(juce::Image::RGB, width, height, true); createDefaultColourMap(); }
void Spectrogram::prepareToPlay(double) {}
void Spectrogram::pushSpectrum(const std::vector<float>& spectrum) { spectrogramHistory.push_back(spectrum); if (spectrogramHistory.size() > static_cast<size_t>(width)) spectrogramHistory.pop_front(); updateImage(); }
void Spectrogram::setSize(int w, int h) { width = w; height = h; spectrogramImage = juce::Image(juce::Image::RGB, width, height, true); }
void Spectrogram::setColourMap(const std::vector<juce::Colour>& colours) { colourMap = colours; }
void Spectrogram::updateImage() {}
juce::Colour Spectrogram::getColourForMagnitude(float magnitude) const { int index = static_cast<int>((magnitude + 100.0f) / 100.0f * colourMap.size()); return colourMap[juce::jlimit(0, static_cast<int>(colourMap.size()) - 1, index)]; }
void Spectrogram::createDefaultColourMap() { colourMap = {juce::Colours::black, juce::Colours::blue, juce::Colours::cyan, juce::Colours::green, juce::Colours::yellow, juce::Colours::red, juce::Colours::white}; }

CorrelationMeter::CorrelationMeter() {}
void CorrelationMeter::prepareToPlay(double sr, int) { sampleRate = sr; reset(); }
void CorrelationMeter::process(const juce::AudioBuffer<float>& buffer) { if (buffer.getNumChannels() < 2) return; for (int i = 0; i < buffer.getNumSamples(); ++i) { float left = buffer.getSample(0, i); float right = buffer.getSample(1, i); sumLeft += left * left; sumRight += right * right; sumProduct += left * right; sampleCount++; } if (sampleCount >= static_cast<int>(sampleRate * integrationTime)) { float denom = std::sqrt(sumLeft * sumRight); correlation = (denom > 0.0f) ? (sumProduct / denom) : 0.0f; reset(); } }
void CorrelationMeter::reset() { sumLeft = 0.0f; sumRight = 0.0f; sumProduct = 0.0f; sampleCount = 0; }
void CorrelationMeter::setIntegrationTime(float seconds) { integrationTime = seconds; }

LUFSMeter::LUFSMeter() : oversampling(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR) {}
void LUFSMeter::prepareToPlay(double sr, int blockSize) { sampleRate = sr; juce::dsp::ProcessSpec spec{sr, static_cast<juce::uint32>(blockSize), 2}; oversampling.initProcessing(blockSize); updateFilters(); }
void LUFSMeter::process(const juce::AudioBuffer<float>&) {}
void LUFSMeter::reset() { momentaryLUFS = shortTermLUFS = integratedLUFS = -70.0f; lra = 0.0f; truePeakLeft = truePeakRight = -70.0f; }
void LUFSMeter::updateFilters() {}
float LUFSMeter::calculateLoudness(const std::deque<float>&) const { return -23.0f; }
void LUFSMeter::applyGating() {}

Vectorscope::Vectorscope(int) {}
void Vectorscope::prepareToPlay(double sr) { sampleRate = sr; }
void Vectorscope::process(const juce::AudioBuffer<float>& buffer) { if (buffer.getNumChannels() < 2) return; points.clear(); for (int i = 0; i < buffer.getNumSamples(); i += 4) { float left = buffer.getSample(0, i); float right = buffer.getSample(1, i); points.push_back({(left + right) * 0.5f * scale, (left - right) * 0.5f * scale, 1.0f}); } }
void Vectorscope::reset() { points.clear(); }
void Vectorscope::setPersistence(float seconds) { persistence = seconds; }
void Vectorscope::setRotation(float degrees) { rotation = degrees; }
void Vectorscope::setScale(float s) { scale = s; }

AudioAnalyzer::AudioAnalyzer() = default;
AudioAnalyzer::~AudioAnalyzer() = default;
void AudioAnalyzer::prepareToPlay(double sampleRate, int blockSize) { fftAnalyzer.prepareToPlay(sampleRate); spectrogram.prepareToPlay(sampleRate); correlationMeter.prepareToPlay(sampleRate, blockSize); lufsMeter.prepareToPlay(sampleRate, blockSize); vectorscope.prepareToPlay(sampleRate); }
void AudioAnalyzer::process(const juce::AudioBuffer<float>& buffer) { if (fftEnabled) fftAnalyzer.pushSamples(buffer); if (spectrogramEnabled && fftAnalyzer.hasNewData()) spectrogram.pushSpectrum(fftAnalyzer.getMagnitudeSpectrum()); if (correlationEnabled) correlationMeter.process(buffer); if (lufsEnabled) lufsMeter.process(buffer); if (vectorscopeEnabled) vectorscope.process(buffer); }
void AudioAnalyzer::reset() { correlationMeter.reset(); lufsMeter.reset(); vectorscope.reset(); }

} // namespace OmegaStudio
