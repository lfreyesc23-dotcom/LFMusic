/*
  ==============================================================================

    AdvancedVisualizers.h
    Goniometer, Spectrogram, and advanced visual analysis tools

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <deque>

namespace omega {
namespace Analysis {

/**
 * @brief Goniometer (Lissajous phase scope) for stereo field visualization
 */
class Goniometer {
public:
    Goniometer();
    ~Goniometer() = default;
    
    void initialize(double sampleRate, int maxHistoryMs = 200);
    void setSampleRate(double newSampleRate);
    
    // Processing
    void process(const juce::AudioBuffer<float>& buffer);
    void clear();
    
    // Get visualization data
    struct Point { float x, y; };
    const std::vector<Point>& getPoints() const { return points_; }
    
    // Phase correlation
    float getCorrelation() const { return correlation_; }
    float getLeftLevel() const { return leftLevel_; }
    float getRightLevel() const { return rightLevel_; }
    
    // Settings
    void setDecayRate(float rate) { decayRate_ = juce::jlimit(0.0f, 1.0f, rate); }
    void setMaxPoints(int count) { maxPoints_ = count; }
    void setDisplayMode(int mode) { displayMode_ = mode; } // 0=XY, 1=MS
    
private:
    void calculateCorrelation(const juce::AudioBuffer<float>& buffer);
    
    double sampleRate_ = 48000.0;
    std::vector<Point> points_;
    int maxPoints_ = 1000;
    float decayRate_ = 0.95f;
    int displayMode_ = 0;
    
    // Analysis
    float correlation_ = 0.0f;
    float leftLevel_ = 0.0f;
    float rightLevel_ = 0.0f;
    
    // History buffer
    juce::AudioBuffer<float> historyBuffer_;
    int historyWritePos_ = 0;
    int maxHistorySamples_ = 9600; // 200ms at 48kHz
};

/**
 * @brief Spectrogram (time-frequency visualization)
 */
class Spectrogram {
public:
    Spectrogram();
    ~Spectrogram() = default;
    
    void initialize(double sampleRate, int fftSize = 2048);
    void setSampleRate(double newSampleRate);
    void setFFTSize(int size);
    
    // Processing
    void process(const juce::AudioBuffer<float>& buffer);
    void clear();
    
    // Get visualization data
    struct SpectrogramLine {
        std::vector<float> magnitudes; // dB values
        juce::int64 timestamp;
    };
    
    const std::deque<SpectrogramLine>& getLines() const { return lines_; }
    int getNumBins() const { return fftSize_ / 2; }
    float getFrequencyForBin(int bin) const;
    
    // Settings
    void setMaxLines(int count) { maxLines_ = count; }
    void setColorMap(int map) { colorMap_ = map; } // 0=Jet, 1=Hot, 2=Grayscale
    void setDbRange(float min, float max) { minDb_ = min; maxDb_ = max; }
    
    // Color mapping
    juce::Colour getColorForDb(float db) const;
    
private:
    void performFFT(const juce::AudioBuffer<float>& buffer);
    
    double sampleRate_ = 48000.0;
    int fftSize_ = 2048;
    int maxLines_ = 100;
    
    // FFT
    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;
    std::vector<float> fftData_;
    
    // Spectrogram data
    std::deque<SpectrogramLine> lines_;
    int sampleCounter_ = 0;
    int samplesPerLine_ = 2048;
    
    // Display settings
    int colorMap_ = 0;
    float minDb_ = -80.0f;
    float maxDb_ = 0.0f;
};

/**
 * @brief Vectorscope (phase + amplitude visualization)
 */
class Vectorscope {
public:
    Vectorscope();
    ~Vectorscope() = default;
    
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void clear();
    
    // Visualization data
    struct VectorPoint {
        float angle;      // Radians
        float magnitude;  // 0-1
        float hue;        // For frequency coloring
    };
    
    const std::vector<VectorPoint>& getPoints() const { return points_; }
    
    // Settings
    void setPersistence(float ms) { persistenceMs_ = ms; }
    void setFrequencyColoring(bool enable) { frequencyColoring_ = enable; }
    
private:
    std::vector<VectorPoint> points_;
    float persistenceMs_ = 100.0f;
    bool frequencyColoring_ = false;
};

/**
 * @brief Combined visualization panel
 */
class VisualizationSuite {
public:
    VisualizationSuite();
    ~VisualizationSuite() = default;
    
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void clear();
    
    // Access individual visualizers
    Goniometer& getGoniometer() { return goniometer_; }
    Spectrogram& getSpectrogram() { return spectrogram_; }
    Vectorscope& getVectorscope() { return vectorscope_; }
    
    const Goniometer& getGoniometer() const { return goniometer_; }
    const Spectrogram& getSpectrogram() const { return spectrogram_; }
    const Vectorscope& getVectorscope() const { return vectorscope_; }
    
    // Global settings
    void setUpdateRate(float hz) { updateRateHz_ = hz; }
    
private:
    Goniometer goniometer_;
    Spectrogram spectrogram_;
    Vectorscope vectorscope_;
    
    float updateRateHz_ = 30.0f;
    int updateCounter_ = 0;
};

/**
 * @brief Phase correlation meter
 */
class CorrelationMeter {
public:
    CorrelationMeter();
    ~CorrelationMeter() = default;
    
    void initialize(double sampleRate);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    
    // Get correlation values
    float getInstantaneous() const { return instantaneous_; }
    float getShortTerm() const { return shortTerm_; }      // 1 second
    float getIntegrated() const { return integrated_; }     // Overall
    
    // Stereo width estimation
    float getStereoWidth() const;
    
private:
    void updateCorrelation(float leftSample, float rightSample);
    
    double sampleRate_ = 48000.0;
    
    float instantaneous_ = 0.0f;
    float shortTerm_ = 0.0f;
    float integrated_ = 0.0f;
    
    // Buffers for averaging
    std::vector<float> shortTermBuffer_;
    int shortTermIndex_ = 0;
    double integratedSum_ = 0.0;
    juce::int64 integratedSamples_ = 0;
};

} // namespace Analysis
} // namespace omega
