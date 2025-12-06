//==============================================================================
// VisualMeteringSystem.h - Metering Profesional (LUFS, True Peak, Phase)
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

namespace OmegaStudio {

//==============================================================================
/** Sistema de Metering Profesional
 *  - LUFS (EBU R128): Momentary, Short-term, Integrated
 *  - True Peak detection (inter-sample peaks)
 *  - Phase correlation meter
 *  - K-System metering (K-12, K-14, K-20)
 *  - VU meter emulation
 *  - Stereo imaging visualizer
 */
class ProfessionalMeter {
public:
    //==========================================================================
    enum class MeterType {
        Peak,           // Peak meter (dBFS)
        RMS,            // RMS meter (dBFS)
        LUFS,           // Loudness (LUFS)
        TruePeak,       // True peak (dBTP)
        VU,             // VU meter (-20 to +3 VU)
        KSystem         // K-System (K-12, K-14, K-20)
    };
    
    enum class KSystemScale {
        K12,            // -12 dBFS = 0 VU (broadcast)
        K14,            // -14 dBFS = 0 VU (mastering)
        K20             // -20 dBFS = 0 VU (film)
    };
    
    //==========================================================================
    struct MeterData {
        // Peak levels
        float peakLeft = -100.0f;       // dBFS
        float peakRight = -100.0f;
        
        // RMS levels
        float rmsLeft = -100.0f;
        float rmsRight = -100.0f;
        
        // LUFS
        float lufsMomentary = -70.0f;   // 400ms
        float lufsShortTerm = -70.0f;   // 3 seconds
        float lufsIntegrated = -70.0f;  // Gated, full track
        float lufsRange = 0.0f;         // Dynamic range (LU)
        
        // True Peak
        float truePeakLeft = -100.0f;   // dBTP
        float truePeakRight = -100.0f;
        
        // Phase correlation
        float phaseCorrelation = 0.0f;  // -1 (out of phase) to +1 (mono)
        
        // Stereo width
        float stereoWidth = 0.0f;       // 0.0 (mono) to 1.0 (wide)
        
        // Peaks hold
        float peakHoldLeft = -100.0f;
        float peakHoldRight = -100.0f;
        float peakHoldTime = 0.0f;      // seconds
        
        // Clipping detection
        int clippedSamplesLeft = 0;
        int clippedSamplesRight = 0;
        bool isClipping = false;
    };
    
    //==========================================================================
    ProfessionalMeter();
    ~ProfessionalMeter() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Processing
    void process(const juce::AudioBuffer<float>& buffer);
    
    // Get meter data
    const MeterData& getMeterData() const { return meterData_; }
    
    // Settings
    void setMeterType(MeterType type) { meterType_ = type; }
    MeterType getMeterType() const { return meterType_; }
    
    void setKSystemScale(KSystemScale scale) { kSystemScale_ = scale; }
    KSystemScale getKSystemScale() const { return kSystemScale_; }
    
    void setPeakHoldTime(float seconds) { peakHoldTime_ = seconds; }
    void setIntegrationTime(float seconds) { integrationTime_ = seconds; }
    
    // Reset specific meters
    void resetLUFS();
    void resetPeakHold();
    void resetClipping();
    
private:
    //==========================================================================
    MeterData meterData_;
    MeterType meterType_ = MeterType::Peak;
    KSystemScale kSystemScale_ = KSystemScale::K14;
    
    double sampleRate_ = 48000.0;
    float peakHoldTime_ = 2.0f;
    float integrationTime_ = 3.0f;
    
    // LUFS calculation (EBU R128)
    class LUFSProcessor {
    public:
        void prepare(double sampleRate);
        void process(const juce::AudioBuffer<float>& buffer);
        
        float getMomentary() const { return momentary_; }
        float getShortTerm() const { return shortTerm_; }
        float getIntegrated() const { return integrated_; }
        float getRange() const { return range_; }
        
        void reset();
        
    private:
        double sampleRate_ = 48000.0;
        
        // K-weighting filters (pre-filter + RLB filter)
        juce::dsp::IIR::Filter<float> preFilterLeft_;
        juce::dsp::IIR::Filter<float> preFilterRight_;
        juce::dsp::IIR::Filter<float> rlbFilterLeft_;
        juce::dsp::IIR::Filter<float> rlbFilterRight_;
        
        // Measurement windows
        std::vector<float> momentaryWindow_;  // 400ms
        std::vector<float> shortTermWindow_;  // 3s
        std::vector<float> integratedBuffer_; // Entire track
        
        float momentary_ = -70.0f;
        float shortTerm_ = -70.0f;
        float integrated_ = -70.0f;
        float range_ = 0.0f;
        
        int windowPos_ = 0;
        
        void updateLoudness();
        float calculateLoudness(const std::vector<float>& samples) const;
        void updateIntegrated();
    };
    
