//==============================================================================
// MultibandCompressor.cpp - Implementation
//==============================================================================

#include "MultibandCompressor.h"
#include <cmath>

namespace OmegaStudio {

//==============================================================================
// LinkwitzRileyCrossover Implementation
//==============================================================================

void MultibandCompressor::LinkwitzRileyCrossover::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
    reset();
}

void MultibandCompressor::LinkwitzRileyCrossover::setCrossoverFrequency(float frequency) {
    frequency_ = juce::jlimit(20.0f, 20000.0f, frequency);
    updateCoefficients();
}

void MultibandCompressor::LinkwitzRileyCrossover::updateCoefficients() {
    // Butterworth 2nd order lowpass/highpass
    const float omega = juce::MathConstants<float>::twoPi * frequency_ / static_cast<float>(sampleRate_);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * 0.7071f); // Q = 0.7071 (Butterworth)
    
    // Lowpass
    const float b0_lp = (1.0f - cosOmega) / 2.0f;
    const float b1_lp = 1.0f - cosOmega;
    const float b2_lp = (1.0f - cosOmega) / 2.0f;
    const float a0_lp = 1.0f + alpha;
    const float a1_lp = -2.0f * cosOmega;
    const float a2_lp = 1.0f - alpha;
    
    coeffsLow_[0] = b0_lp / a0_lp;
    coeffsLow_[1] = b1_lp / a0_lp;
    coeffsLow_[2] = b2_lp / a0_lp;
    coeffsLow_[3] = a1_lp / a0_lp;
    
    // Highpass
    const float b0_hp = (1.0f + cosOmega) / 2.0f;
    const float b1_hp = -(1.0f + cosOmega);
    const float b2_hp = (1.0f + cosOmega) / 2.0f;
    const float a0_hp = 1.0f + alpha;
    const float a1_hp = -2.0f * cosOmega;
    const float a2_hp = 1.0f - alpha;
    
    coeffsHigh_[0] = b0_hp / a0_hp;
    coeffsHigh_[1] = b1_hp / a0_hp;
    coeffsHigh_[2] = b2_hp / a0_hp;
    coeffsHigh_[3] = a1_hp / a0_hp;
}

void MultibandCompressor::LinkwitzRileyCrossover::process(const float* input, 
                                                            float* lowOut, 
                                                            float* highOut, 
                                                            int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        const float sample = input[i];
        
        // Lowpass (2 passes para LR4)
        float low = sample;
        for (int pass = 0; pass < 2; ++pass) {
            const float output = coeffsLow_[0] * low + stateLow_[pass * 2];
            stateLow_[pass * 2] = coeffsLow_[1] * low - coeffsLow_[3] * output + stateLow_[pass * 2 + 1];
            stateLow_[pass * 2 + 1] = coeffsLow_[2] * low - coeffsLow_[3] * output;
            low = output;
        }
        lowOut[i] = low;
        
        // Highpass (2 passes para LR4)
        float high = sample;
        for (int pass = 0; pass < 2; ++pass) {
            const float output = coeffsHigh_[0] * high + stateHigh_[pass * 2];
            stateHigh_[pass * 2] = coeffsHigh_[1] * high - coeffsHigh_[3] * output + stateHigh_[pass * 2 + 1];
            stateHigh_[pass * 2 + 1] = coeffsHigh_[2] * high - coeffsHigh_[3] * output;
            high = output;
        }
        highOut[i] = high;
    }
}

void MultibandCompressor::LinkwitzRileyCrossover::reset() {
    std::fill(stateLow_.begin(), stateLow_.end(), 0.0f);
    std::fill(stateHigh_.begin(), stateHigh_.end(), 0.0f);
}

//==============================================================================
// BandCompressor Implementation
//==============================================================================

void MultibandCompressor::BandCompressor::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
    reset();
}

void MultibandCompressor::BandCompressor::setParameters(const BandSettings::Band& params) {
    params_ = params;
    updateCoefficients();
}

void MultibandCompressor::BandCompressor::updateCoefficients() {
    // Attack coefficient
    const float attackTime = params_.attack * 0.001f; // ms to seconds
    attackCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * attackTime));
    
    // Release coefficient
    const float releaseTime = params_.release * 0.001f;
    releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * releaseTime));
}

