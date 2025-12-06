//==============================================================================
// ParametricEQ.h - 7-Band Parametric EQ con Visualización
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <complex>

namespace OmegaStudio {

//==============================================================================
/** 7-Band Parametric EQ Profesional
 *  - 7 bandas totalmente parametrizables
 *  - Visualización de curva en tiempo real
 *  - Tipos de filtro: Bell, LowShelf, HighShelf, LowCut, HighCut, Notch
 *  - Q variable (0.1 - 20.0)
 *  - Gain ±24 dB
 *  - Análisis de frecuencia FFT para feedback visual
 */
class ParametricEQ {
public:
    //==========================================================================
    enum class FilterType {
        Bell,           // Campana paramétrica (boost/cut)
        LowShelf,       // Estantería graves
        HighShelf,      // Estantería agudos
        LowCut,         // Corte graves (12/24/48 dB/oct)
        HighCut,        // Corte agudos (12/24/48 dB/oct)
        Notch,          // Rechazo de banda
        AllPass         // All-pass (para phase)
    };
    
    enum class Slope {
        dB12,           // 12 dB/octave (2-pole)
        dB24,           // 24 dB/octave (4-pole)
        dB48            // 48 dB/octave (8-pole)
    };
    
    //==========================================================================
    struct Band {
        bool enabled = true;
        FilterType type = FilterType::Bell;
        float frequency = 1000.0f;      // 20 Hz - 20 kHz
        float gain = 0.0f;               // ±24 dB
        float Q = 1.0f;                  // 0.1 - 20.0
        Slope slope = Slope::dB24;       // Para filtros cut
        
        // Coeficientes de filtro IIR (biquad)
        std::array<float, 5> coeffs{0, 0, 0, 0, 0}; // b0, b1, b2, a1, a2
        
        // Estado del filtro (para procesamiento)
        std::array<float, 2> z{0, 0};   // z1, z2
    };
    
    //==========================================================================
    ParametricEQ();
    ~ParametricEQ() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    float processSample(float sample, int bandIndex);
    
    // Band Control
    void setBandEnabled(int bandIndex, bool enabled);
    void setBandType(int bandIndex, FilterType type);
    void setBandFrequency(int bandIndex, float frequency);
    void setBandGain(int bandIndex, float gainDb);
    void setBandQ(int bandIndex, float Q);
    void setBandSlope(int bandIndex, Slope slope);
    
    // Presets
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    juce::StringArray getPresetList() const;
    
    // Visualization
    struct FrequencyResponse {
        std::vector<float> frequencies;   // Log-spaced 20 Hz - 20 kHz
        std::vector<float> magnitudes;    // dB
        std::vector<float> phases;        // radians
    };
    
    FrequencyResponse calculateFrequencyResponse(int numPoints = 512) const;
    std::array<Band, 7>& getBands() { return bands_; }
    const std::array<Band, 7>& getBands() const { return bands_; }
    
    // Analysis
    void analyzeInput(const juce::AudioBuffer<float>& buffer);
    const std::vector<float>& getInputSpectrum() const { return inputSpectrum_; }
    const std::vector<float>& getOutputSpectrum() const { return outputSpectrum_; }
    
private:
    //==========================================================================
    void updateCoefficients(int bandIndex);
    void calculateBellCoeffs(Band& band);
    void calculateShelfCoeffs(Band& band, bool isLow);
    void calculateCutCoeffs(Band& band, bool isLow);
    void calculateNotchCoeffs(Band& band);
    
    float getMagnitudeAt(float frequency, int bandIndex) const;
    float getPhaseAt(float frequency, int bandIndex) const;
    
    //==========================================================================
    std::array<Band, 7> bands_;
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    
    // FFT para análisis espectral
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder; // 4096
    
    juce::dsp::FFT fft_{fftOrder};
    juce::dsp::WindowingFunction<float> window_{fftSize, juce::dsp::WindowingFunction<float>::hann};
    
    std::array<float, fftSize * 2> fftData_{};
    std::vector<float> inputSpectrum_;
    std::vector<float> outputSpectrum_;
    
    // Smooth para cambios de parámetros
    juce::SmoothedValue<float> smoothedGains_[7];
    juce::SmoothedValue<float> smoothedFreqs_[7];
    juce::SmoothedValue<float> smoothedQs_[7];
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQ)
};

//==============================================================================
/** Presets profesionales predefinidos */
class EQPresets {
public:
    static void applyPreset(ParametricEQ& eq, const juce::String& presetName) {
        if (presetName == "Flat") {
            applyFlat(eq);
        } else if (presetName == "Vocal Presence") {
            applyVocalPresence(eq);
        } else if (presetName == "Kick Punch") {
            applyKickPunch(eq);
        } else if (presetName == "Air & Silk") {
            applyAirAndSilk(eq);
        } else if (presetName == "Telephone") {
            applyTelephone(eq);
        } else if (presetName == "Master Bus") {
            applyMasterBus(eq);
        } else if (presetName == "De-Harsh") {
            applyDeHarsh(eq);
        }
    }
    
private:
    static void applyFlat(ParametricEQ& eq) {
        for (int i = 0; i < 7; ++i) {
            eq.setBandEnabled(i, false);
            eq.setBandGain(i, 0.0f);
        }
    }
    
