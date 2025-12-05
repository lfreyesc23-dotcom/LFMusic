/*
  ==============================================================================
    SpectralAnalyzer.h
    
    Análisis espectral profesional:
    - FFT Analyzer (realtime spectrum)
    - Spectrogram (waterfall display)
    - Correlation Meter (phase)
    - LUFS Metering (loudness)
    - Vectorscope (stereo imaging)
    - Peak/RMS Meter
    
    Professional audio analysis tools
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <deque>
#include <array>

namespace OmegaStudio {

//==============================================================================
/** FFT Analyzer - Real-time spectrum */
class FFTAnalyzer {
public:
    static constexpr int fftOrder = 12;      // 4096 samples
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int numBins = fftSize / 2;
    
    FFTAnalyzer();
    ~FFTAnalyzer();
    
    void prepareToPlay(double sampleRate);
    void pushSamples(const juce::AudioBuffer<float>& buffer);
    
    // Get magnitude spectrum (dB)
    const std::vector<float>& getMagnitudeSpectrum() const { return magnitudeSpectrum; }
    
    // Get frequency for bin
    float getFrequencyForBin(int bin) const;
    int getBinForFrequency(float frequency) const;
    
    // Settings
    void setWindowType(juce::dsp::WindowingFunction<float>::WindowingMethod method);
    void setAveraging(float amount);     // 0-1 (smoothing)
    void setMinDb(float minDb) { minDecibels = minDb; }
    void setMaxDb(float maxDb) { maxDecibels = maxDb; }
    
    float getMinDb() const { return minDecibels; }
    float getMaxDb() const { return maxDecibels; }
    
    bool hasNewData() const { return newDataAvailable; }
    void clearNewDataFlag() { newDataAvailable = false; }
    
private:
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float>::WindowingMethod windowType { juce::dsp::WindowingFunction<float>::hann };
    
    std::array<float, fftSize * 2> fftData { 0.0f };
    std::vector<float> magnitudeSpectrum;
    std::vector<float> smoothedSpectrum;
    
    std::vector<float> fifo;
    int fifoIndex { 0 };
    bool newDataAvailable { false };
    
    double sampleRate { 48000.0 };
    float averaging { 0.7f };
    float minDecibels { -100.0f };
    float maxDecibels { 0.0f };
    
    void performFFT();
    void smoothSpectrum();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTAnalyzer)
};

//==============================================================================
/** Spectrogram - Waterfall display */
class Spectrogram {
public:
    Spectrogram(int width = 512, int height = 256);
    
    void prepareToPlay(double sampleRate);
    void pushSpectrum(const std::vector<float>& spectrum);
    
    // Get image data for rendering
    const juce::Image& getImage() const { return spectrogramImage; }
    
    // Settings
    void setSize(int width, int height);
    void setColourMap(const std::vector<juce::Colour>& colours);
    
private:
    juce::Image spectrogramImage;
    std::deque<std::vector<float>> spectrogramHistory;
    
    int width { 512 };
    int height { 256 };
    
    std::vector<juce::Colour> colourMap;
    
    void updateImage();
    juce::Colour getColourForMagnitude(float magnitude) const;
    void createDefaultColourMap();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Spectrogram)
};

//==============================================================================
/** Correlation Meter - Phase relationship */
class CorrelationMeter {
public:
    CorrelationMeter();
    
    void prepareToPlay(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Correlation value: +1 = in phase, 0 = no correlation, -1 = out of phase
    float getCorrelation() const { return correlation; }
    
    // Integration time
    void setIntegrationTime(float seconds);
    
private:
    float correlation { 0.0f };
    float integrationTime { 0.3f };
    double sampleRate { 48000.0 };
    
    float sumLeft { 0.0f };
    float sumRight { 0.0f };
    float sumProduct { 0.0f };
    int sampleCount { 0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CorrelationMeter)
};

//==============================================================================
/** LUFS Meter - Loudness metering (EBU R128 / ITU BS.1770) */
class LUFSMeter {
public:
    LUFSMeter();
    
