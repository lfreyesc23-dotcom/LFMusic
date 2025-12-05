/**
 * @file VocalProcessingBundle.cpp
 */

#include "VocalProcessingBundle.h"
#include <cmath>

namespace omega {

//==============================================================================
// VocalRider Implementation
//==============================================================================

VocalRider::VocalRider() {
}

void VocalRider::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_currentGain = 1.0f;
    m_envelopeFollower = 0.0f;
}

void VocalRider::process(float* buffer, int numSamples) {
    float attackCoeff = 1.0f - std::exp(-1000.0f / (m_speed * 100.0f * m_sampleRate));
    float releaseCoeff = 1.0f - std::exp(-1000.0f / ((1.0f - m_speed) * 200.0f * m_sampleRate));
    
    for (int i = 0; i < numSamples; ++i) {
        float inputLevel = std::abs(buffer[i]);
        float inputDB = juce::Decibels::gainToDecibels(inputLevel + 0.0001f);
        
        // Envelope follower
        float coeff = (inputLevel > m_envelopeFollower) ? attackCoeff : releaseCoeff;
        m_envelopeFollower += coeff * (inputLevel - m_envelopeFollower);
        
        float envDB = juce::Decibels::gainToDecibels(m_envelopeFollower + 0.0001f);
        
        // Calculate required gain
        float difference = m_targetLevel - envDB;
        difference *= m_sensitivity;
        difference = juce::jlimit(-m_range, m_range, difference);
        
        float targetGain = juce::Decibels::decibelsToGain(difference);
        
        // Smooth gain changes
        m_currentGain += attackCoeff * (targetGain - m_currentGain);
        
        // Apply gain
        buffer[i] *= m_currentGain;
    }
}

void VocalRider::reset() {
    m_currentGain = 1.0f;
    m_envelopeFollower = 0.0f;
}

//==============================================================================
// FormantShifter Implementation
//==============================================================================

FormantShifter::FormantShifter() {
}

void FormantShifter::initialize(double sampleRate, int maxBlockSize) {
    m_sampleRate = sampleRate;
    m_fft = std::make_unique<juce::dsp::FFT>(11); // 2048-point FFT
    m_buffer.setSize(1, 2048);
}

void FormantShifter::process(const float* input, float* output, int numSamples) {
    // Simplified formant shifting (production would use phase vocoder)
    float shiftFactor = std::pow(2.0f, m_formantShift / 12.0f);
    
    // Gender control adjusts formant shift
    float genderShift = m_gender * 3.0f; // +/- 3 semitones
    shiftFactor *= std::pow(2.0f, genderShift / 12.0f);
    
    // Copy input to output (placeholder for full implementation)
    juce::FloatVectorOperations::copy(output, input, numSamples);
}

void FormantShifter::reset() {
    m_buffer.clear();
}

//==============================================================================
// PitchEditor Implementation
//==============================================================================

PitchEditor::PitchEditor() {
}

void PitchEditor::initialize(double sampleRate) {
    m_sampleRate = sampleRate;
}

int PitchEditor::addNote(const PitchNote& note) {
    m_notes.push_back(note);
    return static_cast<int>(m_notes.size()) - 1;
}

void PitchEditor::removeNote(int noteIndex) {
    if (noteIndex >= 0 && noteIndex < static_cast<int>(m_notes.size())) {
        m_notes.erase(m_notes.begin() + noteIndex);
    }
}

void PitchEditor::clearNotes() {
    m_notes.clear();
}

PitchNote* PitchEditor::getNote(int index) {
    if (index >= 0 && index < static_cast<int>(m_notes.size()))
        return &m_notes[index];
    return nullptr;
}

const PitchNote* PitchEditor::getNote(int index) const {
    if (index >= 0 && index < static_cast<int>(m_notes.size()))
        return &m_notes[index];
    return nullptr;
}

void PitchEditor::transposeNote(int noteIndex, int semitones) {
    if (auto* note = getNote(noteIndex)) {
        note->midiNote += semitones;
        note->midiNote = juce::jlimit(0, 127, note->midiNote);
    }
}

