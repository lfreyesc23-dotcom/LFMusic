#pragma once
#include <JuceHeader.h>
#include <vector>

namespace OmegaStudio {
namespace Audio {

/**
 * @brief Advanced Audio Clip with FL Studio 2025 Features
 * Supports envelopes, stretch, pitch, slice, reverse, normalize
 */
class AudioClip
{
public:
    struct EnvelopePoint {
        double time;      // 0.0 to 1.0 (normalized position)
        float value;      // 0.0 to 1.0
        float curvature;  // -1.0 to 1.0 (bezier curve)
    };

    enum class EnvelopeType {
        Volume,
        Pan,
        Pitch,
        Cutoff,
        Resonance,
        Custom
    };

    struct Envelope {
        EnvelopeType type = EnvelopeType::Volume;
        std::vector<EnvelopePoint> points;
        bool enabled = true;
        juce::String name;
    };

    struct ClipSettings {
        double startTime = 0.0;
        double length = 0.0;
        double offset = 0.0;        // Trim start
        double fadeIn = 0.0;
        double fadeOut = 0.0;
        float volume = 1.0f;
        float pan = 0.0f;           // -1.0 to 1.0
        float pitch = 0.0f;         // Semitones
        float timeStretch = 1.0f;   // 0.5 to 2.0
        bool reverse = false;
        bool normalize = false;
        juce::Colour colour;
    };

    AudioClip(const juce::String& name = "Audio Clip");
    ~AudioClip();

    // Audio data
    void setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate);
    const juce::AudioBuffer<float>& getAudioData() const { return audioBuffer; }
    
    // Clip settings
    void setSettings(const ClipSettings& settings);
    const ClipSettings& getSettings() const { return settings; }
    
    // Envelopes
    void addEnvelope(const Envelope& envelope);
    void removeEnvelope(EnvelopeType type);
    Envelope* getEnvelope(EnvelopeType type);
    std::vector<Envelope>& getAllEnvelopes() { return envelopes; }
    
    // Envelope editing
    void addEnvelopePoint(EnvelopeType type, double time, float value);
    void removeEnvelopePoint(EnvelopeType type, int index);
    void moveEnvelopePoint(EnvelopeType type, int index, double newTime, float newValue);
    
    // Processing
    void renderToBuffer(juce::AudioBuffer<float>& output, 
                       int startSample, 
                       int numSamples,
                       double sampleRate);
    
    // Clip operations
    void normalize();
    void reverse();
    void trimSilence(float threshold = 0.01f);
    void detectTempo();
    void detectKey();
    
    // Slicing
    struct SlicePoint {
        int samplePosition;
        float confidence;
        juce::String label;
    };
    std::vector<SlicePoint> detectSlicePoints();
    void setSlicePoints(const std::vector<SlicePoint>& points);
    
    // Metadata
    void setName(const juce::String& name) { clipName = name; }
    juce::String getName() const { return clipName; }
    void setColour(juce::Colour colour) { settings.colour = colour; }
    
    double getDuration() const { return audioBuffer.getNumSamples() / currentSampleRate; }
    double getSampleRate() const { return currentSampleRate; }
    
    // Serialization
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree);

private:
    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> processedBuffer;
    
    ClipSettings settings;
    std::vector<Envelope> envelopes;
    std::vector<SlicePoint> slicePoints;
    
    juce::String clipName;
    double currentSampleRate = 44100.0;
    
    bool needsReprocessing = true;
    
    // Processing helpers
    void applyEnvelopes(juce::AudioBuffer<float>& buffer, double sampleRate);
    void applyTimeStretch(juce::AudioBuffer<float>& buffer, float ratio);
    void applyPitchShift(juce::AudioBuffer<float>& buffer, float semitones);
    float getEnvelopeValue(const Envelope& env, double normalizedTime);
    float interpolateCubic(float y0, float y1, float y2, float y3, float t);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClip)
};

/**
 * @brief Audio Clip Manager for organizing clips in Playlist
 */
class AudioClipManager
{
public:
    AudioClipManager();
    ~AudioClipManager();
    
    // Clip management
    AudioClip* createClip(const juce::String& name);
    void removeClip(AudioClip* clip);
    void clearAllClips();
    
    std::vector<AudioClip*> getAllClips() const { return clips; }
    AudioClip* getClipByName(const juce::String& name);
    
    // Clipboard operations
    void copyClip(AudioClip* clip);
    AudioClip* pasteClip();
    
private:
    std::vector<AudioClip*> clips;
    std::unique_ptr<AudioClip> clipboard;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClipManager)
};

} // namespace Audio
} // namespace OmegaStudio
