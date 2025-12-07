#include "DenoiseService.h"
#include <cmath>

namespace omega {
namespace AI {

DenoiseService::DenoiseService() {
    jobQueue_ = std::make_unique<AIJobQueue>(2); // 2 worker threads
}

DenoiseService::~DenoiseService() {
    cancelAll();
}

DenoiseResult DenoiseService::processAudio(const juce::AudioBuffer<float>& input) {
    if (input.getNumSamples() == 0) {
        return {juce::AudioBuffer<float>(), 0.0f, 0.0f, false, "Empty input buffer"};
    }
    
    isProcessing_ = true;
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    
    DenoiseResult result;
    
    // Use ONNX model if available, otherwise fallback to spectral subtraction
    if (config_.modelPath.existsAsFile()) {
        result = applyRNNoise(input);
    } else {
        result = applySpectralSubtraction(input);
    }
    
    // Post-process to reduce artifacts
    if (result.success && config_.preserveTransients) {
        reduceArtifacts(result.denoisedAudio);
    }
    
    result.processingTimeMs = static_cast<float>(juce::Time::getMillisecondCounterHiRes() - startTime);
    isProcessing_ = false;
    
    return result;
}

void DenoiseService::processAudioAsync(const juce::AudioBuffer<float>& input,
                                       std::function<void(DenoiseResult)> callback) {
    if (!jobQueue_) return;
    
    auto inputCopy = input;
    
    jobQueue_->addJob([this, inputCopy, callback]() {
        auto result = processAudio(inputCopy);
        if (callback) {
            callback(result);
        }
    }, AIJobPriority::High); // Denoise is high priority for user experience
}

void DenoiseService::cancelAll() {
    isProcessing_ = false;
    if (jobQueue_) {
        jobQueue_.reset();
        jobQueue_ = std::make_unique<AIJobQueue>(2);
    }
}

DenoiseResult DenoiseService::applyRNNoise(const juce::AudioBuffer<float>& input) {
    // TODO: Integrate ONNX RNNoise model
    // For now, fallback to spectral subtraction
    return applySpectralSubtraction(input);
}

DenoiseResult DenoiseService::applySpectralSubtraction(const juce::AudioBuffer<float>& input) {
    DenoiseResult result;
    result.denoisedAudio.setSize(input.getNumChannels(), input.getNumSamples());
    
    // Simple spectral subtraction algorithm
    const int fftSize = 2048;
    const int hopSize = fftSize / 4;
    
    juce::dsp::FFT fft(11); // 2^11 = 2048
    juce::HeapBlock<juce::dsp::Complex<float>> fftBuffer;
    fftBuffer.calloc(fftSize * 2);
    
    for (int channel = 0; channel < input.getNumChannels(); ++channel) {
        const float* inputData = input.getReadPointer(channel);
        float* outputData = result.denoisedAudio.getWritePointer(channel);
        
        // Estimate noise floor from first 100ms
        int noiseEstimateSamples = juce::jmin((int)(config_.sampleRate * 0.1), input.getNumSamples() / 4);
        float noiseFloor = 0.0f;
        
        for (int i = 0; i < noiseEstimateSamples; ++i) {
            noiseFloor += std::abs(inputData[i]);
        }
        noiseFloor /= noiseEstimateSamples;
        noiseFloor *= config_.reductionAmount;
        
        // Process in overlapping windows
        for (int pos = 0; pos < input.getNumSamples(); pos += hopSize) {
            int windowSize = juce::jmin(fftSize, input.getNumSamples() - pos);
            
            // Copy window with Hann windowing
            for (int i = 0; i < windowSize; ++i) {
                float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / (windowSize - 1)));
                fftBuffer[i].real(inputData[pos + i] * window);
                fftBuffer[i].imag(0.0f);
            }
            
            // Zero pad
            for (int i = windowSize; i < fftSize; ++i) {
                fftBuffer[i] = juce::dsp::Complex<float>(0.0f, 0.0f);
            }
            
            // Forward FFT
            fft.perform(fftBuffer, fftBuffer + fftSize, false);
            
            // Spectral subtraction
            for (int i = 0; i < fftSize / 2; ++i) {
                float magnitude = std::sqrt(fftBuffer[i].real() * fftBuffer[i].real() + 
                                          fftBuffer[i].imag() * fftBuffer[i].imag());
                float phase = std::atan2(fftBuffer[i].imag(), fftBuffer[i].real());
                
                // Subtract noise floor
                magnitude = juce::jmax(0.0f, magnitude - noiseFloor);
                
                // Reconstruct complex number
                fftBuffer[i].real(magnitude * std::cos(phase));
                fftBuffer[i].imag(magnitude * std::sin(phase));
                
                // Mirror for negative frequencies
                if (i > 0 && i < fftSize / 2) {
                    fftBuffer[fftSize - i] = std::conj(fftBuffer[i]);
                }
            }
            
            // Inverse FFT
            fft.perform(fftBuffer + fftSize, fftBuffer, true);
            
            // Overlap-add
            for (int i = 0; i < windowSize && (pos + i) < input.getNumSamples(); ++i) {
                float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / (windowSize - 1)));
                outputData[pos + i] += fftBuffer[fftSize + i].real() * window / fftSize;
            }
        }
        
        // Calculate noise reduction amount
        float inputRMS = input.getRMSLevel(channel, 0, input.getNumSamples());
        float outputRMS = result.denoisedAudio.getRMSLevel(channel, 0, result.denoisedAudio.getNumSamples());
        result.noiseReductionDb = 20.0f * std::log10(outputRMS / juce::jmax(inputRMS, 0.0001f));
    }
    
    result.success = true;
    return result;
}

void DenoiseService::reduceArtifacts(juce::AudioBuffer<float>& buffer) {
    // Apply gentle smoothing to reduce musical noise artifacts
    const int smoothingSize = 3;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* data = buffer.getWritePointer(channel);
        
        for (int i = smoothingSize; i < buffer.getNumSamples() - smoothingSize; ++i) {
            float smoothed = 0.0f;
            for (int j = -smoothingSize; j <= smoothingSize; ++j) {
                smoothed += data[i + j];
            }
            data[i] = smoothed / (2 * smoothingSize + 1);
        }
    }
}

} // namespace AI
} // namespace omega