void PitchEditor::adjustCents(int noteIndex, float cents) {
    if (auto* note = getNote(noteIndex)) {
        note->cents += cents;
        note->cents = juce::jlimit(-100.0f, 100.0f, note->cents);
    }
}

void PitchEditor::setNoteDuration(int noteIndex, double duration) {
    if (auto* note = getNote(noteIndex)) {
        note->duration = juce::jmax(0.01, duration);
    }
}

void PitchEditor::setNoteStart(int noteIndex, double startTime) {
    if (auto* note = getNote(noteIndex)) {
        note->startTime = juce::jmax(0.0, startTime);
    }
}

void PitchEditor::transposeSelected(int semitones) {
    for (auto& note : m_notes) {
        if (note.selected) {
            note.midiNote += semitones;
            note.midiNote = juce::jlimit(0, 127, note.midiNote);
        }
    }
}

void PitchEditor::quantizePitchSelected(float strength) {
    for (auto& note : m_notes) {
        if (note.selected) {
            // Snap cents towards 0
            note.cents *= (1.0f - strength);
        }
    }
}

void PitchEditor::quantizeTimingSelected(double gridSize) {
    for (auto& note : m_notes) {
        if (note.selected) {
            // Snap start time to grid
            double quantized = std::round(note.startTime / gridSize) * gridSize;
            note.startTime = quantized;
        }
    }
}

void PitchEditor::detectNotesFromAudio(const juce::AudioBuffer<float>& audio, double sampleRate) {
    // Placeholder for note detection algorithm
    clearNotes();
}

void PitchEditor::applyEditToAudio(juce::AudioBuffer<float>& audio, double sampleRate) {
    // Placeholder for audio rendering with pitch edits
}

void PitchEditor::selectNote(int noteIndex, bool addToSelection) {
    if (!addToSelection) {
        deselectAll();
    }
    
    if (auto* note = getNote(noteIndex)) {
        note->selected = true;
    }
}

void PitchEditor::deselectAll() {
    for (auto& note : m_notes) {
        note.selected = false;
    }
}

void PitchEditor::selectInTimeRange(double startTime, double endTime) {
    for (auto& note : m_notes) {
        if (note.startTime >= startTime && note.startTime < endTime) {
            note.selected = true;
        }
    }
}

//==============================================================================
// VocalPresetManager Implementation
//==============================================================================

VocalPresetManager::VocalPresetManager() {
    initializeBuiltInPresets();
}

void VocalPresetManager::loadPreset(const juce::String& presetName) {
    for (const auto& preset : m_presets) {
        if (preset.name == presetName) {
            m_currentPreset = preset;
            return;
        }
    }
}

VocalPreset VocalPresetManager::createPopPreset() {
    VocalPreset preset;
    preset.name = "Pop Vocals";
    preset.genre = "Pop";
    preset.description = "Bright, clear, and present pop vocal sound";
    
    preset.deEsser.enabled = true;
    preset.deEsser.threshold = -22.0f;
    preset.deEsser.ratio = 4.0f;
    
    preset.breathControl.enabled = true;
    preset.breathControl.threshold = -42.0f;
    preset.breathControl.reduction = 10.0f;
    
    preset.pitchCorrection.enabled = true;
    preset.pitchCorrection.strength = 0.6f;
    preset.pitchCorrection.speed = 0.5f;
    
    preset.vocalRider.enabled = true;
    preset.vocalRider.targetLevel = -18.0f;
    preset.vocalRider.sensitivity = 0.75f;
    
    preset.doubler.enabled = true;
    preset.doubler.numVoices = 2;
    preset.doubler.width = 0.7f;
    
    preset.eq.enabled = true;
    preset.eq.lowCut = 100.0f;
    preset.eq.presence = 3000.0f;
    preset.eq.presenceGain = 4.0f;
    preset.eq.airBand = 12000.0f;
    preset.eq.airGain = 2.5f;
    
    preset.compressor.enabled = true;
    preset.compressor.threshold = -20.0f;
    preset.compressor.ratio = 4.0f;
    preset.compressor.attack = 5.0f;
    preset.compressor.release = 100.0f;
    
    preset.reverb.enabled = true;
    preset.reverb.decay = 1.2f;
    preset.reverb.mix = 0.12f;
    
    return preset;
}