void MultibandCompressor::BandCompressor::process(float* buffer, int numSamples) {
    if (!params_.enabled) return;
    
    for (int i = 0; i < numSamples; ++i) {
        const float input = buffer[i];
        
        // Detección de nivel
        float level = 0.0f;
        if (params_.detectionMode == DetectionMode::Peak) {
            level = std::abs(input);
        } else { // RMS
            level = input * input;
        }
        
        // Envelope follower
        if (level > envelopeFollower_) {
            envelopeFollower_ = attackCoeff_ * (envelopeFollower_ - level) + level;
        } else {
            envelopeFollower_ = releaseCoeff_ * (envelopeFollower_ - level) + level;
        }
        
        // Convertir a dB
        float envelopeDb = 0.0f;
        if (params_.detectionMode == DetectionMode::RMS) {
            envelopeDb = 10.0f * std::log10(std::max(envelopeFollower_, 0.00001f));
        } else {
            envelopeDb = 20.0f * std::log10(std::max(envelopeFollower_, 0.00001f));
        }
        
        // Calcular ganancia de compresión
        float gain = computeGain(envelopeDb);
        
        // Aplicar ganancia
        float output = input * gain;
        
        // Makeup gain
        float makeupGain = params_.makeupGain;
        if (params_.autoMakeup) {
            // Auto-makeup: compensar la reducción promedio
            makeupGain = (params_.threshold - envelopeDb) * (1.0f - 1.0f / params_.ratio) * 0.5f;
            makeupGain = juce::jlimit(0.0f, 24.0f, makeupGain);
        }
        
        output *= std::pow(10.0f, makeupGain / 20.0f);
        
        buffer[i] = output;
        
        // Metering
        inputLevel_ = std::max(inputLevel_ * 0.99f, std::abs(input));
        outputLevel_ = std::max(outputLevel_ * 0.99f, std::abs(output));
        gainReduction_ = 20.0f * std::log10(std::max(gain, 0.00001f));
    }
}

float MultibandCompressor::BandCompressor::computeGain(float inputLevelDb) {
    const float threshold = params_.threshold;
    const float ratio = params_.ratio;
    const float knee = params_.knee;
    
    float gainDb = 0.0f;
    
    if (inputLevelDb < (threshold - knee / 2.0f)) {
        // Por debajo del knee: sin compresión
        gainDb = 0.0f;
    } else if (inputLevelDb > (threshold + knee / 2.0f)) {
        // Por encima del knee: compresión completa
        gainDb = (threshold - inputLevelDb) + (inputLevelDb - threshold) / ratio;
    } else {
        // Dentro del knee: transición suave
        const float x = inputLevelDb - threshold + knee / 2.0f;
        gainDb = -x * x / (2.0f * knee);
    }
    
    // Convertir de dB a linear
    return std::pow(10.0f, gainDb / 20.0f);
}

void MultibandCompressor::BandCompressor::reset() {
    envelopeFollower_ = 0.0f;
    gainReduction_ = 0.0f;
    inputLevel_ = 0.0f;
    outputLevel_ = 0.0f;
}

//==============================================================================
// MultibandCompressor Implementation
//==============================================================================

MultibandCompressor::MultibandCompressor() {
    // Setup default settings
    settings_.crossoverLow = 120.0f;
    settings_.crossoverMid = 1000.0f;
    settings_.crossoverHigh = 8000.0f;
    
    for (int i = 0; i < 4; ++i) {
        settings_.bands[i].enabled = true;
        settings_.bands[i].threshold = -20.0f;
        settings_.bands[i].ratio = 3.0f;
        settings_.bands[i].attack = 10.0f;
        settings_.bands[i].release = 100.0f;
        settings_.bands[i].knee = 6.0f;
        settings_.bands[i].makeupGain = 0.0f;
        settings_.bands[i].autoMakeup = false;
    }
}

void MultibandCompressor::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    
    // Preparar crossovers
    crossovers_[0].prepare(sampleRate);
    crossovers_[0].setCrossoverFrequency(settings_.crossoverLow);
    
    crossovers_[1].prepare(sampleRate);
    crossovers_[1].setCrossoverFrequency(settings_.crossoverMid);
    
    crossovers_[2].prepare(sampleRate);
    crossovers_[2].setCrossoverFrequency(settings_.crossoverHigh);
    
    // Preparar compresores
    for (int i = 0; i < 4; ++i) {
        compressors_[i].prepare(sampleRate);
        compressors_[i].setParameters(settings_.bands[i]);
        
        // Alocar buffers de banda
        bandBuffers_[i].setSize(2, samplesPerBlock);
    }
    
    reset();
}

