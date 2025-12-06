//==============================================================================
// SpectrumAnalyzer.h - Analizador de Espectro en Tiempo Real
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>

namespace OmegaStudio {

//==============================================================================
/** Analizador de Espectro Profesional en Tiempo Real
 *  - FFT de alta resolución (hasta 32768 samples)
 *  - Múltiples modos de visualización
 *  - Peak hold y decay
 *  - Análisis estéreo (L/R/Mid/Side)
 *  - Sonogram/Spectrogram mode
 *  - Frequency guides (notas musicales)
 */
class SpectrumAnalyzer {
public:
    //==========================================================================
    enum class DisplayMode {
        Bars,           // Barras clásicas
        Line,           // Línea continua
        Filled,         // Área rellena
        Sonogram        // Espectrograma (cascada)
    };
    
    enum class ChannelMode {
        Stereo,         // L+R combinados
        Left,           // Solo canal izquierdo
        Right,          // Solo canal derecho
        Mid,            // Componente Mid (L+R)
        Side            // Componente Side (L-R)
    };
    
    enum class FrequencyScale {
        Linear,         // Escala lineal
        Logarithmic     // Escala logarítmica (musical)
    };
    
    //==========================================================================
    struct Settings {
        // FFT
        int fftOrder = 13;              // 2^13 = 8192 samples
        int numBands = 128;             // Número de bandas de frecuencia
        
        // Display
        DisplayMode displayMode = DisplayMode::Filled;
        ChannelMode channelMode = ChannelMode::Stereo;
        FrequencyScale frequencyScale = FrequencyScale::Logarithmic;
        
        // Range
        float minFrequency = 20.0f;     // Hz
        float maxFrequency = 20000.0f;  // Hz
        float minDb = -90.0f;           // dB
        float maxDb = 0.0f;             // dB
        
        // Smoothing
        float smoothing = 0.7f;         // 0.0 - 1.0
        float peakHold = 2.0f;          // segundos
        float peakDecay = 1.0f;         // dB/segundo
        
        // Visual
        bool showPeakHold = true;
        bool showGrid = true;
        bool showFrequencyLabels = true;
        bool showDbLabels = true;
        bool showMusicalNotes = true;   // Marcar notas musicales
        
        // Colors
        juce::Colour primaryColor = juce::Colours::orange;
        juce::Colour secondaryColor = juce::Colours::grey;
        juce::Colour gridColor = juce::Colour(0x40FFFFFF);
    };
    
    //==========================================================================
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void pushBuffer(const juce::AudioBuffer<float>& buffer);
    
    // Settings
    void setSettings(const Settings& settings);
    Settings& getSettings() { return settings_; }
    const Settings& getSettings() const { return settings_; }
    
    // Data Access (para renderizado)
    const std::vector<float>& getMagnitudeSpectrum() const { return magnitudeSpectrum_; }
    const std::vector<float>& getPeakSpectrum() const { return peakSpectrum_; }
    const std::vector<float>& getFrequencies() const { return frequencies_; }
    
    // Sonogram data
    const std::vector<std::vector<float>>& getSonogramData() const { return sonogramData_; }
    
    // Análisis de banda específica
    float getMagnitudeAt(float frequency) const;
    float getPeakAt(float frequency) const;
    
    // Detectar pitch dominante
    float getDominantFrequency() const;
    
private:
    //==========================================================================
    Settings settings_;
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    
    // FFT
    static constexpr int maxFftOrder = 15;  // 2^15 = 32768
    static constexpr int maxFftSize = 1 << maxFftOrder;
    
    juce::dsp::FFT fft_{13};
    juce::dsp::WindowingFunction<float> window_{1 << 13, juce::dsp::WindowingFunction<float>::hann};
    
    std::array<float, maxFftSize * 2> fftData_{};
    juce::AudioBuffer<float> fifo_;
    int fifoPos_ = 0;
    
    // Spectrum data
    std::vector<float> magnitudeSpectrum_;
    std::vector<float> peakSpectrum_;
    std::vector<float> frequencies_;
    std::vector<float> peakHoldTimes_;
    
    // Sonogram (para modo cascada)
    std::vector<std::vector<float>> sonogramData_;
    static constexpr int sonogramHistory = 256;
    
    // Processing
    void processFFT();
    void updateMagnitudeSpectrum();
    void updatePeakSpectrum(float deltaTime);
    void updateSonogram();
    
    // Helpers
    int frequencyToBin(float frequency) const;
    float binToFrequency(int bin) const;
    float magnitudeToDb(float magnitude) const;
    
    // Smoothing
    juce::SmoothedValue<float> smoothedMagnitudes_[maxFftSize / 2];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};

} // namespace OmegaStudio
