#pragma once
#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio {
namespace AudioEditors {

/**
 * @brief Edison - Professional Audio Editor and Recorder (FL Studio)
 * Detailed waveform editing, spectral analysis, noise removal
 */
class Edison
{
public:
    Edison();
    ~Edison();
    
    // Audio data
    void loadAudio(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void loadFromFile(const juce::File& file);
    juce::AudioBuffer<float> getAudio() const { return audioBuffer; }
    
    // Selection
    void setSelection(int startSample, int endSample);
    juce::Range<int> getSelection() const { return selection; }
    void selectAll();
    void deselectAll();
    
    // Basic operations
    void cut();
    void copy();
    void paste(int position);
    void deleteSelection();
    void trim();
    void silence();
    void reverse();
    void invert();
    void normalize(float targetLevel = 0.9f);
    
    // Time operations
    void stretch(float ratio, bool preservePitch = true);
    void pitchShift(float semitones, bool preserveFormants = true);
    
    // Fade operations
    void fadeIn(int samples);
    void fadeOut(int samples);
    void crossfade(int samples);
    
    // Noise removal (FL Studio signature feature)
    void captureNoiseProfile(int startSample, int endSample);
    void removeNoise(float threshold = 0.5f, float reduction = 0.8f);
    void declicker(float sensitivity = 0.5f);
    void deesser(float frequency = 6000.0f, float amount = 0.5f);
    
    // Spectral editing
    struct SpectralEdit {
        float minFreq;
        float maxFreq;
        int startSample;
        int endSample;
        float gain;
    };
    void applySpectralEdit(const SpectralEdit& edit);
    void spectralBlur(float amount);
    void spectralDelay(float time);
    
    // Convolution
    void loadImpulseResponse(const juce::File& file);
    void applyConvolution();
    
    // Loop tools
    struct LoopRegion {
        int startSample;
        int endSample;
        int crossfadeSamples = 0;
    };
    void setLoopRegion(const LoopRegion& region);
    LoopRegion detectBestLoop() const;
    void renderLoop(int repetitions);
    
    // Analysis
    struct Analysis {
        float peakAmplitude;
        float rmsLevel;
        float dynamicRange;
        int zeroCrossings;
        float fundamentalFreq;
        std::vector<float> spectrum;
    };
    Analysis analyze() const;
    std::vector<float> getSpectrum(int fftSize = 8192) const;
    std::vector<float> getWaveform(int width) const;
    
    // EQ
    void applyEQ(const std::vector<std::pair<float, float>>& bands);  // freq, gain pairs
    
    // Resampling
    void resample(double newSampleRate);
    
    // Bit depth
    void changeBitDepth(int bits);
    
    // Effects
    void applyReverb(float roomSize, float damping, float wetLevel);
    void applyDelay(float time, float feedback, float wetLevel);
    void applyChorus(float rate, float depth, float wetLevel);
    void applyFlanger(float rate, float depth, float feedback);
    void applyPhaser(float rate, float depth, int stages);
    
    // Distortion/Saturation
    void applyDistortion(float drive, float tone);
    void applySaturation(float amount);
    
    // Filters
    void applyLowPass(float cutoff, float resonance = 0.0f);
    void applyHighPass(float cutoff, float resonance = 0.0f);
    void applyBandPass(float centerFreq, float bandwidth);
    void applyNotch(float centerFreq, float bandwidth);
    
    // Save/Export
    bool saveToFile(const juce::File& file);
    bool exportSelection(const juce::File& file);
    
    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const { return undoIndex > 0; }
    bool canRedo() const { return undoIndex < static_cast<int>(undoStack.size()) - 1; }

private:
    juce::AudioBuffer<float> audioBuffer;
    double sampleRate = 44100.0;
    juce::Range<int> selection;
    
    // Noise profile
    std::vector<float> noiseProfile;
    bool hasNoiseProfile = false;
    
    // Impulse response
    juce::AudioBuffer<float> impulseResponse;
    
    // Undo/Redo
    std::vector<juce::AudioBuffer<float>> undoStack;
    int undoIndex = -1;
    void pushToUndoStack();
    
    // Processing helpers
    juce::dsp::FFT fft{13};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Edison)
};

/**
 * @brief Newtone - Pitch Correction and Time Alignment (FL Studio)
 */
class Newtone
{
public:
    Newtone();
    ~Newtone();
    
