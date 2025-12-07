#pragma once
#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio {
namespace Visualizers {

/**
 * @brief Wave Candy - Real-time Audio Visualizer (FL Studio)
 * Waveforms, spectrums, vectorscopes, peak metering
 */
class WaveCandy : public juce::Component, public juce::Timer
{
public:
    enum class DisplayMode {
        Waveform,
        Spectrum,
        Spectrogram,
        Vectorscope,
        PeakMeter,
        PhaseScope,
        Goniometer
    };
    
    WaveCandy();
    ~WaveCandy() override;
    
    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return currentMode; }
    
    void processAudio(const juce::AudioBuffer<float>& buffer);
    
    // Waveform settings
    void setWaveformScale(float scale) { waveformScale = scale; }
    void setWaveformColour(juce::Colour colour) { waveformColour = colour; }
    
    // Spectrum settings
    void setFFTSize(int size);
    void setSpectrumRange(float minDB, float maxDB);
    void setSpectrumColour(juce::Colour colour) { spectrumColour = colour; }
    
    // Spectrogram settings
    void setSpectrogramSpeed(float speed) { spectrogramSpeed = speed; }
    void setSpectrogramColourScheme(int scheme) { colourScheme = scheme; }
    
    // Update rate
    void setRefreshRate(int hz);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    DisplayMode currentMode = DisplayMode::Spectrum;
    
    // Waveform
    std::vector<float> waveformData;
    float waveformScale = 1.0f;
    juce::Colour waveformColour = juce::Colours::green;
    
    // Spectrum
    juce::dsp::FFT fft{11};
    std::vector<float> fftData;
    std::vector<float> spectrumData;
    float minDB = -100.0f;
    float maxDB = 0.0f;
    juce::Colour spectrumColour = juce::Colours::cyan;
    
    // Spectrogram
    std::vector<std::vector<float>> spectrogramData;
    float spectrogramSpeed = 1.0f;
    int colourScheme = 0;
    
    // Vectorscope
    std::vector<juce::Point<float>> vectorscopePoints;
    
    // Peak meter
    float peakL = 0.0f, peakR = 0.0f;
    float rmsL = 0.0f, rmsR = 0.0f;
    
    void drawWaveform(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g);
    void drawSpectrogram(juce::Graphics& g);
    void drawVectorscope(juce::Graphics& g);
    void drawPeakMeter(juce::Graphics& g);
    void drawPhaseScope(juce::Graphics& g);
    void drawGoniometer(juce::Graphics& g);
    
    juce::Colour getSpectrogramColour(float value) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveCandy)
};

/**
 * @brief Fruity Spectroman - Spectrum Analyzer (FL Studio)
 */
class FruitySpectroman : public juce::Component, public juce::Timer
{
public:
    FruitySpectroman();
    ~FruitySpectroman() override;
    
    void processAudio(const juce::AudioBuffer<float>& buffer);
    
    void setResolution(int fftOrder);
    void setPeakHold(bool enabled) { peakHoldEnabled = enabled; }
    void setShowGrid(bool show) { showGrid = show; }
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::dsp::FFT fft{12};
    std::vector<float> fftData;
    std::vector<float> spectrumData;
    std::vector<float> peakData;
    bool peakHoldEnabled = true;
    bool showGrid = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FruitySpectroman)
};

/**
 * @brief Fruity Video Player - Video Playback (FL Studio)
 */
class FruityVideoPlayer : public juce::Component
{
public:
    FruityVideoPlayer();
    ~FruityVideoPlayer() override;
    
    bool loadVideo(const juce::File& file);
    void play();
    void pause();
    void stop();
    void seek(double seconds);
    
    void setPlaybackPosition(double beats);
    double getPlaybackPosition() const { return currentPosition; }
    
    void setVolume(float volume);
    float getVolume() const { return videoVolume; }
    
    void setVideoSize(int width, int height);
    void fitToWindow();
    
    bool isLoaded() const { return videoLoaded; }
    bool isPlaying() const { return playing; }
    
    double getDuration() const { return videoDuration; }
    
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    bool videoLoaded = false;
    bool playing = false;
    double currentPosition = 0.0;
    double videoDuration = 0.0;
    float videoVolume = 1.0f;
    
    juce::Image currentFrame;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FruityVideoPlayer)
};

/**
 * @brief ZGameEditor Visualizer - 3D Audio-Reactive Visuals (FL Studio)
 */
class ZGameEditorVisualizer : public juce::Component, public juce::Timer
{
public:
    struct VisualizerSettings {
        enum class PresetType {
            Oscilloscope3D,
            SpectrumBars,
            ParticleSystem,
            WaveformTunnel,
            FractalReactive,
            GeometricShapes,
            LightShow,
            Custom
        };
        