    LUFSProcessor lufsProcessor_;
    
    // True peak detection (4x oversampling)
    juce::dsp::Oversampling<float> oversampling_{2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};
    
    // Phase correlation
    float calculatePhaseCorrelation(const juce::AudioBuffer<float>& buffer);
    
    // Peak hold timer
    float peakHoldTimer_ = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfessionalMeter)
};

//==============================================================================
/** Color Coding System - Sistema de colores para tracks/clips */
class ColorCodingSystem {
public:
    //==========================================================================
    struct ColorScheme {
        juce::String name;
        std::map<juce::String, juce::Colour> colors;
    };
    
    //==========================================================================
    ColorCodingSystem();
    ~ColorCodingSystem() = default;
    
    // Color assignment
    void setItemColor(int itemId, const juce::Colour& color);
    void setItemColor(int itemId, const juce::String& colorName);
    
    juce::Colour getItemColor(int itemId) const;
    bool hasColor(int itemId) const;
    
    void clearItemColor(int itemId);
    void clearAllColors();
    
    // Predefined colors
    juce::Colour getColorByName(const juce::String& name) const;
    juce::StringArray getAvailableColorNames() const;
    
    // Color schemes
    void loadScheme(const juce::String& schemeName);
    void saveScheme(const juce::String& schemeName);
    
    juce::StringArray getAvailableSchemes() const;
    
    // Auto-coloring
    void autoColorByType(const juce::String& type, const juce::Colour& color);
    void autoColorByIndex(int startIndex, bool cycle = true);
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::map<int, juce::Colour> itemColors_;
    
    // Predefined colors (FL Studio style)
    static const std::map<juce::String, juce::Colour> defaultColors_;
    
    // Color schemes
    std::map<juce::String, ColorScheme> schemes_;
    
    void initializeDefaultColors();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColorCodingSystem)
};

//==============================================================================
/** Icon Library - Iconografía consistente */
class IconLibrary {
public:
    //==========================================================================
    enum class IconType {
        // Transport
        Play, Pause, Stop, Record, Loop,
        
        // Tools
        Pencil, Eraser, Select, Slice, Mute,
        
        // Views
        Mixer, Playlist, PianoRoll, Browser,
        
        // Effects
        EQ, Compressor, Reverb, Delay,
        
        // File
        Save, Load, Export, Import,
        
        // Edit
        Cut, Copy, Paste, Undo, Redo,
        
        // Navigation
        ZoomIn, ZoomOut, Home, Settings
    };
    
    //==========================================================================
    static juce::Path getIcon(IconType type, float size = 24.0f);
    static juce::String getIconEmoji(IconType type);
    
    // Custom icons
    static void registerCustomIcon(const juce::String& name, const juce::Path& path);
    static juce::Path getCustomIcon(const juce::String& name);
    
private:
    static std::map<juce::String, juce::Path> customIcons_;
};

//==============================================================================
/** Theme System - Dark/Light themes completos */
class ThemeSystem {
public:
    //==========================================================================
    enum class ThemeMode {
        Dark,
        Light,
        Auto            // Sigue el sistema
    };
    
    struct Theme {
        juce::String name;
        ThemeMode mode;
        
        // Colors
        juce::Colour background;
        juce::Colour foreground;
        juce::Colour accent;
        juce::Colour text;
        juce::Colour textSecondary;
        juce::Colour border;
        juce::Colour highlight;
        juce::Colour error;
        juce::Colour warning;
        juce::Colour success;
        
        // Component-specific
        juce::Colour buttonBackground;
        juce::Colour buttonHover;
        juce::Colour buttonActive;
        
        juce::Colour trackBackground;
        juce::Colour clipBackground;
        
        juce::Colour gridLines;
        juce::Colour waveform;
        juce::Colour meters;
    };
    
    //==========================================================================
    ThemeSystem();
    ~ThemeSystem() = default;
    
    // Theme management
    void setTheme(const juce::String& themeName);
    void setThemeMode(ThemeMode mode);
    
    const Theme& getCurrentTheme() const { return currentTheme_; }
    ThemeMode getThemeMode() const { return themeMode_; }
    
    // Available themes
    juce::StringArray getAvailableThemes() const;
    
    // Custom themes
    void registerTheme(const Theme& theme);
    void saveCurrentTheme(const juce::String& name);
    
    // Apply to components
    void applyToLookAndFeel(juce::LookAndFeel& laf);
    
    // Listeners
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void themeChanged(const Theme& newTheme) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    Theme currentTheme_;
    ThemeMode themeMode_ = ThemeMode::Dark;
    
    std::map<juce::String, Theme> themes_;
    juce::ListenerList<Listener> listeners_;
    
    void initializeDefaultThemes();
    void notifyThemeChanged();
    
    // System theme detection
    bool isSystemDarkMode() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeSystem)
};

} // namespace OmegaStudio
