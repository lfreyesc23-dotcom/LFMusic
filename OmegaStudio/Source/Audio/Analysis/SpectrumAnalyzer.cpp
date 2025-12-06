//==============================================================================
// SpectrumAnalyzer.cpp - Real-Time FFT Spectrum Analyzer Implementation
// FL Studio Killer - Professional DAW
//==============================================================================

#include "SpectrumAnalyzer.h"

namespace OmegaStudio {

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer()
{
    magnitudeSpectrum_.resize(settings_.numBands, 0.0f);
    peakSpectrum_.resize(settings_.numBands, -100.0f);
    frequencies_.resize(settings_.numBands);
    peakHoldTimes_.resize(settings_.numBands, 0.0f);
    
    // Initialize sonogram
    sonogramData_.resize(sonogramHistory);
    for (auto& row : sonogramData_)
        row.resize(settings_.numBands, 0.0f);
}

void SpectrumAnalyzer::prepare(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    
    const int fftSize = 1 << settings_.fftOrder;
    fifo_.setSize(2, fftSize);
    fifo_.clear();
    fifoPos_ = 0;
    
    reset();
}

void SpectrumAnalyzer::reset()
{
    fifoPos_ = 0;
    fifo_.clear();
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    std::fill(magnitudeSpectrum_.begin(), magnitudeSpectrum_.end(), 0.0f);
    std::fill(peakSpectrum_.begin(), peakSpectrum_.end(), -100.0f);
    std::fill(peakHoldTimes_.begin(), peakHoldTimes_.end(), 0.0f);
    
    for (auto& row : sonogramData_)
        std::fill(row.begin(), row.end(), 0.0f);
}

//==============================================================================
void SpectrumAnalyzer::pushBuffer(const juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0)
        return;
    
    const int fftSize = 1 << settings_.fftOrder;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = 0.0f;
        
        // Channel mode
        switch (settings_.channelMode)
        {
            case ChannelMode::Stereo:
                value = buffer.getSample(0, sample);
                if (numChannels > 1)
                    value = (value + buffer.getSample(1, sample)) * 0.5f;
                break;
                
            case ChannelMode::Left:
                value = buffer.getSample(0, sample);
                break;
                
            case ChannelMode::Right:
                value = numChannels > 1 ? buffer.getSample(1, sample) : buffer.getSample(0, sample);
                break;
                
            case ChannelMode::Mid:
                value = buffer.getSample(0, sample);
                if (numChannels > 1)
                    value = (value + buffer.getSample(1, sample)) * 0.5f;
                break;
                
            case ChannelMode::Side:
                if (numChannels > 1)
                    value = (buffer.getSample(0, sample) - buffer.getSample(1, sample)) * 0.5f;
                break;
        }
        
        fifo_.setSample(0, fifoPos_, value);
        
        if (++fifoPos_ >= fftSize)
        {
            processFFT();
            fifoPos_ = fftSize / 2; // 50% overlap
            
            // Move second half to first half
            for (int i = 0; i < fftSize / 2; ++i)
                fifo_.setSample(0, i, fifo_.getSample(0, i + fftSize / 2));
        }
    }
}

void SpectrumAnalyzer::processFFT()
{
    const int fftSize = 1 << settings_.fftOrder;
    
    // Copy to FFT buffer
    for (int i = 0; i < fftSize; ++i)
        fftData_[i] = fifo_.getSample(0, i);
    
    // Apply window
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize);
    
    // Perform FFT
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Update spectrum
    updateMagnitudeSpectrum();
    updatePeakSpectrum(0.016f); // ~60fps
    updateSonogram();
}

void SpectrumAnalyzer::updateMagnitudeSpectrum()
{
    const float binHz = static_cast<float>(sampleRate_) / (1 << settings_.fftOrder);
    const int numBins = (1 << settings_.fftOrder) / 2;
    
    for (int i = 0; i < settings_.numBands; ++i)
    {
        float frequency = settings_.minFrequency * 
            std::pow(settings_.maxFrequency / settings_.minFrequency, 
                    static_cast<float>(i) / settings_.numBands);
        
        frequencies_[i] = frequency;
        
        int bin = frequencyToBin(frequency);
        bin = juce::jlimit(0, numBins - 1, bin);
        
        float magnitude = fftData_[bin];
        float magnitudeDB = magnitudeToDb(magnitude);
        
        // Smooth
        magnitudeSpectrum_[i] = magnitudeSpectrum_[i] * settings_.smoothing +
                               magnitudeDB * (1.0f - settings_.smoothing);
    }
}

