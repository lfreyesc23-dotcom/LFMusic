//==============================================================================
// LimiterMaximizer.cpp - Implementation (Simplified but Professional)
//==============================================================================

#include "LimiterMaximizer.h"
#include <cmath>

namespace OmegaStudio {

//==============================================================================
// LookAheadBuffer Implementation
//==============================================================================

void LimiterMaximizer::LookAheadBuffer::prepare(int maxSamples) {
    buffer_.resize(maxSamples, 0.0f);
    delay_ = maxSamples;
    writePos_ = 0;
    readPos_ = 0;
}

void LimiterMaximizer::LookAheadBuffer::write(float sample) {
    buffer_[writePos_] = sample;
}

float LimiterMaximizer::LookAheadBuffer::read() const {
    return buffer_[readPos_];
}

void LimiterMaximizer::LookAheadBuffer::advance() {
    writePos_ = (writePos_ + 1) % buffer_.size();
    readPos_ = (readPos_ + 1) % buffer_.size();
}

void LimiterMaximizer::LookAheadBuffer::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writePos_ = 0;
    readPos_ = 0;
}

//==============================================================================
// LimiterMaximizer Implementation
//==============================================================================

LimiterMaximizer::LimiterMaximizer() {
    resetMetering();
}

void LimiterMaximizer::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    
    // Preparar look-ahead buffers
    const int lookAheadSamples = static_cast<int>(settings_.lookAhead * 0.001 * sampleRate);
    lookAheadBuffers_.resize(2);  // Estéreo
    for (auto& buffer : lookAheadBuffers_) {
        buffer.prepare(lookAheadSamples);
    }
    
    // Calcular coeficiente de release
    const float releaseTime = settings_.release * 0.001f;
    releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate) * releaseTime));
    
    reset();
}

void LimiterMaximizer::reset() {
    for (auto& buffer : lookAheadBuffers_) {
        buffer.reset();
    }
    envelopeFollower_ = 0.0f;
    resetMetering();
}

void LimiterMaximizer::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Input gain
    if (settings_.inputGain != 0.0f) {
        const float inputGainLinear = std::pow(10.0f, settings_.inputGain / 20.0f);
        buffer.applyGain(inputGainLinear);
    }
    
    // Auto gain (maximizar al ceiling)
    if (settings_.autoGain) {
        // Analizar nivel actual
        float currentPeak = buffer.getMagnitude(0, numSamples);
        if (currentPeak > 0.00001f) {
            const float currentDb = 20.0f * std::log10(currentPeak);
            const float targetDb = settings_.ceiling - 0.5f; // Dejar headroom
            const float autoGainDb = targetDb - currentDb;
            const float autoGainLinear = std::pow(10.0f, autoGainDb / 20.0f);
            buffer.applyGain(autoGainLinear);
        }
    }
    
    // Look-ahead processing
    const float ceilingLinear = std::pow(10.0f, settings_.ceiling / 20.0f);
    const float thresholdLinear = std::pow(10.0f, settings_.threshold / 20.0f);
    
    for (int i = 0; i < numSamples; ++i) {
        // Analizar peak de todos los canales
        float peakSample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            const float sample = buffer.getSample(ch, i);
            peakSample = std::max(peakSample, std::abs(sample));
        }
        
        // Calcular gain reduction necesaria
        float targetGain = 1.0f;
        if (peakSample > thresholdLinear) {
            targetGain = ceilingLinear / peakSample;
        }
        
        // Envelope follower (solo ataque instantáneo, release suave)
        if (targetGain < envelopeFollower_) {
            envelopeFollower_ = targetGain;
        } else {
            envelopeFollower_ = releaseCoeff_ * envelopeFollower_ + (1.0f - releaseCoeff_) * targetGain;
        }
        
        // Aplicar limiting
        float gain = envelopeFollower_;
        gain = std::min(gain, 1.0f);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float sample = buffer.getSample(ch, i);
            
            // Soft clip (opcional)
            if (settings_.softClip) {
                const float threshold = 0.8f;
                if (std::abs(sample) > threshold) {
                    sample = threshold + (sample - threshold * (sample > 0 ? 1 : -1)) * 
                             settings_.softClipAmount;
                }
            }
            
            // Aplicar gain
            sample *= gain;
            
            // Hard ceiling (brickwall)
            sample = juce::jlimit(-ceilingLinear, ceilingLinear, sample);
            
            buffer.setSample(ch, i, sample);
            
            // Metering
            if (settings_.meteringEnabled) {
                metering_.inputPeak = std::max(metering_.inputPeak, std::abs(buffer.getSample(ch, i)));
                metering_.outputPeak = std::max(metering_.outputPeak, std::abs(sample));
                
                if (std::abs(sample) >= ceilingLinear * 0.99f) {
                    metering_.clippedSamples++;
                }
            }
        }
        
        // Calcular gain reduction
        const float grDb = 20.0f * std::log10(std::max(gain, 0.00001f));
        metering_.gainReduction = std::min(metering_.gainReduction, grDb);
    }
    
    // Update metering
    if (settings_.meteringEnabled) {
        // True peak (simplificado sin oversampling por ahora)
        metering_.truePeak = 20.0f * std::log10(std::max(metering_.outputPeak, 0.00001f));
        
        // RMS
        float rmsSum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            rmsSum += buffer.getRMSLevel(ch, 0, numSamples);
        }
        metering_.rms = 20.0f * std::log10(std::max(rmsSum / numChannels, 0.00001f));
        
        // Crest factor
        const float peakDb = 20.0f * std::log10(std::max(metering_.outputPeak, 0.00001f));
        metering_.crestFactor = peakDb - metering_.rms;
    }
    
    totalSamplesProcessed_ += numSamples;
}