    static void applyVocalPresence(ParametricEQ& eq) {
        // Band 0: Low cut @ 80 Hz
        eq.setBandEnabled(0, true);
        eq.setBandType(0, ParametricEQ::FilterType::LowCut);
        eq.setBandFrequency(0, 80.0f);
        eq.setBandSlope(0, ParametricEQ::Slope::dB24);
        
        // Band 1: Cut low mids @ 250 Hz
        eq.setBandEnabled(1, true);
        eq.setBandType(1, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(1, 250.0f);
        eq.setBandGain(1, -2.5f);
        eq.setBandQ(1, 1.5f);
        
        // Band 2: Boost presence @ 3 kHz
        eq.setBandEnabled(2, true);
        eq.setBandType(2, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(2, 3000.0f);
        eq.setBandGain(2, 3.5f);
        eq.setBandQ(2, 2.0f);
        
        // Band 3: De-ess @ 8 kHz
        eq.setBandEnabled(3, true);
        eq.setBandType(3, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(3, 8000.0f);
        eq.setBandGain(3, -2.0f);
        eq.setBandQ(3, 3.0f);
        
        // Band 4: Air @ 12 kHz
        eq.setBandEnabled(4, true);
        eq.setBandType(4, ParametricEQ::FilterType::HighShelf);
        eq.setBandFrequency(4, 12000.0f);
        eq.setBandGain(4, 2.0f);
        eq.setBandQ(4, 0.7f);
    }
    
    static void applyKickPunch(ParametricEQ& eq) {
        // Band 0: Sub boost @ 60 Hz
        eq.setBandEnabled(0, true);
        eq.setBandType(0, ParametricEQ::FilterType::LowShelf);
        eq.setBandFrequency(0, 60.0f);
        eq.setBandGain(0, 4.0f);
        eq.setBandQ(0, 0.7f);
        
        // Band 1: Cut mud @ 300 Hz
        eq.setBandEnabled(1, true);
        eq.setBandType(1, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(1, 300.0f);
        eq.setBandGain(1, -4.0f);
        eq.setBandQ(1, 1.0f);
        
        // Band 2: Punch @ 2.5 kHz
        eq.setBandEnabled(2, true);
        eq.setBandType(2, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(2, 2500.0f);
        eq.setBandGain(2, 3.0f);
        eq.setBandQ(2, 1.5f);
        
        // Band 3: High cut @ 10 kHz
        eq.setBandEnabled(3, true);
        eq.setBandType(3, ParametricEQ::FilterType::HighCut);
        eq.setBandFrequency(3, 10000.0f);
        eq.setBandSlope(3, ParametricEQ::Slope::dB12);
    }
    
    static void applyAirAndSilk(ParametricEQ& eq) {
        // Band 0: High shelf @ 8 kHz
        eq.setBandEnabled(0, true);
        eq.setBandType(0, ParametricEQ::FilterType::HighShelf);
        eq.setBandFrequency(0, 8000.0f);
        eq.setBandGain(0, 3.5f);
        eq.setBandQ(0, 0.7f);
        
        // Band 1: Air bump @ 15 kHz
        eq.setBandEnabled(1, true);
        eq.setBandType(1, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(1, 15000.0f);
        eq.setBandGain(1, 2.5f);
        eq.setBandQ(1, 1.0f);
    }
    
    static void applyTelephone(ParametricEQ& eq) {
        // Band 0: Low cut @ 500 Hz
        eq.setBandEnabled(0, true);
        eq.setBandType(0, ParametricEQ::FilterType::LowCut);
        eq.setBandFrequency(0, 500.0f);
        eq.setBandSlope(0, ParametricEQ::Slope::dB48);
        
        // Band 1: High cut @ 3 kHz
        eq.setBandEnabled(1, true);
        eq.setBandType(1, ParametricEQ::FilterType::HighCut);
        eq.setBandFrequency(1, 3000.0f);
        eq.setBandSlope(1, ParametricEQ::Slope::dB48);
    }
    
    static void applyMasterBus(ParametricEQ& eq) {
        // Band 0: Sub control @ 30 Hz
        eq.setBandEnabled(0, true);
        eq.setBandType(0, ParametricEQ::FilterType::HighShelf);
        eq.setBandFrequency(0, 30.0f);
        eq.setBandGain(0, -1.5f);
        eq.setBandQ(0, 0.7f);
        
        // Band 1: Low mids @ 200 Hz
        eq.setBandEnabled(1, true);
        eq.setBandType(1, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(1, 200.0f);
        eq.setBandGain(1, 1.0f);
        eq.setBandQ(1, 1.0f);
        
        // Band 2: Presence @ 4 kHz
        eq.setBandEnabled(2, true);
        eq.setBandType(2, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(2, 4000.0f);
        eq.setBandGain(2, 1.5f);
        eq.setBandQ(2, 1.5f);
        
        // Band 3: Air @ 16 kHz
        eq.setBandEnabled(3, true);
        eq.setBandType(3, ParametricEQ::FilterType::HighShelf);
        eq.setBandFrequency(3, 16000.0f);
        eq.setBandGain(3, 1.0f);
        eq.setBandQ(3, 0.7f);
    }
    
    static void applyDeHarsh(ParametricEQ& eq) {
        // Band 0: Cut @ 3 kHz
        eq.setBandEnabled(0, true);
        eq.setBandType(0, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(0, 3000.0f);
        eq.setBandGain(0, -3.0f);
        eq.setBandQ(0, 3.0f);
        
        // Band 1: Cut @ 6 kHz
        eq.setBandEnabled(1, true);
        eq.setBandType(1, ParametricEQ::FilterType::Bell);
        eq.setBandFrequency(1, 6000.0f);
        eq.setBandGain(1, -2.5f);
        eq.setBandQ(1, 2.5f);
    }
};

} // namespace OmegaStudio
