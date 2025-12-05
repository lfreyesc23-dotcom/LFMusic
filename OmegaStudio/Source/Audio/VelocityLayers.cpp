/*
  ==============================================================================

    VelocityLayers.cpp
    Implementation of multi-sample velocity layer engine

  ==============================================================================
*/

#include "VelocityLayers.h"
#include <cmath>

namespace omega {

VelocityLayerEngine::VelocityLayerEngine() {
    initialize(48000.0, 64);
}

void VelocityLayerEngine::initialize(double sampleRate, int maxVoices) {
    sampleRate_ = sampleRate;
    voices_.resize(maxVoices);
    for (auto& voice : voices_) {
        voice.isActive = false;
    }
    updateEnvelopeCoefficients();
}

void VelocityLayerEngine::setSampleRate(double newSampleRate) {
    sampleRate_ = newSampleRate;
    updateEnvelopeCoefficients();
}

void VelocityLayerEngine::updateEnvelopeCoefficients() {
    // Calculate envelope coefficients for exponential curves
    attackCoeff_ = attackTime_ > 0.0f ? std::exp(-1.0f / (sampleRate_ * attackTime_ * 0.001f)) : 0.0f;
    releaseCoeff_ = releaseTime_ > 0.0f ? std::exp(-1.0f / (sampleRate_ * releaseTime_ * 0.001f)) : 0.0f;
}

void VelocityLayerEngine::clearAllSamples() {
    allNotesOff();
    noteMappings_.clear();
}

void VelocityLayerEngine::addSample(int midiNote, int minVelocity, int maxVelocity, const Sample& sample) {
    // Get or create note mapping
    auto& mapping = noteMappings_[midiNote];
    mapping.midiNote = midiNote;
    
    // Find existing velocity layer or create new one
    VelocityLayer* targetLayer = nullptr;
    for (auto& layer : mapping.velocityLayers) {
        if (layer.minVelocity == minVelocity && layer.maxVelocity == maxVelocity) {
            targetLayer = &layer;
            break;
        }
    }
    
    if (!targetLayer) {
        mapping.velocityLayers.emplace_back(minVelocity, maxVelocity);
        targetLayer = &mapping.velocityLayers.back();
    }
    
    // Add sample to layer (for round-robin)
    targetLayer->addSample(sample);
}

void VelocityLayerEngine::loadSampleFile(const juce::File& file, int midiNote, int minVel, int maxVel) {
    if (!file.existsAsFile()) return;
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto* reader = formatManager.createReaderFor(file);
    if (reader == nullptr) return;
    
    Sample sample;
    sample.buffer.setSize(static_cast<int>(reader->numChannels), 
                         static_cast<int>(reader->lengthInSamples));
    reader->read(&sample.buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    sample.sampleRate = reader->sampleRate;
    sample.rootNote = midiNote;
    
    delete reader;
    
    addSample(midiNote, minVel, maxVel, sample);
}

void VelocityLayerEngine::noteOn(int midiNote, int velocity) {
    // Find note mapping
    auto it = noteMappings_.find(midiNote);
    if (it == noteMappings_.end()) return;
    
    NoteMapping& mapping = it->second;
    VelocityLayer* layer = mapping.getLayerForVelocity(velocity);
    if (!layer) return;
    
    // Get sample (with round-robin if enabled)
    const Sample* sample = roundRobinEnabled_ ? layer->getNextSample() : 
                          (!layer->samples.empty() ? &layer->samples[0] : nullptr);
    
    if (!sample) return;
    
    // Handle velocity crossfade between layers
    if (velocityCrossfade_ && mapping.velocityLayers.size() > 1) {
        // Find adjacent layers for crossfade
        VelocityLayer* lowerLayer = nullptr;
        VelocityLayer* upperLayer = nullptr;
        
        for (auto& l : mapping.velocityLayers) {
            if (l.maxVelocity < velocity && (!lowerLayer || l.maxVelocity > lowerLayer->maxVelocity)) {
                lowerLayer = &l;
            }
            if (l.minVelocity > velocity && (!upperLayer || l.minVelocity < upperLayer->minVelocity)) {
                upperLayer = &l;
            }
        }
        
        // TODO: Implement crossfade between layers
        // For now, just use primary layer
    }
    
    // Find free voice and start playback
    PlaybackVoice* voice = findFreeVoice();
    if (voice) {
        voice->start(sample, midiNote, velocity, sampleRate_);
    }
}

void VelocityLayerEngine::noteOff(int midiNote) {
    // Find all voices playing this note and release them
    for (auto& voice : voices_) {
        if (voice.isActive && voice.midiNote == midiNote && 
            voice.envState != PlaybackVoice::EnvState::Release) {
            voice.stop();
        }
    }
}

void VelocityLayerEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.forceStop();
    }
}

void VelocityLayerEngine::process(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    
    for (auto& voice : voices_) {
        if (voice.isActive) {
            processVoice(voice, buffer, 0, buffer.getNumSamples());
        }
    }
}

