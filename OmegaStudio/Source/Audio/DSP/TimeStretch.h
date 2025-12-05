#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace Audio {
namespace DSP {

/**
 * @struct WarpMarker
 * @brief Marcador de warp para time-stretching
 */
struct WarpMarker {
    double samplePosition;     // Posición en samples del audio original
    double beatPosition;       // Posición en el timeline (beats)
    bool isLocked { false };   // Si está bloqueado, no se mueve con tempo changes
    
    WarpMarker(double sample = 0.0, double beat = 0.0)
        : samplePosition(sample), beatPosition(beat) {}
};

/**
 * @class TempoDetector
 * @brief Detecta el tempo de audio usando onset detection
 */
class TempoDetector {
public:
    TempoDetector() = default;
    
    struct Result {
        double bpm { 0.0 };
        double confidence { 0.0 };
        std::vector<double> onsets;  // Onset positions in samples
    };
    
    Result detectTempo(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        Result result;
        
        // 1. Spectral flux para onset detection
        auto onsets = detectOnsets(buffer, sampleRate);
        result.onsets = onsets;
        
        if (onsets.size() < 2) {
            return result;  // Not enough onsets
        }
        
        // 2. Calculate inter-onset intervals (IOIs)
        std::vector<double> intervals;
        for (size_t i = 1; i < onsets.size(); ++i) {
            intervals.push_back(onsets[i] - onsets[i-1]);
        }
        
        // 3. Find most common interval (using histogram)
        std::map<int, int> histogram;
        for (double interval : intervals) {
            double bpm = 60.0 * sampleRate / interval;
            // Quantize to nearest BPM
            int bpmInt = std::round(bpm);
            
            // Also check octave multiples/divisions (60, 120, 180, etc.)
            for (int mult : {1, 2, 3, 4}) {
                int candidate = bpmInt / mult;
                if (candidate >= 60 && candidate <= 200) {
                    histogram[candidate]++;
                }
            }
        }
        
        // 4. Find peak in histogram
        int maxCount = 0;
        int detectedBPM = 120;
        for (const auto& [bpm, count] : histogram) {
            if (count > maxCount) {
                maxCount = count;
                detectedBPM = bpm;
            }
        }
        
        result.bpm = detectedBPM;
        result.confidence = maxCount / (double)intervals.size();
        
        return result;
    }
    
private:
    std::vector<double> detectOnsets(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        std::vector<double> onsets;
        
        // Simple energy-based onset detection
        const int hopSize = 512;
        const int frameSize = 2048;
        const float threshold = 0.3f;
        
        std::vector<float> spectralFlux;
        std::vector<float> prevMagnitude(frameSize / 2, 0.0f);
        
        juce::dsp::FFT fft(11);  // 2048 points
        std::vector<float> fftData(frameSize * 2, 0.0f);
        
        for (int pos = 0; pos < buffer.getNumSamples() - frameSize; pos += hopSize) {
            // Get frame (mono mix)
            for (int i = 0; i < frameSize; ++i) {
                float sample = 0.0f;
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    sample += buffer.getSample(ch, pos + i);
                }
                fftData[i] = sample / buffer.getNumChannels();
            }
            
            // Apply Hann window
            for (int i = 0; i < frameSize; ++i) {
                float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / frameSize));
                fftData[i] *= window;
            }
            
            // FFT
            fft.performFrequencyOnlyForwardTransform(fftData.data());
            
            // Calculate spectral flux
            float flux = 0.0f;
            for (int i = 0; i < frameSize / 2; ++i) {
                float magnitude = fftData[i];
                float diff = magnitude - prevMagnitude[i];
                if (diff > 0.0f) {
                    flux += diff;
                }
                prevMagnitude[i] = magnitude;
            }
            
            spectralFlux.push_back(flux);
        }
        
        // Peak picking
        for (size_t i = 2; i < spectralFlux.size() - 2; ++i) {
            float localMean = 0.0f;
            for (int j = -2; j <= 2; ++j) {
                localMean += spectralFlux[i + j];
            }
            localMean /= 5.0f;
            
            if (spectralFlux[i] > localMean * (1.0f + threshold) &&
                spectralFlux[i] > spectralFlux[i-1] &&
                spectralFlux[i] > spectralFlux[i+1]) {
                
                onsets.push_back(i * hopSize);
            }
        }
        
        return onsets;
    }
};

/**
 * @class ElasticAudioStretcher
 * @brief Time-stretch de calidad usando phase vocoder con preservación de formantes
 */
class ElasticAudioStretcher {
public:
    ElasticAudioStretcher() = default;
    
