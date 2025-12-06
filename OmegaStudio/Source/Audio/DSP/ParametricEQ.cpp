//==============================================================================
// ParametricEQ.cpp - Implementation
//==============================================================================

#include "ParametricEQ.h"
#include <cmath>

namespace OmegaStudio {

//==============================================================================
ParametricEQ::ParametricEQ() {
    // Inicializar bandas con frecuencias distribuidas logarítmicamente
    const float frequencies[7] = {80.0f, 200.0f, 500.0f, 1200.0f, 3000.0f, 8000.0f, 15000.0f};
    
    for (int i = 0; i < 7; ++i) {
        bands_[i].frequency = frequencies[i];
        bands_[i].gain = 0.0f;
        bands_[i].Q = 1.0f;
        bands_[i].enabled = false;
        
        // Tipo de filtro por banda
        if (i == 0) {
            bands_[i].type = FilterType::LowShelf;
        } else if (i == 6) {
            bands_[i].type = FilterType::HighShelf;
        } else {
            bands_[i].type = FilterType::Bell;
        }
    }
    
    inputSpectrum_.resize(fftSize / 2);
    outputSpectrum_.resize(fftSize / 2);
}

//==============================================================================
void ParametricEQ::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    
    // Setup smoothing (20ms ramp)
    const float rampTime = 0.02f;
    for (int i = 0; i < 7; ++i) {
        smoothedGains_[i].reset(sampleRate, rampTime);
        smoothedFreqs_[i].reset(sampleRate, rampTime);
        smoothedQs_[i].reset(sampleRate, rampTime);
        
        smoothedGains_[i].setCurrentAndTargetValue(bands_[i].gain);
        smoothedFreqs_[i].setCurrentAndTargetValue(bands_[i].frequency);
        smoothedQs_[i].setCurrentAndTargetValue(bands_[i].Q);
    }
    
    // Calcular coeficientes iniciales
    for (int i = 0; i < 7; ++i) {
        updateCoefficients(i);
    }
    
    reset();
}

//==============================================================================
void ParametricEQ::reset() {
    for (auto& band : bands_) {
        band.z[0] = 0.0f;
        band.z[1] = 0.0f;
    }
    
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
}

//==============================================================================
void ParametricEQ::process(juce::AudioBuffer<float>& buffer) {
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Procesar cada banda habilitada
    for (int band = 0; band < 7; ++band) {
        if (!bands_[band].enabled) continue;
        
        // Actualizar coeficientes si los parámetros cambiaron
        if (smoothedGains_[band].isSmoothing() || 
            smoothedFreqs_[band].isSmoothing() || 
            smoothedQs_[band].isSmoothing()) {
            updateCoefficients(band);
        }
        
        // Procesar cada canal
        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            const auto& coeffs = bands_[band].coeffs;
            auto& z = bands_[band].z;
            
            // Biquad filter: Direct Form II Transposed
            for (int i = 0; i < numSamples; ++i) {
                const float input = channelData[i];
                const float output = coeffs[0] * input + z[0];
                
                z[0] = coeffs[1] * input - coeffs[3] * output + z[1];
                z[1] = coeffs[2] * input - coeffs[4] * output;
                
                channelData[i] = output;
            }
        }
    }
}

//==============================================================================
float ParametricEQ::processSample(float sample, int bandIndex) {
    if (bandIndex < 0 || bandIndex >= 7 || !bands_[bandIndex].enabled) {
        return sample;
    }
    
    const auto& coeffs = bands_[bandIndex].coeffs;
    auto& z = bands_[bandIndex].z;
    
    const float output = coeffs[0] * sample + z[0];
    z[0] = coeffs[1] * sample - coeffs[3] * output + z[1];
    z[1] = coeffs[2] * sample - coeffs[4] * output;
    
    return output;
}

//==============================================================================
void ParametricEQ::setBandEnabled(int bandIndex, bool enabled) {
    if (bandIndex >= 0 && bandIndex < 7) {
        bands_[bandIndex].enabled = enabled;
    }
}

void ParametricEQ::setBandType(int bandIndex, FilterType type) {
    if (bandIndex >= 0 && bandIndex < 7) {
        bands_[bandIndex].type = type;
        updateCoefficients(bandIndex);
    }
}

