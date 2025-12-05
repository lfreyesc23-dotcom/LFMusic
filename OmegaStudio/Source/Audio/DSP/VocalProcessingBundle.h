/**
 * @file VocalProcessingBundle.h
 * @brief Complete vocal processing bundle with all essential tools
 * 
 * Includes: VocalRider, FormantShifter, PitchEditor, VocalPresets
 */

#pragma once

#include <JuceHeader.h>
#include "../../Utils/Constants.h"

namespace omega {

//==============================================================================
/** VocalRider - Automatic vocal level riding */
class VocalRider {
public:
    VocalRider();
    void initialize(double sampleRate, int maxBlockSize);
    void process(float* buffer, int numSamples);
    void reset();
    
    void setTargetLevel(float dB) { m_targetLevel = juce::jlimit(-30.0f, 0.0f, dB); }
    float getTargetLevel() const { return m_targetLevel; }
    
    void setSensitivity(float sens) { m_sensitivity = juce::jlimit(0.0f, 1.0f, sens); }
    float getSensitivity() const { return m_sensitivity; }
    
    void setSpeed(float speed) { m_speed = juce::jlimit(0.0f, 1.0f, speed); }
    float getSpeed() const { return m_speed; }
    
    void setRange(float dB) { m_range = juce::jlimit(0.0f, 24.0f, dB); }
    float getRange() const { return m_range; }
    
    float getCurrentGain() const { return m_currentGain; }
    
private:
    float m_targetLevel { -20.0f };
    float m_sensitivity { 0.7f };
    float m_speed { 0.5f };
    float m_range { 12.0f };
    float m_currentGain { 1.0f };
    float m_envelopeFollower { 0.0f };
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** FormantShifter - Formant shifting without pitch change */
class FormantShifter {
public:
    FormantShifter();
    void initialize(double sampleRate, int maxBlockSize);
    void process(const float* input, float* output, int numSamples);
    void reset();
    
    void setFormantShift(float semitones) { m_formantShift = juce::jlimit(-12.0f, 12.0f, semitones); }
    float getFormantShift() const { return m_formantShift; }
    
    void setPreservePitch(bool preserve) { m_preservePitch = preserve; }
    bool isPreservePitch() const { return m_preservePitch; }
    
    void setGenderControl(float gender) { m_gender = juce::jlimit(-1.0f, 1.0f, gender); }
    float getGenderControl() const { return m_gender; }
    
private:
    std::unique_ptr<juce::dsp::FFT> m_fft;
    juce::AudioBuffer<float> m_buffer;
    float m_formantShift { 0.0f };
    bool m_preservePitch { true };
    float m_gender { 0.0f };  // -1 = masculine, +1 = feminine
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** PitchNote - Single note in pitch editor */
struct PitchNote {
    double startTime { 0.0 };
    double duration { 0.0 };
    int midiNote { 60 };
    float cents { 0.0f };  // Fine tuning
    float vibrato { 0.0f };
    float volume { 1.0f };
    juce::String text;  // Lyric/phoneme
    
    bool selected { false };
    juce::Colour color { juce::Colours::blue };
    
    double getEndTime() const { return startTime + duration; }
    bool containsTime(double time) const { return time >= startTime && time < getEndTime(); }
};

//==============================================================================
/** PitchEditor - Melodyne-style pitch editor */
class PitchEditor {
public:
    PitchEditor();
    void initialize(double sampleRate);
    
    // Note management
    int addNote(const PitchNote& note);
    void removeNote(int noteIndex);
    void clearNotes();
    
    PitchNote* getNote(int index);
    const PitchNote* getNote(int index) const;
    int getNumNotes() const { return static_cast<int>(m_notes.size()); }
    
    // Editing operations
    void transposeNote(int noteIndex, int semitones);
    void adjustCents(int noteIndex, float cents);
    void setNoteDuration(int noteIndex, double duration);
    void setNoteStart(int noteIndex, double startTime);
    
    // Bulk operations
    void transposeSelected(int semitones);
    void quantizePitchSelected(float strength);
    void quantizeTimingSelected(double gridSize);
    
    // Analysis and detection
    void detectNotesFromAudio(const juce::AudioBuffer<float>& audio, double sampleRate);
    
    // Rendering
    void applyEditToAudio(juce::AudioBuffer<float>& audio, double sampleRate);
    
    // Selection
    void selectNote(int noteIndex, bool addToSelection = false);
    void deselectAll();
    void selectInTimeRange(double startTime, double endTime);
    
private:
    std::vector<PitchNote> m_notes;
    double m_sampleRate { 48000.0 };
};

//==============================================================================
/** VocalPreset - Complete vocal processing chain preset */
struct VocalPreset {
    juce::String name;
    juce::String genre;
    juce::String description;
    
    // Processing chain parameters
    struct {
        bool enabled { false };
        float threshold { -20.0f };
        float ratio { 4.0f };
    } deEsser;
    
    struct {
        bool enabled { false };
        float threshold { -40.0f };
        float reduction { 12.0f };
    } breathControl;
    
    struct {
        bool enabled { false };
        float strength { 0.7f };
        float speed { 0.5f };
    } pitchCorrection;
    
    struct {
        bool enabled { false };
        float targetLevel { -20.0f };
        float sensitivity { 0.7f };
    } vocalRider;
    
    struct {
        bool enabled { false };
        float numVoices { 2 };
        float width { 0.8f };
    } doubler;
    
    struct {
        bool enabled { false };
        float lowCut { 100.0f };
        float presence { 5000.0f };
        float presenceGain { 3.0f };
        float airBand { 12000.0f };
        float airGain { 2.0f };
    } eq;
    
    struct {
        bool enabled { false };
        float threshold { -24.0f };
        float ratio { 4.0f };
        float attack { 5.0f };
        float release { 100.0f };
    } compressor;
    
    struct {
        bool enabled { false };
        float decay { 1.5f };
        float mix { 0.15f };
    } reverb;
};

//==============================================================================
/** VocalPresetManager - Manage vocal presets */
class VocalPresetManager {
public:
    VocalPresetManager();
    
    // Preset loading
    void loadPreset(const juce::String& presetName);
    const VocalPreset& getCurrentPreset() const { return m_currentPreset; }
    
    // Built-in presets
    static VocalPreset createPopPreset();
    static VocalPreset createRnBPreset();
    static VocalPreset createTrapPreset();
    static VocalPreset createDrillPreset();
    static VocalPreset createReggaetonPreset();
    static VocalPreset createRapPreset();
    static VocalPreset createRockPreset();
    static VocalPreset createCountryPreset();
    
    // Preset management
    void savePreset(const VocalPreset& preset, const juce::File& file);
    VocalPreset loadPresetFromFile(const juce::File& file);
    
    std::vector<juce::String> getAvailablePresets() const;
    
private:
    VocalPreset m_currentPreset;
    std::vector<VocalPreset> m_presets;
    
    void initializeBuiltInPresets();
};

} // namespace omega