void MultibandCompressor::reset() {
    for (auto& crossover : crossovers_) {
        crossover.reset();
    }
    
    for (auto& compressor : compressors_) {
        compressor.reset();
    }
    
    for (auto& buffer : bandBuffers_) {
        buffer.clear();
    }
    
    std::fill(bandInputLevels_.begin(), bandInputLevels_.end(), 0.0f);
    std::fill(bandOutputLevels_.begin(), bandOutputLevels_.end(), 0.0f);
}

void MultibandCompressor::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Procesar cada canal
    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        
        // Buffers temporales
        std::vector<float> tempBuffers[4];
        for (int i = 0; i < 4; ++i) {
            tempBuffers[i].resize(numSamples);
        }
        
        // Crossover 1: Separar Low (band 0) de Rest (bands 1-3)
        std::vector<float> lowBand(numSamples);
        std::vector<float> restBand1(numSamples);
        crossovers_[0].process(channelData, lowBand.data(), restBand1.data(), numSamples);
        
        // Crossover 2: Separar LowMid (band 1) de Rest (bands 2-3)
        std::vector<float> lowMidBand(numSamples);
        std::vector<float> restBand2(numSamples);
        crossovers_[1].process(restBand1.data(), lowMidBand.data(), restBand2.data(), numSamples);
        
        // Crossover 3: Separar MidHigh (band 2) de High (band 3)
        std::vector<float> midHighBand(numSamples);
        std::vector<float> highBand(numSamples);
        crossovers_[2].process(restBand2.data(), midHighBand.data(), highBand.data(), numSamples);
        
        // Copiar a tempBuffers
        std::copy(lowBand.begin(), lowBand.end(), tempBuffers[0].begin());
        std::copy(lowMidBand.begin(), lowMidBand.end(), tempBuffers[1].begin());
        std::copy(midHighBand.begin(), midHighBand.end(), tempBuffers[2].begin());
        std::copy(highBand.begin(), highBand.end(), tempBuffers[3].begin());
        
        // Comprimir cada banda
        bool anySolo = false;
        for (int i = 0; i < 4; ++i) {
            if (settings_.bands[i].solo) {
                anySolo = true;
                break;
            }
        }
        
        for (int i = 0; i < 4; ++i) {
            // Metering de entrada
            for (int s = 0; s < numSamples; ++s) {
                bandInputLevels_[i] = std::max(bandInputLevels_[i] * 0.99f, 
                                                std::abs(tempBuffers[i][s]));
            }
            
            // Aplicar solo/mute
            if (anySolo && !settings_.bands[i].solo) {
                std::fill(tempBuffers[i].begin(), tempBuffers[i].end(), 0.0f);
                continue;
            }
            
            if (settings_.bands[i].mute) {
                std::fill(tempBuffers[i].begin(), tempBuffers[i].end(), 0.0f);
                continue;
            }
            
            // Comprimir
            compressors_[i].setParameters(settings_.bands[i]);
            compressors_[i].process(tempBuffers[i].data(), numSamples);
            
            // Metering de salida
            for (int s = 0; s < numSamples; ++s) {
                bandOutputLevels_[i] = std::max(bandOutputLevels_[i] * 0.99f, 
                                                 std::abs(tempBuffers[i][s]));
            }
            
            // Guardar gain reduction
            settings_.bands[i].gainReduction = compressors_[i].getGainReduction();
        }
        
        // Mezclar bandas de vuelta
        for (int i = 0; i < numSamples; ++i) {
            channelData[i] = tempBuffers[0][i] + tempBuffers[1][i] + 
                              tempBuffers[2][i] + tempBuffers[3][i];
        }
    }
}

//==============================================================================
void MultibandCompressor::setSettings(const BandSettings& settings) {
    settings_ = settings;
    
    // Actualizar crossovers
    crossovers_[0].setCrossoverFrequency(settings_.crossoverLow);
    crossovers_[1].setCrossoverFrequency(settings_.crossoverMid);
    crossovers_[2].setCrossoverFrequency(settings_.crossoverHigh);
    
    // Actualizar compresores
    for (int i = 0; i < 4; ++i) {
        compressors_[i].setParameters(settings_.bands[i]);
    }
}