void ParametricEQ::setBandFrequency(int bandIndex, float frequency) {
    if (bandIndex >= 0 && bandIndex < 7) {
        bands_[bandIndex].frequency = juce::jlimit(20.0f, 20000.0f, frequency);
        smoothedFreqs_[bandIndex].setTargetValue(bands_[bandIndex].frequency);
        updateCoefficients(bandIndex);
    }
}

void ParametricEQ::setBandGain(int bandIndex, float gainDb) {
    if (bandIndex >= 0 && bandIndex < 7) {
        bands_[bandIndex].gain = juce::jlimit(-24.0f, 24.0f, gainDb);
        smoothedGains_[bandIndex].setTargetValue(bands_[bandIndex].gain);
        updateCoefficients(bandIndex);
    }
}

void ParametricEQ::setBandQ(int bandIndex, float Q) {
    if (bandIndex >= 0 && bandIndex < 7) {
        bands_[bandIndex].Q = juce::jlimit(0.1f, 20.0f, Q);
        smoothedQs_[bandIndex].setTargetValue(bands_[bandIndex].Q);
        updateCoefficients(bandIndex);
    }
}

void ParametricEQ::setBandSlope(int bandIndex, Slope slope) {
    if (bandIndex >= 0 && bandIndex < 7) {
        bands_[bandIndex].slope = slope;
        updateCoefficients(bandIndex);
    }
}

//==============================================================================
void ParametricEQ::updateCoefficients(int bandIndex) {
    auto& band = bands_[bandIndex];
    
    switch (band.type) {
        case FilterType::Bell:
            calculateBellCoeffs(band);
            break;
        case FilterType::LowShelf:
            calculateShelfCoeffs(band, true);
            break;
        case FilterType::HighShelf:
            calculateShelfCoeffs(band, false);
            break;
        case FilterType::LowCut:
            calculateCutCoeffs(band, true);
            break;
        case FilterType::HighCut:
            calculateCutCoeffs(band, false);
            break;
        case FilterType::Notch:
            calculateNotchCoeffs(band);
            break;
        default:
            break;
    }
}

//==============================================================================
void ParametricEQ::calculateBellCoeffs(Band& band) {
    const float freq = band.frequency;
    const float gain = band.gain;
    const float Q = band.Q;
    
    const float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sampleRate_);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * Q);
    const float A = std::pow(10.0f, gain / 40.0f);
    
    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cosOmega;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cosOmega;
    const float a2 = 1.0f - alpha / A;
    
    // Normalizar
    band.coeffs[0] = b0 / a0;
    band.coeffs[1] = b1 / a0;
    band.coeffs[2] = b2 / a0;
    band.coeffs[3] = a1 / a0;
    band.coeffs[4] = a2 / a0;
}

//==============================================================================
void ParametricEQ::calculateShelfCoeffs(Band& band, bool isLow) {
    const float freq = band.frequency;
    const float gain = band.gain;
    const float Q = band.Q;
    
    const float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sampleRate_);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float A = std::pow(10.0f, gain / 40.0f);
    const float beta = std::sqrt(A) / Q;
    
    float b0, b1, b2, a0, a1, a2;
    
    if (isLow) {
        b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega);
        b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega);
        a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega;
        a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega;
    } else {
        b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega);
        a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega;
    }
    
    // Normalizar
    band.coeffs[0] = b0 / a0;
    band.coeffs[1] = b1 / a0;
    band.coeffs[2] = b2 / a0;
    band.coeffs[3] = a1 / a0;
    band.coeffs[4] = a2 / a0;
}

//==============================================================================
void ParametricEQ::calculateCutCoeffs(Band& band, bool isLow) {
    const float freq = band.frequency;
    const float Q = 0.7071f; // Butterworth
    
    const float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sampleRate_);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * Q);
    
    float b0, b1, b2, a0, a1, a2;
    
    if (isLow) {
        // High-pass filter
        b0 = (1.0f + cosOmega) / 2.0f;
        b1 = -(1.0f + cosOmega);
        b2 = (1.0f + cosOmega) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosOmega;
        a2 = 1.0f - alpha;
    } else {
        // Low-pass filter
        b0 = (1.0f - cosOmega) / 2.0f;
        b1 = 1.0f - cosOmega;
        b2 = (1.0f - cosOmega) / 2.0f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosOmega;
        a2 = 1.0f - alpha;
    }
    
    // Normalizar
    band.coeffs[0] = b0 / a0;
    band.coeffs[1] = b1 / a0;
    band.coeffs[2] = b2 / a0;
    band.coeffs[3] = a1 / a0;
    band.coeffs[4] = a2 / a0;
    
    // Para slopes mayores, aplicar en cascada (implementación simplificada)
    if (band.slope == Slope::dB48) {
        // Doblar el efecto (aproximación)
        for (int i = 0; i < 5; ++i) {
            band.coeffs[i] *= 2.0f;
        }
    }
}