VocalPreset VocalPresetManager::createRnBPreset() {
    VocalPreset preset;
    preset.name = "R&B Vocals";
    preset.genre = "R&B";
    preset.description = "Smooth, silky R&B vocal tone";
    
    preset.deEsser.enabled = true;
    preset.deEsser.threshold = -24.0f;
    preset.deEsser.ratio = 3.0f;
    
    preset.breathControl.enabled = true;
    preset.breathControl.threshold = -45.0f;
    preset.breathControl.reduction = 8.0f;
    
    preset.pitchCorrection.enabled = true;
    preset.pitchCorrection.strength = 0.4f;  // Less aggressive
    preset.pitchCorrection.speed = 0.3f;
    
    preset.vocalRider.enabled = true;
    preset.vocalRider.targetLevel = -20.0f;
    preset.vocalRider.sensitivity = 0.8f;
    
    preset.doubler.enabled = true;
    preset.doubler.numVoices = 3;
    preset.doubler.width = 0.85f;
    
    preset.eq.enabled = true;
    preset.eq.lowCut = 80.0f;
    preset.eq.presence = 2500.0f;
    preset.eq.presenceGain = 3.0f;
    preset.eq.airBand = 10000.0f;
    preset.eq.airGain = 1.5f;
    
    preset.compressor.enabled = true;
    preset.compressor.threshold = -22.0f;
    preset.compressor.ratio = 3.5f;
    preset.compressor.attack = 8.0f;
    preset.compressor.release = 150.0f;
    
    preset.reverb.enabled = true;
    preset.reverb.decay = 2.0f;
    preset.reverb.mix = 0.18f;
    
    return preset;
}

VocalPreset VocalPresetManager::createTrapPreset() {
    VocalPreset preset;
    preset.name = "Trap Vocals";
    preset.genre = "Trap";
    preset.description = "Dark, auto-tuned trap vocal style";
    
    preset.deEsser.enabled = true;
    preset.deEsser.threshold = -20.0f;
    preset.deEsser.ratio = 5.0f;
    
    preset.breathControl.enabled = true;
    preset.breathControl.threshold = -40.0f;
    preset.breathControl.reduction = 15.0f;
    
    preset.pitchCorrection.enabled = true;
    preset.pitchCorrection.strength = 0.95f;  // Hard auto-tune
    preset.pitchCorrection.speed = 0.9f;  // Fast correction
    
    preset.vocalRider.enabled = true;
    preset.vocalRider.targetLevel = -16.0f;
    preset.vocalRider.sensitivity = 0.9f;
    
    preset.doubler.enabled = true;
    preset.doubler.numVoices = 2;
    preset.doubler.width = 0.5f;
    
    preset.eq.enabled = true;
    preset.eq.lowCut = 120.0f;
    preset.eq.presence = 4000.0f;
    preset.eq.presenceGain = 5.0f;
    preset.eq.airBand = 8000.0f;
    preset.eq.airGain = -2.0f;  // Dark sound
    
    preset.compressor.enabled = true;
    preset.compressor.threshold = -18.0f;
    preset.compressor.ratio = 6.0f;
    preset.compressor.attack = 3.0f;
    preset.compressor.release = 80.0f;
    
    preset.reverb.enabled = true;
    preset.reverb.decay = 0.8f;  // Short reverb
    preset.reverb.mix = 0.08f;
    
    return preset;
}