    void loadAudio(const juce::AudioBuffer<float>& buffer, double sampleRate);
    juce::AudioBuffer<float> getProcessedAudio() const;
    
    // Pitch detection and editing
    struct PitchSegment {
        int startSample;
        int endSample;
        float detectedPitch;  // Hz
        float correctedPitch; // Hz
        int targetNote;       // MIDI note
        float vibrato;
    };
    
    void analyzePitch();
    std::vector<PitchSegment>& getPitchSegments() { return pitchSegments; }
    
    // Pitch correction
    void setPitchCorrection(int segmentIndex, int midiNote);
    void autoCorrectPitch(const juce::String& key, const juce::String& scale);
    void quantizePitchToScale(const juce::String& key, const juce::String& scale);
    
    // Time correction
    void detectBeat();
    void alignToGrid(double bpm, int subdivision = 16);
    void stretchSegment(int segmentIndex, float ratio);
    
    // Vibrato control
    void addVibrato(int segmentIndex, float rate, float depth);
    void removeVibrato(int segmentIndex);
    
    // Formant preservation
    void setFormantPreservation(bool preserve) { preserveFormants = preserve; }
    bool getFormantPreservation() const { return preserveFormants; }
    
    // Export
    juce::AudioBuffer<float> renderCorrectedAudio();
    juce::MidiBuffer exportToMidi();

private:
    juce::AudioBuffer<float> originalBuffer;
    std::vector<PitchSegment> pitchSegments;
    double sampleRate = 44100.0;
    bool preserveFormants = true;
    
    float detectPitchAtSample(int sample);
    void applyPitchCorrection(juce::AudioBuffer<float>& buffer, const PitchSegment& segment);
};

/**
 * @brief Newtime - Time Manipulation and Slicing (FL Studio)
 */
class Newtime
{
public:
    Newtime();
    ~Newtime();
    
    void loadAudio(const juce::AudioBuffer<float>& buffer, double sampleRate);
    juce::AudioBuffer<float> getProcessedAudio() const;
    
    // Slice detection
    struct Slice {
        int startSample;
        int endSample;
        float confidence;
        float amplitude;
        bool isTransient;
    };
    
    void detectSlices(float sensitivity = 0.5f);
    std::vector<Slice>& getSlices() { return slices; }
    void addSlice(int sample);
    void removeSlice(int index);
    void clearSlices();
    
    // Time stretching
    void stretchToLength(int targetSamples);
    void stretchToBPM(double sourceBPM, double targetBPM);
    void stretchRatio(float ratio);
    
    // Slice manipulation
    void rearrangeSlices(const std::vector<int>& order);
    void reverseSlices();
    void randomizeSlices();
    void repeatSlice(int sliceIndex, int times);
    void deleteSlice(int sliceIndex);
    
    // Time warping
    struct WarpPoint {
        int sourceSample;
        int targetSample;
    };
    void addWarpPoint(int sourceSample, int targetSample);
    void removeWarpPoint(int index);
    void applyWarping();
    
    // Tempo detection
    double detectTempo();
    void setTempo(double bpm) { currentBPM = bpm; }
    double getTempo() const { return currentBPM; }
    
    // Export individual slices
    std::vector<juce::AudioBuffer<float>> exportSlices() const;
    bool exportSlicesToFiles(const juce::File& folder) const;

private:
    juce::AudioBuffer<float> originalBuffer;
    juce::AudioBuffer<float> processedBuffer;
    std::vector<Slice> slices;
    std::vector<WarpPoint> warpPoints;
    double sampleRate = 44100.0;
    double currentBPM = 120.0;
    
    float calculateTransientStrength(int sample);
    void applyTimeStretch(float ratio);
};

/**
 * @brief Audio Editor Manager
 */
class AudioEditorManager
{
public:
    AudioEditorManager();
    ~AudioEditorManager();
    
    Edison* getEdison() { return edison.get(); }
    Newtone* getNewtone() { return newtone.get(); }
    Newtime* getNewtime() { return newtime.get(); }
    
    void openEdison(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void openNewtone(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void openNewtime(const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    std::unique_ptr<Edison> edison;
    std::unique_ptr<Newtone> newtone;
    std::unique_ptr<Newtime> newtime;
};

} // namespace AudioEditors
} // namespace OmegaStudio