//==============================================================================
void ParametricEQ::calculateNotchCoeffs(Band& band) {
    const float freq = band.frequency;
    const float Q = band.Q;
    
    const float omega = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sampleRate_);
    const float sinOmega = std::sin(omega);
    const float cosOmega = std::cos(omega);
    const float alpha = sinOmega / (2.0f * Q);
    
    const float b0 = 1.0f;
    const float b1 = -2.0f * cosOmega;
    const float b2 = 1.0f;
    const float a0 = 1.0f + alpha;
    const float a1 = -2.0f * cosOmega;
    const float a2 = 1.0f - alpha;
    
    // Normalizar
    band.coeffs[0] = b0 / a0;
    band.coeffs[1] = b1 / a0;
    band.coeffs[2] = b2 / a0;
    band.coeffs[3] = a1 / a0;
    band.coeffs[4] = a2 / a0;
}

//==============================================================================
ParametricEQ::FrequencyResponse ParametricEQ::calculateFrequencyResponse(int numPoints) const {
    FrequencyResponse response;
    response.frequencies.resize(numPoints);
    response.magnitudes.resize(numPoints);
    response.phases.resize(numPoints);
    
    // Generar frecuencias logarítmicamente espaciadas
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float logMin = std::log10(minFreq);
    const float logMax = std::log10(maxFreq);
    
    for (int i = 0; i < numPoints; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
        response.frequencies[i] = std::pow(10.0f, logMin + t * (logMax - logMin));
    }
    
    // Calcular respuesta para cada frecuencia
    for (int i = 0; i < numPoints; ++i) {
        const float freq = response.frequencies[i];
        
        float magnitude = 1.0f;
        float phase = 0.0f;
        
        // Combinar respuesta de todas las bandas habilitadas
        for (int band = 0; band < 7; ++band) {
            if (bands_[band].enabled) {
                magnitude *= getMagnitudeAt(freq, band);
                phase += getPhaseAt(freq, band);
            }
        }
        
        response.magnitudes[i] = 20.0f * std::log10(std::max(magnitude, 0.00001f));
        response.phases[i] = phase;
    }
    
    return response;
}

//==============================================================================
float ParametricEQ::getMagnitudeAt(float frequency, int bandIndex) const {
    const auto& band = bands_[bandIndex];
    const auto& c = band.coeffs;
    
    const float omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sampleRate_);
    const std::complex<float> z = std::exp(std::complex<float>(0.0f, omega));
    const std::complex<float> z2 = z * z;
    
    // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
    const std::complex<float> numerator = c[0] + c[1] / z + c[2] / z2;
    const std::complex<float> denominator = 1.0f + c[3] / z + c[4] / z2;
    
    const std::complex<float> H = numerator / denominator;
    return std::abs(H);
}

//==============================================================================
float ParametricEQ::getPhaseAt(float frequency, int bandIndex) const {
    const auto& band = bands_[bandIndex];
    const auto& c = band.coeffs;
    
    const float omega = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(sampleRate_);
    const std::complex<float> z = std::exp(std::complex<float>(0.0f, omega));
    const std::complex<float> z2 = z * z;
    
    const std::complex<float> numerator = c[0] + c[1] / z + c[2] / z2;
    const std::complex<float> denominator = 1.0f + c[3] / z + c[4] / z2;
    
    const std::complex<float> H = numerator / denominator;
    return std::arg(H);
}

//==============================================================================
void ParametricEQ::analyzeInput(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumSamples() < fftSize) return;
    
    // Copiar datos de entrada al buffer FFT
    const float* channelData = buffer.getReadPointer(0);
    for (int i = 0; i < fftSize; ++i) {
        fftData_[i] = channelData[i];
    }
    
    // Aplicar ventana
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize);
    
    // Realizar FFT
    fft_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // Convertir a dB
    for (int i = 0; i < fftSize / 2; ++i) {
        const float magnitude = fftData_[i];
        inputSpectrum_[i] = 20.0f * std::log10(std::max(magnitude, 0.00001f));
    }
}

} // namespace OmegaStudio