        PresetType preset = PresetType::SpectrumBars;
        juce::Colour primaryColour = juce::Colours::cyan;
        juce::Colour secondaryColour = juce::Colours::magenta;
        float reactivity = 0.8f;
        float smoothing = 0.5f;
        float rotationSpeed = 1.0f;
        float cameraDistance = 5.0f;
    };
    
    ZGameEditorVisualizer();
    ~ZGameEditorVisualizer() override;
    
    void setSettings(const VisualizerSettings& settings);
    VisualizerSettings getSettings() const { return settings; }
    
    void processAudio(const juce::AudioBuffer<float>& buffer);
    
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    std::vector<juce::String> getPresetNames() const;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    // Export
    bool exportVideo(const juce::File& outputFile, int durationSeconds);

private:
    VisualizerSettings settings;
    
    // Audio analysis
    juce::dsp::FFT fft{10};
    std::vector<float> fftData;
    std::vector<float> spectrumData;
    float bassLevel = 0.0f;
    float midLevel = 0.0f;
    float trebleLevel = 0.0f;
    
    // 3D rendering
    float rotation = 0.0f;
    float cameraAngle = 0.0f;
    
    void render3D(juce::Graphics& g);
    void renderOscilloscope3D(juce::Graphics& g);
    void renderSpectrumBars(juce::Graphics& g);
    void renderParticleSystem(juce::Graphics& g);
    void renderWaveformTunnel(juce::Graphics& g);
    
    struct Particle {
        juce::Point<float> position;
        juce::Point<float> velocity;
        float lifetime;
        juce::Colour colour;
    };
    std::vector<Particle> particles;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZGameEditorVisualizer)
};

/**
 * @brief Fruity Big Clock - Large Time Display (FL Studio)
 */
class FruityBigClock : public juce::Component, public juce::Timer
{
public:
    enum class DisplayFormat {
        BarBeatTick,
        TimeCode,
        Samples,
        Seconds
    };
    
    FruityBigClock();
    ~FruityBigClock() override;
    
    void setPosition(double bars, double beats, double ticks);
    void setTimeSeconds(double seconds);
    void setSamples(int64_t samples);
    
    void setDisplayFormat(DisplayFormat format);
    DisplayFormat getDisplayFormat() const { return displayFormat; }
    
    void setFontSize(float size);
    void setColour(juce::Colour colour) { textColour = colour; }
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    DisplayFormat displayFormat = DisplayFormat::BarBeatTick;
    double currentBars = 0.0;
    double currentBeats = 0.0;
    double currentTicks = 0.0;
    double currentSeconds = 0.0;
    int64_t currentSamples = 0;
    float fontSize = 48.0f;
    juce::Colour textColour = juce::Colours::white;
    
    juce::String getDisplayString() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FruityBigClock)
};

/**
 * @brief Fruity dB Meter - Peak Level Meter (FL Studio)
 */
class FruityDBMeter : public juce::Component, public juce::Timer
{
public:
    FruityDBMeter();
    ~FruityDBMeter() override;
    
    void processAudio(const juce::AudioBuffer<float>& buffer);
    
    void setPeakHoldTime(int milliseconds) { peakHoldTime = milliseconds; }
    void setClipIndicatorTime(int milliseconds) { clipIndicatorTime = milliseconds; }
    
    float getPeakLevelLeft() const { return peakL; }
    float getPeakLevelRight() const { return peakR; }
    bool isClipping() const { return clipping; }
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    float peakL = 0.0f;
    float peakR = 0.0f;
    float holdPeakL = 0.0f;
    float holdPeakR = 0.0f;
    bool clipping = false;
    int peakHoldTime = 2000;
    int clipIndicatorTime = 3000;
    int64_t clipStartTime = 0;
    int64_t peakHoldStartTime = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FruityDBMeter)
};

/**
 * @brief Visualizer Manager
 */
class VisualizerManager
{
public:
    VisualizerManager();
    ~VisualizerManager();
    
    WaveCandy* createWaveCandy();
    FruitySpectroman* createSpectroman();
    FruityVideoPlayer* createVideoPlayer();
    ZGameEditorVisualizer* createZGameEditor();
    FruityBigClock* createBigClock();
    FruityDBMeter* createDBMeter();
    
    void processAudioForAll(const juce::AudioBuffer<float>& buffer);

private:
    std::vector<std::unique_ptr<WaveCandy>> waveCandies;
    std::vector<std::unique_ptr<FruitySpectroman>> spectromans;
    std::vector<std::unique_ptr<FruityVideoPlayer>> videoPlayers;
    std::vector<std::unique_ptr<ZGameEditorVisualizer>> zGameEditors;
    std::vector<std::unique_ptr<FruityBigClock>> bigClocks;
    std::vector<std::unique_ptr<FruityDBMeter>> dbMeters;
};

} // namespace Visualizers
} // namespace OmegaStudio