    void prepareToPlay(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Loudness values
    float getMomentaryLUFS() const { return momentaryLUFS; }    // 400ms
    float getShortTermLUFS() const { return shortTermLUFS; }    // 3s
    float getIntegratedLUFS() const { return integratedLUFS; }  // entire duration
    
    // Loudness Range (LRA)
    float getLRA() const { return lra; }
    
    // True Peak
    float getTruePeakLeft() const { return truePeakLeft; }
    float getTruePeakRight() const { return truePeakRight; }
    
private:
    double sampleRate { 48000.0 };
    
    float momentaryLUFS { -70.0f };
    float shortTermLUFS { -70.0f };
    float integratedLUFS { -70.0f };
    float lra { 0.0f };
    float truePeakLeft { -70.0f };
    float truePeakRight { -70.0f };
    
    // K-weighting filter (IIR)
    juce::dsp::IIR::Filter<float> preFilterLeft, preFilterRight;
    juce::dsp::IIR::Filter<float> rlbFilterLeft, rlbFilterRight;
    
    // Gating blocks
    std::deque<float> momentaryBlocks;
    std::deque<float> shortTermBlocks;
    std::vector<float> allBlocks;
    
    // True peak oversampling
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    
    void updateFilters();
    float calculateLoudness(const std::deque<float>& blocks) const;
    void applyGating();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LUFSMeter)
};

//==============================================================================
/** Vectorscope - Stereo imaging visualization */
class Vectorscope {
public:
    Vectorscope(int size = 512);
    
    void prepareToPlay(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Get point cloud for rendering
    struct Point {
        float x, y;      // -1 to +1
        float intensity; // 0-1
    };
    const std::vector<Point>& getPoints() const { return points; }
    
    // Settings
    void setPersistence(float seconds);
    void setRotation(float degrees);     // Rotate display
    void setScale(float scale);          // Zoom
    
private:
    std::vector<Point> points;
    int maxPoints { 1000 };
    
    float persistence { 0.5f };
    float rotation { 0.0f };
    float scale { 1.0f };
    
    double sampleRate { 48000.0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vectorscope)
};

//==============================================================================
/** Comprehensive Audio Analyzer */
class AudioAnalyzer {
public:
    AudioAnalyzer();
    ~AudioAnalyzer();
    
    void prepareToPlay(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Access to analyzers
    FFTAnalyzer& getFFTAnalyzer() { return fftAnalyzer; }
    const FFTAnalyzer& getFFTAnalyzer() const { return fftAnalyzer; }
    
    Spectrogram& getSpectrogram() { return spectrogram; }
    const Spectrogram& getSpectrogram() const { return spectrogram; }
    
    CorrelationMeter& getCorrelationMeter() { return correlationMeter; }
    const CorrelationMeter& getCorrelationMeter() const { return correlationMeter; }
    
    LUFSMeter& getLUFSMeter() { return lufsMeter; }
    const LUFSMeter& getLUFSMeter() const { return lufsMeter; }
    
    Vectorscope& getVectorscope() { return vectorscope; }
    const Vectorscope& getVectorscope() const { return vectorscope; }
    
    // Enable/disable specific analyzers for CPU optimization
    void setFFTEnabled(bool enabled) { fftEnabled = enabled; }
    void setSpectrogramEnabled(bool enabled) { spectrogramEnabled = enabled; }
    void setCorrelationEnabled(bool enabled) { correlationEnabled = enabled; }
    void setLUFSEnabled(bool enabled) { lufsEnabled = enabled; }
    void setVectorscopeEnabled(bool enabled) { vectorscopeEnabled = enabled; }
    
private:
    FFTAnalyzer fftAnalyzer;
    Spectrogram spectrogram;
    CorrelationMeter correlationMeter;
    LUFSMeter lufsMeter;
    Vectorscope vectorscope;
    
    bool fftEnabled { true };
    bool spectrogramEnabled { true };
    bool correlationEnabled { true };
    bool lufsEnabled { true };
    bool vectorscopeEnabled { true };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioAnalyzer)
};

} // namespace OmegaStudio