void VelocityLayerEngine::processVoice(PlaybackVoice& voice, juce::AudioBuffer<float>& buffer, 
                                      int startSample, int numSamples) {
    if (!voice.sample || !voice.isActive) return;
    
    const auto& sampleBuffer = voice.sample->buffer;
    const int sampleLength = sampleBuffer.getNumSamples();
    const int numChannels = std::min(buffer.getNumChannels(), sampleBuffer.getNumChannels());
    
    for (int i = 0; i < numSamples; ++i) {
        // Update envelope
        switch (voice.envState) {
            case PlaybackVoice::EnvState::Attack:
                voice.envelopeLevel = 1.0f - (1.0f - voice.envelopeLevel) * attackCoeff_;
                if (voice.envelopeLevel >= 0.99f) {
                    voice.envelopeLevel = 1.0f;
                    voice.envState = PlaybackVoice::EnvState::Sustain;
                }
                break;
                
            case PlaybackVoice::EnvState::Sustain:
                voice.envelopeLevel = 1.0f;
                break;
                
            case PlaybackVoice::EnvState::Release:
                voice.envelopeLevel *= releaseCoeff_;
                if (voice.envelopeLevel < 0.001f) {
                    voice.forceStop();
                    return;
                }
                break;
                
            case PlaybackVoice::EnvState::Idle:
                voice.forceStop();
                return;
        }
        
        // Get sample position (with pitch ratio)
        int samplePos = static_cast<int>(voice.playbackPosition);
        
        // Check if we've reached the end
        if (samplePos >= sampleLength) {
            if (voice.sample->isLooped && voice.sample->loopEnd > voice.sample->loopStart) {
                // Loop back
                voice.playbackPosition = voice.sample->loopStart;
                samplePos = voice.sample->loopStart;
            } else {
                // Stop voice
                voice.forceStop();
                return;
            }
        }
        
        // Linear interpolation for pitch shifting
        float frac = static_cast<float>(voice.playbackPosition - samplePos);
        int nextPos = std::min(samplePos + 1, sampleLength - 1);
        
        float finalGain = voice.gain * voice.envelopeLevel;
        
        // Mix sample into output buffer
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* sampleData = sampleBuffer.getReadPointer(ch);
            float* outputData = buffer.getWritePointer(ch);
            
            float sample1 = sampleData[samplePos];
            float sample2 = sampleData[nextPos];
            float interpolatedSample = sample1 + frac * (sample2 - sample1);
            
            outputData[startSample + i] += interpolatedSample * finalGain;
        }
        
        // Advance playback position
        voice.playbackPosition += voice.pitchRatio;
    }
}

PlaybackVoice* VelocityLayerEngine::findFreeVoice() {
    // First, try to find completely inactive voice
    for (auto& voice : voices_) {
        if (!voice.isActive) {
            return &voice;
        }
    }
    
    // If no free voice, steal oldest voice in release
    PlaybackVoice* oldest = nullptr;
    for (auto& voice : voices_) {
        if (voice.envState == PlaybackVoice::EnvState::Release) {
            if (!oldest || voice.envelopeLevel < oldest->envelopeLevel) {
                oldest = &voice;
            }
        }
    }
    
    if (oldest) {
        oldest->forceStop();
        return oldest;
    }
    
    // Last resort: steal any voice
    return &voices_[0];
}

PlaybackVoice* VelocityLayerEngine::findVoiceForNote(int midiNote) {
    for (auto& voice : voices_) {
        if (voice.isActive && voice.midiNote == midiNote) {
            return &voice;
        }
    }
    return nullptr;
}

int VelocityLayerEngine::getActiveVoiceCount() const {
    int count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive) ++count;
    }
    return count;
}

int VelocityLayerEngine::getTotalSampleCount() const {
    int count = 0;
    for (const auto& pair : noteMappings_) {
        for (const auto& layer : pair.second.velocityLayers) {
            count += static_cast<int>(layer.samples.size());
        }
    }
    return count;
}

// ============================================================================
// Preset Builder Implementation
// ============================================================================

void VelocityLayerPresetBuilder::buildPianoPreset(VelocityLayerEngine& engine, const juce::File& sampleFolder) {
    // Standard piano velocity layers: pp, mp, mf, f, ff
    struct VelRange { int min, max; juce::String suffix; };
    const VelRange layers[] = {
        {1, 25, "pp"},
        {26, 50, "mp"},
        {51, 75, "mf"},
        {76, 100, "f"},
        {101, 127, "ff"}
    };
    
    // Scan folder for piano samples
    auto files = sampleFolder.findChildFiles(juce::File::findFiles, false, "*.wav");
    
    for (const auto& file : files) {
        juce::String name = file.getFileNameWithoutExtension();
        
        // Parse note name and velocity layer
        for (const auto& layer : layers) {
            if (name.containsIgnoreCase(layer.suffix)) {
                int midiNote = midiNoteFromString(name);
                if (midiNote >= 0) {
                    engine.loadSampleFile(file, midiNote, layer.min, layer.max);
                }
                break;
            }
        }
    }
}