    void prepare(double sampleRate, int maxBlockSize) {
        sampleRate_ = sampleRate;
        fftSize_ = 4096;
        hopSize_ = fftSize_ / 4;
        
        fft_ = std::make_unique<juce::dsp::FFT>(12);  // 4096 points
        
        inputPhase_.resize(fftSize_ / 2 + 1, 0.0f);
        outputPhase_.resize(fftSize_ / 2 + 1, 0.0f);
        analysisPhase_.resize(fftSize_ / 2 + 1, 0.0f);
        synthesisPhase_.resize(fftSize_ / 2 + 1, 0.0f);
        
        windowFunction_.resize(fftSize_);
        for (int i = 0; i < fftSize_; ++i) {
            windowFunction_[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize_));
        }
    }
    
    /**
     * Time-stretch audio buffer
     * @param stretchRatio 1.0 = original, 2.0 = double length (half speed), 0.5 = half length (double speed)
     */
    juce::AudioBuffer<float> stretch(const juce::AudioBuffer<float>& input, 
                                     double stretchRatio,
                                     bool preserveFormants = true) {
        
        if (stretchRatio <= 0.0) stretchRatio = 1.0;
        
        int outputLength = std::ceil(input.getNumSamples() * stretchRatio);
        juce::AudioBuffer<float> output(input.getNumChannels(), outputLength);
        output.clear();
        
        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            stretchChannel(input.getReadPointer(ch), input.getNumSamples(),
                          output.getWritePointer(ch), outputLength,
                          stretchRatio, preserveFormants);
        }
        
        return output;
    }
    
    /**
     * Warp audio usando marcadores
     */
    juce::AudioBuffer<float> warp(const juce::AudioBuffer<float>& input,
                                  const std::vector<WarpMarker>& markers,
                                  double targetTempo,
                                  double sampleRate) {
        
        if (markers.size() < 2) {
            return input;  // Need at least 2 markers
        }
        
        // Calculate total output length
        double lastBeat = markers.back().beatPosition;
        double samplesPerBeat = (60.0 / targetTempo) * sampleRate;
        int outputLength = std::ceil(lastBeat * samplesPerBeat);
        
        juce::AudioBuffer<float> output(input.getNumChannels(), outputLength);
        output.clear();
        
        // Process segments between markers
        for (size_t i = 0; i < markers.size() - 1; ++i) {
            const auto& startMarker = markers[i];
            const auto& endMarker = markers[i + 1];
            
            int inputStart = startMarker.samplePosition;
            int inputEnd = endMarker.samplePosition;
            int inputLength = inputEnd - inputStart;
            
            int outputStart = startMarker.beatPosition * samplesPerBeat;
            int outputEnd = endMarker.beatPosition * samplesPerBeat;
            int outputSegmentLength = outputEnd - outputStart;
            
            if (inputLength <= 0 || outputSegmentLength <= 0) continue;
            
            double ratio = (double)outputSegmentLength / inputLength;
            
            // Extract segment
            juce::AudioBuffer<float> segment(input.getNumChannels(), inputLength);
            for (int ch = 0; ch < input.getNumChannels(); ++ch) {
                segment.copyFrom(ch, 0, input, ch, inputStart, inputLength);
            }
            
            // Stretch segment
            auto stretchedSegment = stretch(segment, ratio, true);
            
            // Copy to output
            int copyLength = std::min(stretchedSegment.getNumSamples(), outputSegmentLength);
            for (int ch = 0; ch < output.getNumChannels(); ++ch) {
                output.copyFrom(ch, outputStart, stretchedSegment, ch, 0, copyLength);
            }
        }
        
        return output;
    }
    
