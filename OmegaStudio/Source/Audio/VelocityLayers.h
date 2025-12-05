/*
  ==============================================================================

    VelocityLayers.h
    Multi-sample playback engine with velocity layers and round-robin rotation
    Professional sampler functionality for realistic instrument reproduction

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <unordered_map>
#include <atomic>

namespace omega {

/**
 * @brief Single audio sample with metadata
 */
struct Sample {
    juce::AudioBuffer<float> buffer;
    int rootNote = 60;          // MIDI note C4
    double sampleRate = 44100.0;
    bool isLooped = false;
    int loopStart = 0;
    int loopEnd = 0;
    
    Sample() = default;
    Sample(const juce::AudioBuffer<float>& buf, int note = 60)
        : buffer(buf), rootNote(note), sampleRate(44100.0) {}
};

/**
 * @brief Velocity layer containing multiple samples for round-robin
 */
struct VelocityLayer {
    int minVelocity = 0;        // 0-127
    int maxVelocity = 127;      // 0-127
    std::vector<Sample> samples; // Round-robin samples
    int currentRRIndex = 0;      // Round-robin rotation index
    
    VelocityLayer() = default;
    VelocityLayer(int minVel, int maxVel) 
        : minVelocity(minVel), maxVelocity(maxVel) {}
    
    /**
     * @brief Get next sample using round-robin rotation
     */
    const Sample* getNextSample() {
        if (samples.empty()) return nullptr;
        const Sample* sample = &samples[currentRRIndex];
        currentRRIndex = (currentRRIndex + 1) % static_cast<int>(samples.size());
        return sample;
    }
    
    /**
     * @brief Add sample to this layer
     */
    void addSample(const Sample& sample) {
        samples.push_back(sample);
    }
};

/**
 * @brief Note mapping containing multiple velocity layers
 */
struct NoteMapping {
    int midiNote = 60;
    std::vector<VelocityLayer> velocityLayers;
    
    NoteMapping() = default;
    explicit NoteMapping(int note) : midiNote(note) {}
    
    /**
     * @brief Get appropriate velocity layer for given velocity
     */
    VelocityLayer* getLayerForVelocity(int velocity) {
        for (auto& layer : velocityLayers) {
            if (velocity >= layer.minVelocity && velocity <= layer.maxVelocity) {
                return &layer;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief Add velocity layer
     */
    void addLayer(int minVel, int maxVel) {
        velocityLayers.emplace_back(minVel, maxVel);
    }
};

/**
 * @brief Voice for polyphonic sample playback
 */
struct PlaybackVoice {
    const Sample* sample = nullptr;
    int midiNote = -1;
    int velocity = 0;
    double playbackPosition = 0.0;
    double pitchRatio = 1.0;
    float gain = 1.0f;
    bool isActive = false;
    
    // Envelope
    float envelopeLevel = 0.0f;
    enum class EnvState { Attack, Sustain, Release, Idle };
    EnvState envState = EnvState::Idle;
    
    void start(const Sample* smp, int note, int vel, double sampleRate) {
        sample = smp;
        midiNote = note;
        velocity = vel;
        playbackPosition = 0.0;
        gain = vel / 127.0f;
        isActive = true;
        envelopeLevel = 0.0f;
        envState = EnvState::Attack;
        
        // Calculate pitch ratio for resampling
        if (sample) {
            int semitoneOffset = note - sample->rootNote;
            pitchRatio = std::pow(2.0, semitoneOffset / 12.0);
        }
    }
    
    void stop() {
        envState = EnvState::Release;
    }
    
    void forceStop() {
        isActive = false;
        envState = EnvState::Idle;
    }
};

/**
 * @brief Multi-sample playback engine with velocity layers and round-robin
 */
class VelocityLayerEngine {
public:
    VelocityLayerEngine();
    ~VelocityLayerEngine() = default;
    
    // Setup
    void initialize(double sampleRate, int maxVoices = 64);
    void setSampleRate(double newSampleRate);
    
    // Sample management
    void clearAllSamples();
    void addSample(int midiNote, int minVelocity, int maxVelocity, const Sample& sample);
    void loadSampleFile(const juce::File& file, int midiNote, int minVel, int maxVel);
    
    // Playback control
    void noteOn(int midiNote, int velocity);
    void noteOff(int midiNote);
    void allNotesOff();
    
    // Processing
    void process(juce::AudioBuffer<float>& buffer);
    
    // Settings
    void setVelocityCrossfade(bool enabled) { velocityCrossfade_ = enabled; }
    void setRoundRobinEnabled(bool enabled) { roundRobinEnabled_ = enabled; }
    void setAttackTime(float ms) { attackTime_ = ms; }
    void setReleaseTime(float ms) { releaseTime_ = ms; }
    
    // Stats
    int getActiveVoiceCount() const;
    int getTotalSampleCount() const;
    
private:
    void processVoice(PlaybackVoice& voice, juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void updateEnvelope(PlaybackVoice& voice, int numSamples);
    PlaybackVoice* findFreeVoice();
    PlaybackVoice* findVoiceForNote(int midiNote);
    
    double sampleRate_ = 48000.0;
    std::unordered_map<int, NoteMapping> noteMappings_; // MIDI note -> NoteMapping
    std::vector<PlaybackVoice> voices_;
    
    // Settings
    bool velocityCrossfade_ = true;
    bool roundRobinEnabled_ = true;
    float attackTime_ = 5.0f;   // ms
    float releaseTime_ = 50.0f; // ms
    
    // Envelope coefficients
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
    
    void updateEnvelopeCoefficients();
};

/**
 * @brief Helper class for building velocity layer presets
 */
class VelocityLayerPresetBuilder {
public:
    static void buildPianoPreset(VelocityLayerEngine& engine, const juce::File& sampleFolder);
    static void buildDrumPreset(VelocityLayerEngine& engine, const juce::File& sampleFolder);
    static void buildGuitarPreset(VelocityLayerEngine& engine, const juce::File& sampleFolder);
    
    /**
     * @brief Auto-detect velocity layers from file naming
     * Expected format: "Instrument_Note_Velocity.wav" (e.g., "Piano_C4_pp.wav", "Piano_C4_ff.wav")
     */
    static void autoDetectAndLoad(VelocityLayerEngine& engine, const juce::File& sampleFolder);
    
private:
    static int velocityFromDynamicMarking(const juce::String& marking);
    static int midiNoteFromString(const juce::String& noteName);
};

} // namespace omega