void SpectrumAnalyzer::updatePeakSpectrum(float deltaTime)
{
    for (int i = 0; i < settings_.numBands; ++i)
    {
        if (magnitudeSpectrum_[i] > peakSpectrum_[i])
        {
            peakSpectrum_[i] = magnitudeSpectrum_[i];
            peakHoldTimes_[i] = settings_.peakHold;
        }
        else
        {
            peakHoldTimes_[i] -= deltaTime;
            if (peakHoldTimes_[i] <= 0.0f)
                peakSpectrum_[i] -= settings_.peakDecay * deltaTime;
        }
        
        peakSpectrum_[i] = juce::jmax(peakSpectrum_[i], magnitudeSpectrum_[i]);
    }
}

void SpectrumAnalyzer::updateSonogram()
{
    if (settings_.displayMode != DisplayMode::Sonogram)
        return;
    
    // Shift rows
    for (int row = sonogramData_.size() - 1; row > 0; --row)
        sonogramData_[row] = sonogramData_[row - 1];
    
    // Add new row
    sonogramData_[0] = magnitudeSpectrum_;
}

//==============================================================================
void SpectrumAnalyzer::setSettings(const Settings& settings)
{
    settings_ = settings;
    
    magnitudeSpectrum_.resize(settings_.numBands, 0.0f);
    peakSpectrum_.resize(settings_.numBands, -100.0f);
    frequencies_.resize(settings_.numBands);
    peakHoldTimes_.resize(settings_.numBands, 0.0f);
}

float SpectrumAnalyzer::getMagnitudeAt(float frequency) const
{
    if (frequency < settings_.minFrequency || frequency > settings_.maxFrequency)
        return -100.0f;
    
    // Find nearest band
    int nearestBand = 0;
    float minDistance = std::abs(frequencies_[0] - frequency);
    
    for (size_t i = 1; i < frequencies_.size(); ++i)
    {
        float distance = std::abs(frequencies_[i] - frequency);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestBand = static_cast<int>(i);
        }
    }
    
    return magnitudeSpectrum_[nearestBand];
}

float SpectrumAnalyzer::getPeakAt(float frequency) const
{
    if (frequency < settings_.minFrequency || frequency > settings_.maxFrequency)
        return -100.0f;
    
    int nearestBand = 0;
    float minDistance = std::abs(frequencies_[0] - frequency);
    
    for (size_t i = 1; i < frequencies_.size(); ++i)
    {
        float distance = std::abs(frequencies_[i] - frequency);
        if (distance < minDistance)
        {
            minDistance = distance;
            nearestBand = static_cast<int>(i);
        }
    }
    
    return peakSpectrum_[nearestBand];
}

float SpectrumAnalyzer::getDominantFrequency() const
{
    float maxMagnitude = -100.0f;
    size_t maxIndex = 0;
    
    for (size_t i = 0; i < magnitudeSpectrum_.size(); ++i)
    {
        if (magnitudeSpectrum_[i] > maxMagnitude)
        {
            maxMagnitude = magnitudeSpectrum_[i];
            maxIndex = i;
        }
    }
    
    return frequencies_[maxIndex];
}

//==============================================================================
int SpectrumAnalyzer::frequencyToBin(float frequency) const
{
    const float binHz = static_cast<float>(sampleRate_) / (1 << settings_.fftOrder);
    return static_cast<int>(frequency / binHz);
}

float SpectrumAnalyzer::binToFrequency(int bin) const
{
    const float binHz = static_cast<float>(sampleRate_) / (1 << settings_.fftOrder);
    return bin * binHz;
}

float SpectrumAnalyzer::magnitudeToDb(float magnitude) const
{
    return magnitude > 0.0f ? 
        juce::Decibels::gainToDecibels(magnitude, -100.0f) : -100.0f;
}

} // namespace OmegaStudio