VocalPreset VocalPresetManager::createDrillPreset() {
    VocalPreset preset;
    preset.name = "Drill Vocals";
    preset.genre = "Drill";
    preset.description = "Aggressive UK/NY drill vocal treatment";
    
    preset.deEsser.enabled = true;
    preset.deEsser.threshold = -18.0f;
    preset.deEsser.ratio = 6.0f;
    
    preset.breathControl.enabled = true;
    preset.breathControl.threshold = -38.0f;
    preset.breathControl.reduction = 18.0f;
    
    preset.pitchCorrection.enabled = true;
    preset.pitchCorrection.strength = 0.85f;
    preset.pitchCorrection.speed = 0.8f;
    
    preset.vocalRider.enabled = true;
    preset.vocalRider.targetLevel = -14.0f;  // Loud
    preset.vocalRider.sensitivity = 0.95f;
    
    preset.doubler.enabled = false;  // More raw sound
    
    preset.eq.enabled = true;
    preset.eq.lowCut = 150.0f;
    preset.eq.presence = 3500.0f;
    preset.eq.presenceGain = 6.0f;
    preset.eq.airBand = 10000.0f;
    preset.eq.airGain = -3.0f;
    
    preset.compressor.enabled = true;
    preset.compressor.threshold = -15.0f;
    preset.compressor.ratio = 8.0f;  // Heavy compression
    preset.compressor.attack = 2.0f;
    preset.compressor.release = 60.0f;
    
    preset.reverb.enabled = true;
    preset.reverb.decay = 0.5f;  // Very short
    preset.reverb.mix = 0.05f;
    
    return preset;
}

VocalPreset VocalPresetManager::createReggaetonPreset() {
    VocalPreset preset;
    preset.name = "Reggaeton Vocals";
    preset.genre = "Reggaeton";
    preset.description = "Latin urban vocal flavor";
    
    preset.deEsser.enabled = true;
    preset.deEsser.threshold = -21.0f;
    preset.deEsser.ratio = 4.5f;
    
    preset.breathControl.enabled = true;
    preset.breathControl.threshold = -44.0f;
    preset.breathControl.reduction = 12.0f;
    
    preset.pitchCorrection.enabled = true;
    preset.pitchCorrection.strength = 0.7f;
    preset.pitchCorrection.speed = 0.6f;
    
    preset.vocalRider.enabled = true;
    preset.vocalRider.targetLevel = -17.0f;
    preset.vocalRider.sensitivity = 0.85f;
    
    preset.doubler.enabled = true;
    preset.doubler.numVoices = 2;
    preset.doubler.width = 0.6f;
    
    preset.eq.enabled = true;
    preset.eq.lowCut = 110.0f;
    preset.eq.presence = 3200.0f;
    preset.eq.presenceGain = 4.5f;
    preset.eq.airBand = 11000.0f;
    preset.eq.airGain = 2.0f;
    
    preset.compressor.enabled = true;
    preset.compressor.threshold = -19.0f;
    preset.compressor.ratio = 5.0f;
    preset.compressor.attack = 4.0f;
    preset.compressor.release = 90.0f;
    
    preset.reverb.enabled = true;
    preset.reverb.decay = 1.0f;
    preset.reverb.mix = 0.10f;
    
    return preset;
}

void VocalPresetManager::initializeBuiltInPresets() {
    m_presets.clear();
    m_presets.push_back(createPopPreset());
    m_presets.push_back(createRnBPreset());
    m_presets.push_back(createTrapPreset());
    m_presets.push_back(createDrillPreset());
    m_presets.push_back(createReggaetonPreset());
}

std::vector<juce::String> VocalPresetManager::getAvailablePresets() const {
    std::vector<juce::String> names;
    for (const auto& preset : m_presets) {
        names.push_back(preset.name);
    }
    return names;
}

void VocalPresetManager::savePreset(const VocalPreset& preset, const juce::File& file) {
    // Serialize to JSON
}

VocalPreset VocalPresetManager::loadPresetFromFile(const juce::File& file) {
    // Deserialize from JSON
    return VocalPreset();
}

} // namespace omega