void VelocityLayerPresetBuilder::buildDrumPreset(VelocityLayerEngine& engine, const juce::File& sampleFolder) {
    // Standard GM drum mapping
    const struct { int note; juce::String name; } drumMap[] = {
        {36, "kick"},
        {38, "snare"},
        {42, "hihat_closed"},
        {46, "hihat_open"},
        {49, "crash"},
        {51, "ride"}
    };
    
    auto files = sampleFolder.findChildFiles(juce::File::findFiles, false, "*.wav");
    
    for (const auto& file : files) {
        juce::String name = file.getFileNameWithoutExtension().toLowerCase();
        
        for (const auto& drum : drumMap) {
            if (name.contains(drum.name)) {
                // Drums typically use full velocity range per sample
                engine.loadSampleFile(file, drum.note, 1, 127);
                break;
            }
        }
    }
}

void VelocityLayerPresetBuilder::buildGuitarPreset(VelocityLayerEngine& engine, const juce::File& sampleFolder) {
    // Guitar velocity layers
    const struct { int min, max; juce::String suffix; } layers[] = {
        {1, 63, "soft"},
        {64, 127, "hard"}
    };
    
    auto files = sampleFolder.findChildFiles(juce::File::findFiles, false, "*.wav");
    
    for (const auto& file : files) {
        juce::String name = file.getFileNameWithoutExtension();
        
        for (const auto& layer : layers) {
            if (name.containsIgnoreCase(layer.suffix)) {
                int midiNote = midiNoteFromString(name);
                if (midiNote >= 0) {
                    engine.loadSampleFile(file, midiNote, layer.min, layer.max);
                }
                break;
            }
        }
    }
}

void VelocityLayerPresetBuilder::autoDetectAndLoad(VelocityLayerEngine& engine, const juce::File& sampleFolder) {
    auto files = sampleFolder.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff");
    
    for (const auto& file : files) {
        juce::String name = file.getFileNameWithoutExtension();
        
        // Expected format: "Instrument_Note_Velocity" or "Note_Velocity"
        auto tokens = juce::StringArray::fromTokens(name, "_-", "");
        
        if (tokens.size() >= 2) {
            int midiNote = -1;
            int velMin = 1, velMax = 127;
            
            // Try to find note name
            for (const auto& token : tokens) {
                int note = midiNoteFromString(token);
                if (note >= 0) {
                    midiNote = note;
                    break;
                }
            }
            
            // Try to find velocity marking
            for (const auto& token : tokens) {
                int vel = velocityFromDynamicMarking(token);
                if (vel >= 0) {
                    // Map velocity marking to range
                    velMin = std::max(1, vel - 25);
                    velMax = std::min(127, vel + 25);
                    break;
                }
            }
            
            if (midiNote >= 0) {
                engine.loadSampleFile(file, midiNote, velMin, velMax);
            }
        }
    }
}

int VelocityLayerPresetBuilder::velocityFromDynamicMarking(const juce::String& marking) {
    juce::String m = marking.toLowerCase();
    
    if (m == "ppp") return 15;
    if (m == "pp") return 30;
    if (m == "p") return 45;
    if (m == "mp") return 60;
    if (m == "mf") return 75;
    if (m == "f") return 90;
    if (m == "ff") return 105;
    if (m == "fff") return 120;
    
    if (m == "soft") return 40;
    if (m == "medium") return 70;
    if (m == "hard" || m == "loud") return 100;
    
    return -1; // Not recognized
}

int VelocityLayerPresetBuilder::midiNoteFromString(const juce::String& noteName) {
    // Parse note names like C4, F#5, Bb3, etc.
    juce::String name = noteName.toUpperCase();
    
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const char* flatNames[] = {"C", "DB", "D", "EB", "E", "F", "GB", "G", "AB", "A", "BB", "B"};
    
    int noteValue = -1;
    int octave = -1;
    
    // Try sharp notation
    for (int i = 0; i < 12; ++i) {
        if (name.startsWith(noteNames[i])) {
            noteValue = i;
            juce::String rest = name.substring(strlen(noteNames[i]));
            octave = rest.getIntValue();
            break;
        }
    }
    
    // Try flat notation
    if (noteValue == -1) {
        for (int i = 0; i < 12; ++i) {
            if (name.startsWith(flatNames[i])) {
                noteValue = i;
                juce::String rest = name.substring(strlen(flatNames[i]));
                octave = rest.getIntValue();
                break;
            }
        }
    }
    
    if (noteValue >= 0 && octave >= -1 && octave <= 9) {
        return (octave + 1) * 12 + noteValue;
    }
    
    return -1; // Not recognized
}

} // namespace omega