//==============================================================================
void MultibandCompressor::setBandEnabled(int bandIndex, bool enabled) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].enabled = enabled;
    }
}

void MultibandCompressor::setBandThreshold(int bandIndex, float thresholdDb) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].threshold = juce::jlimit(-60.0f, 0.0f, thresholdDb);
    }
}

void MultibandCompressor::setBandRatio(int bandIndex, float ratio) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].ratio = juce::jlimit(1.0f, 20.0f, ratio);
    }
}

void MultibandCompressor::setBandAttack(int bandIndex, float attackMs) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].attack = juce::jlimit(0.1f, 100.0f, attackMs);
        compressors_[bandIndex].setParameters(settings_.bands[bandIndex]);
    }
}

void MultibandCompressor::setBandRelease(int bandIndex, float releaseMs) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].release = juce::jlimit(10.0f, 1000.0f, releaseMs);
        compressors_[bandIndex].setParameters(settings_.bands[bandIndex]);
    }
}

void MultibandCompressor::setBandKnee(int bandIndex, float kneeDb) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].knee = juce::jlimit(0.0f, 12.0f, kneeDb);
    }
}

void MultibandCompressor::setBandMakeupGain(int bandIndex, float gainDb) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].makeupGain = juce::jlimit(-24.0f, 24.0f, gainDb);
    }
}

void MultibandCompressor::setBandAutoMakeup(int bandIndex, bool autoMakeup) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].autoMakeup = autoMakeup;
    }
}

void MultibandCompressor::setBandDetectionMode(int bandIndex, DetectionMode mode) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].detectionMode = mode;
    }
}

//==============================================================================
void MultibandCompressor::setCrossoverLow(float frequency) {
    settings_.crossoverLow = juce::jlimit(20.0f, 1000.0f, frequency);
    crossovers_[0].setCrossoverFrequency(settings_.crossoverLow);
}

void MultibandCompressor::setCrossoverMid(float frequency) {
    settings_.crossoverMid = juce::jlimit(200.0f, 5000.0f, frequency);
    crossovers_[1].setCrossoverFrequency(settings_.crossoverMid);
}

void MultibandCompressor::setCrossoverHigh(float frequency) {
    settings_.crossoverHigh = juce::jlimit(2000.0f, 16000.0f, frequency);
    crossovers_[2].setCrossoverFrequency(settings_.crossoverHigh);
}

//==============================================================================
void MultibandCompressor::setBandSolo(int bandIndex, bool solo) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].solo = solo;
    }
}

void MultibandCompressor::setBandMute(int bandIndex, bool mute) {
    if (bandIndex >= 0 && bandIndex < 4) {
        settings_.bands[bandIndex].mute = mute;
    }
}

//==============================================================================
float MultibandCompressor::getBandGainReduction(int bandIndex) const {
    if (bandIndex >= 0 && bandIndex < 4) {
        return settings_.bands[bandIndex].gainReduction;
    }
    return 0.0f;
}

float MultibandCompressor::getBandInputLevel(int bandIndex) const {
    if (bandIndex >= 0 && bandIndex < 4) {
        return bandInputLevels_[bandIndex];
    }
    return 0.0f;
}

float MultibandCompressor::getBandOutputLevel(int bandIndex) const {
    if (bandIndex >= 0 && bandIndex < 4) {
        return bandOutputLevels_[bandIndex];
    }
    return 0.0f;
}

//==============================================================================
MultibandCompressor::BandSpectrum MultibandCompressor::getCurrentSpectrum() const {
    BandSpectrum spectrum;
    
    for (int i = 0; i < 4; ++i) {
        spectrum.inputLevels[i] = bandInputLevels_[i];
        spectrum.outputLevels[i] = bandOutputLevels_[i];
        spectrum.gainReductions[i] = settings_.bands[i].gainReduction;
    }
    
    return spectrum;
}

//==============================================================================
void MultibandCompressor::loadPreset(const juce::String& presetName) {
    MultibandCompressorPresets::applyPreset(*this, presetName);
}

juce::StringArray MultibandCompressor::getPresetList() const {
    return {
        "Master Glue",
        "Vocal Bus",
        "Drum Bus",
        "Mastering Gentle",
        "Mastering Aggressive",
        "Bass Control",
        "Broadcast"
    };
}

} // namespace OmegaStudio
