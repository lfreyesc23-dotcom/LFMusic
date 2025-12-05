#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace omega {

/**
 * @brief Detected note information
 */
struct DetectedNote {
    double startTime;           // Seconds
    double duration;            // Seconds
    int midiNote;              // 0-127
    float confidence;          // 0.0-1.0
    float velocity;            // 0.0-1.0
    bool isOnset;              // True if strong attack detected
    
    DetectedNote()
        : startTime(0.0), duration(0.0), midiNote(60), 
          confidence(0.0f), velocity(1.0f), isOnset(false) {}
};

/**
 * @brief Rhythm detection result
 */
struct RhythmDetection {
    std::vector<double> onsetTimes;     // Seconds
    double estimatedTempo;               // BPM
    int timeSignatureNum;                // 4 in 4/4
    int timeSignatureDenom;              // 4 in 4/4
    std::vector<float> onsetStrengths;  // 0.0-1.0
};

/**
 * @brief Audio to MIDI converter using pitch and rhythm detection
 */
class AudioToMidi {
public:
    AudioToMidi();
    ~AudioToMidi();
    
    // Initialize
    void initialize(double sampleRate);
    void setSampleRate(double sampleRate);
    
    // Process audio buffer
    void analyzeAudio(const juce::AudioBuffer<float>& audioBuffer);
    
    // Detection settings
    void setMonophonic(bool mono);              // True = single note detection, False = polyphonic
    void setMinNoteLength(double seconds);      // Minimum note duration
    void setOnsetThreshold(float threshold);    // 0.0-1.0 (sensitivity)
    void setPitchQuantization(bool enabled);    // Snap to nearest semitone
    
    // Get results
    const std::vector<DetectedNote>& getDetectedNotes() const { return detectedNotes_; }
    const RhythmDetection& getRhythmInfo() const { return rhythmInfo_; }
    
    // MIDI export
    juce::MidiMessageSequence convertToMidiSequence() const;
    void exportToMidiFile(const juce::File& outputFile) const;
    
    // Real-time detection
    void processBlock(const float* inputData, int numSamples);
    bool hasNewNote() const;
    DetectedNote getLatestNote();
    
    // Quantization
    void quantizeNotes(double gridSize);        // Grid in beats (0.25 = 16th notes)
    void quantizeToScale(const std::vector<int>& scaleNotes);  // MIDI notes in scale
    
    // Advanced options
    void setVibratoTolerance(float cents);      // ±cents to ignore as vibrato
    void setMinPitchHz(float hz);               // Lowest frequency to detect
    void setMaxPitchHz(float hz);               // Highest frequency to detect
    void setPolyphonyLevel(int maxNotes);       // Max simultaneous notes (1-8)
    
private:
    double sampleRate_;
    bool isMonophonic_;
    double minNoteLength_;
    float onsetThreshold_;
    bool pitchQuantization_;
    
    float vibratoTolerance_;
    float minPitchHz_;
    float maxPitchHz_;
    int maxPolyphony_;
    
    std::vector<DetectedNote> detectedNotes_;
    RhythmDetection rhythmInfo_;
    
    // FFT analysis
    juce::dsp::FFT fft_;
    int fftSize_;
    std::vector<float> fftData_;
    std::vector<float> window_;
    
    // Pitch detection state
    struct PitchFrame {
        double time;
        float frequency;
        float confidence;
    };
    std::vector<PitchFrame> pitchFrames_;
    
    // Onset detection state
    std::vector<float> spectralFlux_;
    std::vector<double> onsetTimes_;
    
    // Real-time state
    DetectedNote currentNote_;
    bool noteIsActive_;
    std::vector<DetectedNote> newNotes_;
    
    // Processing methods
    void detectPitch(const float* audioData, int numSamples, float& frequency, float& confidence);
    float autocorrelation(const float* data, int length, int lag);
    float yin(const float* data, int length);
    float spectralPeakDetection(const std::vector<float>& spectrum);
    
    void detectOnsets(const juce::AudioBuffer<float>& audioBuffer);
    float calculateSpectralFlux(const std::vector<float>& spectrum1, const std::vector<float>& spectrum2);
    
    void segmentNotes();
    void mergeNotes();
    
    float frequencyToMidi(float frequency);
    int quantizePitch(float midiFloat);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToMidi)
};

/**
 * @brief Audio to MIDI GUI component
 */
class AudioToMidiComponent : public juce::Component,
                              private juce::Timer {
public:
    AudioToMidiComponent(AudioToMidi& converter);
    ~AudioToMidiComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Visualization
    void setShowWaveform(bool show);
    void setShowPitchTrack(bool show);
    void setShowOnsets(bool show);
    void setShowQuantized(bool show);
    
private:
    AudioToMidi& converter_;
    
    bool showWaveform_;
    bool showPitchTrack_;
    bool showOnsets_;
    bool showQuantized_;
    
    // Helper methods
    void drawWaveform(juce::Graphics& g);
    void drawPitchTrack(juce::Graphics& g);
    void drawOnsets(juce::Graphics& g);
    void drawDetectedNotes(juce::Graphics& g);
    
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToMidiComponent)
};

} // namespace omega