//==============================================================================
void LimiterMaximizer::setSettings(const Settings& settings) {
    settings_ = settings;
    
    // Recalcular coeficientes
    const float releaseTime = settings_.release * 0.001f;
    releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * releaseTime));
}

void LimiterMaximizer::setCeiling(float ceilingDb) {
    settings_.ceiling = juce::jlimit(-20.0f, 0.0f, ceilingDb);
}

void LimiterMaximizer::setThreshold(float thresholdDb) {
    settings_.threshold = juce::jlimit(-60.0f, 0.0f, thresholdDb);
}

void LimiterMaximizer::setRelease(float releaseMs) {
    settings_.release = juce::jlimit(10.0f, 1000.0f, releaseMs);
    const float releaseTime = settings_.release * 0.001f;
    releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * releaseTime));
}

void LimiterMaximizer::setLookAhead(float lookAheadMs) {
    settings_.lookAhead = juce::jlimit(0.0f, 20.0f, lookAheadMs);
}

void LimiterMaximizer::setAutoGain(bool enabled) {
    settings_.autoGain = enabled;
}

void LimiterMaximizer::setInputGain(float gainDb) {
    settings_.inputGain = juce::jlimit(-24.0f, 24.0f, gainDb);
}

void LimiterMaximizer::setSoftClip(bool enabled, float amount) {
    settings_.softClip = enabled;
    settings_.softClipAmount = juce::jlimit(0.0f, 1.0f, amount);
}

//==============================================================================
LimiterMaximizer::MeteringData LimiterMaximizer::getCurrentMetering() const {
    return metering_;
}

void LimiterMaximizer::resetMetering() {
    metering_ = MeteringData();
    totalSamplesProcessed_ = 0;
}

//==============================================================================
void LimiterMaximizer::loadPreset(const juce::String& presetName) {
    LimiterMaximizerPresets::applyPreset(*this, presetName);
}

juce::StringArray LimiterMaximizer::getPresetList() const {
    return {
        "Transparent",
        "Loud Master",
        "Streaming (-14 LUFS)",
        "Broadcast",
        "Mastering Gentle",
        "Club Banger"
    };
}

float LimiterMaximizer::softClip(float sample, float amount) {
    const float threshold = 0.7f;
    if (std::abs(sample) <= threshold) {
        return sample;
    }
    
    const float sign = sample > 0 ? 1.0f : -1.0f;
    const float absS = std::abs(sample);
    const float excess = absS - threshold;
    
    // Tanh soft clipping
    const float clipped = threshold + std::tanh(excess * amount) * (1.0f - threshold);
    
    return sign * clipped;
}

} // namespace OmegaStudio