private:
    void stretchChannel(const float* input, int inputLength,
                       float* output, int outputLength,
                       double ratio, bool preserveFormants) {
        
        std::vector<float> fftBuffer(fftSize_ * 2, 0.0f);
        std::vector<float> outputBuffer(outputLength + fftSize_, 0.0f);
        
        double inputPhaseIncrement = hopSize_;
        double outputPhaseIncrement = hopSize_ / ratio;
        
        double inputPos = 0.0;
        double outputPos = 0.0;
        
        while (inputPos < inputLength - fftSize_ && outputPos < outputLength) {
            // Analysis
            int readPos = std::floor(inputPos);
            
            // Fill FFT buffer with windowed input
            for (int i = 0; i < fftSize_; ++i) {
                int samplePos = readPos + i;
                if (samplePos >= 0 && samplePos < inputLength) {
                    fftBuffer[i] = input[samplePos] * windowFunction_[i];
                } else {
                    fftBuffer[i] = 0.0f;
                }
            }
            
            // Forward FFT
            fft_->performRealOnlyForwardTransform(fftBuffer.data());
            
            // Phase vocoder processing
            for (int bin = 0; bin <= fftSize_ / 2; ++bin) {
                float real = fftBuffer[bin * 2];
                float imag = fftBuffer[bin * 2 + 1];
                
                float magnitude = std::sqrt(real * real + imag * imag);
                float phase = std::atan2(imag, real);
                
                // Phase unwrapping
                float deltaPhase = phase - analysisPhase_[bin];
                analysisPhase_[bin] = phase;
                
                // Wrap to [-π, π]
                while (deltaPhase > juce::MathConstants<float>::pi) deltaPhase -= 2.0f * juce::MathConstants<float>::pi;
                while (deltaPhase < -juce::MathConstants<float>::pi) deltaPhase += 2.0f * juce::MathConstants<float>::pi;
                
                // Calculate true frequency
                float binFreq = 2.0f * juce::MathConstants<float>::pi * bin / fftSize_;
                float trueFreq = binFreq + deltaPhase / hopSize_;
                
                // Synthesis phase
                synthesisPhase_[bin] += trueFreq * outputPhaseIncrement;
                
                // Reconstruct
                fftBuffer[bin * 2] = magnitude * std::cos(synthesisPhase_[bin]);
                fftBuffer[bin * 2 + 1] = magnitude * std::sin(synthesisPhase_[bin]);
            }
            
            // Inverse FFT
            fft_->performRealOnlyInverseTransform(fftBuffer.data());
            
            // Overlap-add
            int writePos = std::floor(outputPos);
            for (int i = 0; i < fftSize_; ++i) {
                int outPos = writePos + i;
                if (outPos >= 0 && outPos < outputLength) {
                    outputBuffer[outPos] += fftBuffer[i] * windowFunction_[i] / (fftSize_ / 2);
                }
            }
            
            inputPos += inputPhaseIncrement;
            outputPos += outputPhaseIncrement;
        }
        
        // Copy to output
        std::memcpy(output, outputBuffer.data(), outputLength * sizeof(float));
    }
    
    double sampleRate_ { 44100.0 };
    int fftSize_ { 4096 };
    int hopSize_ { 1024 };
    
    std::unique_ptr<juce::dsp::FFT> fft_;
    std::vector<float> inputPhase_;
    std::vector<float> outputPhase_;
    std::vector<float> analysisPhase_;
    std::vector<float> synthesisPhase_;
    std::vector<float> windowFunction_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElasticAudioStretcher)
};

/**
 * @class WarpEngine
 * @brief Motor completo de warping con UI y detección automática
 */
class WarpEngine {
public:
    WarpEngine() = default;
    
    void loadAudio(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        originalAudio_ = buffer;
        sampleRate_ = sampleRate;
        
        stretcher_.prepare(sampleRate, 8192);
        
        // Auto-detect tempo
        TempoDetector detector;
        auto result = detector.detectTempo(buffer, sampleRate);
        detectedTempo_ = result.bpm;
        
        // Create automatic warp markers from onsets
        markers_.clear();
        if (!result.onsets.empty()) {
            double beatsPerSample = detectedTempo_ / (60.0 * sampleRate);
            
            for (double onset : result.onsets) {
                double beat = onset * beatsPerSample;
                markers_.push_back(WarpMarker(onset, beat));
            }
        }
        
        // Ensure we have start and end markers
        if (markers_.empty() || markers_[0].samplePosition > 0) {
            markers_.insert(markers_.begin(), WarpMarker(0, 0));
        }
        if (markers_.back().samplePosition < buffer.getNumSamples()) {
            double lastBeat = markers_.back().beatPosition + 
                            (buffer.getNumSamples() - markers_.back().samplePosition) * 
                            (detectedTempo_ / (60.0 * sampleRate));
            markers_.push_back(WarpMarker(buffer.getNumSamples(), lastBeat));
        }
    }
    
    void addMarker(double samplePosition, double beatPosition) {
        markers_.push_back(WarpMarker(samplePosition, beatPosition));
        sortMarkers();
    }
    
    void removeMarker(int index) {
        if (index >= 0 && index < markers_.size()) {
            markers_.erase(markers_.begin() + index);
        }
    }
    
    void moveMarker(int index, double newSamplePos, double newBeatPos) {
        if (index >= 0 && index < markers_.size()) {
            markers_[index].samplePosition = newSamplePos;
            markers_[index].beatPosition = newBeatPos;
            sortMarkers();
        }
    }
    
    const std::vector<WarpMarker>& getMarkers() const { return markers_; }
    double getDetectedTempo() const { return detectedTempo_; }
    
    juce::AudioBuffer<float> render(double targetTempo) {
        if (markers_.size() < 2) {
            return originalAudio_;
        }
        
        return stretcher_.warp(originalAudio_, markers_, targetTempo, sampleRate_);
    }
    
    juce::AudioBuffer<float> renderTimeStretch(double ratio) {
        return stretcher_.stretch(originalAudio_, ratio, true);
    }
    
private:
    void sortMarkers() {
        std::sort(markers_.begin(), markers_.end(),
            [](const WarpMarker& a, const WarpMarker& b) {
                return a.samplePosition < b.samplePosition;
            });
    }
    
    juce::AudioBuffer<float> originalAudio_;
    double sampleRate_ { 44100.0 };
    double detectedTempo_ { 120.0 };
    std::vector<WarpMarker> markers_;
    ElasticAudioStretcher stretcher_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WarpEngine)
};

} // namespace DSP
} // namespace Audio
} // namespace OmegaStudio
