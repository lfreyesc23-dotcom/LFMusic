#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <map>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @class Clip
 * @brief Clip de audio o MIDI reutilizable
 */
class Clip {
public:
    enum class Type { MIDI, Audio, Pattern };
    
    Clip(const juce::String& name, Type type) 
        : name_(name), type_(type) {
    }
    
    virtual ~Clip() = default;
    
    void setName(const juce::String& name) { name_ = name; }
    juce::String getName() const { return name_; }
    
    void setColour(juce::Colour colour) { colour_ = colour; }
    juce::Colour getColour() const { return colour_; }
    
    void setLength(double beats) { lengthBeats_ = beats; }
    double getLength() const { return lengthBeats_; }
    
    void setLoopEnabled(bool enabled) { loopEnabled_ = enabled; }
    bool isLoopEnabled() const { return loopEnabled_; }
    
    void setStartOffset(double beats) { startOffset_ = beats; }
    double getStartOffset() const { return startOffset_; }
    
    Type getType() const { return type_; }
    
    void setMuted(bool muted) { muted_ = muted; }
    bool isMuted() const { return muted_; }
    
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void renderNextBlock(juce::AudioBuffer<float>& buffer, 
                                 juce::MidiBuffer& midiMessages,
                                 double playheadPosition,
                                 double tempo) = 0;
    
protected:
    juce::String name_;
    Type type_;
    juce::Colour colour_ { juce::Colours::blue };
    double lengthBeats_ { 4.0 };
    double startOffset_ { 0.0 };
    bool loopEnabled_ { true };
    bool muted_ { false };
};

/**
 * @class MIDIClip
 * @brief Clip de MIDI
 */
class MIDIClip : public Clip {
public:
    MIDIClip(const juce::String& name) 
        : Clip(name, Type::MIDI) {
    }
    
    void addNote(int noteNumber, float startBeat, float lengthBeats, uint8_t velocity) {
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, noteNumber, velocity);
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, noteNumber);
        
        notes_.push_back({noteOn, startBeat, lengthBeats});
    }
    
    void clearNotes() {
        notes_.clear();
    }
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        sampleRate_ = sampleRate;
    }
    
    void renderNextBlock(juce::AudioBuffer<float>& buffer, 
                        juce::MidiBuffer& midiMessages,
                        double playheadPosition,
                        double tempo) override {
        if (muted_) return;
        
        double samplesPerBeat = (60.0 / tempo) * sampleRate_;
        int numSamples = buffer.getNumSamples();
        
        // Calculate clip position
        double clipPosition = std::fmod(playheadPosition - startOffset_, lengthBeats_);
        if (clipPosition < 0.0) clipPosition += lengthBeats_;
        
        for (const auto& note : notes_) {
            double noteStartBeat = note.startBeat;
            double noteEndBeat = noteStartBeat + note.lengthBeats;
            
            // Check if note starts in this block
            if (noteStartBeat >= clipPosition && 
                noteStartBeat < clipPosition + (numSamples / samplesPerBeat)) {
                
                int sampleOffset = (noteStartBeat - clipPosition) * samplesPerBeat;
                midiMessages.addEvent(note.message, sampleOffset);
            }
            
            // Check if note ends in this block
            if (noteEndBeat >= clipPosition && 
                noteEndBeat < clipPosition + (numSamples / samplesPerBeat)) {
                
                int sampleOffset = (noteEndBeat - clipPosition) * samplesPerBeat;
                juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                    note.message.getChannel(),
                    note.message.getNoteNumber()
                );
                midiMessages.addEvent(noteOff, sampleOffset);
            }
        }
    }
    
private:
    struct Note {
        juce::MidiMessage message;
        float startBeat;
        float lengthBeats;
    };
    
    std::vector<Note> notes_;
    double sampleRate_ { 44100.0 };
};

/**
 * @class AudioClip
 * @brief Clip de audio
 */
class AudioClip : public Clip {
public:
    AudioClip(const juce::String& name) 
        : Clip(name, Type::Audio) {
    }
    
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        audioBuffer_.makeCopyOf(buffer);
        audioSampleRate_ = sampleRate;
    }
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        playbackSampleRate_ = sampleRate;
        readPosition_ = 0.0;
    }
    
    void renderNextBlock(juce::AudioBuffer<float>& buffer, 
                        juce::MidiBuffer& /*midiMessages*/,
                        double playheadPosition,
                        double tempo) override {
        if (muted_ || audioBuffer_.getNumSamples() == 0) return;
        
        double samplesPerBeat = (60.0 / tempo) * playbackSampleRate_;
        double clipLengthSamples = lengthBeats_ * samplesPerBeat;
        
        // Calculate clip position
        double clipPosition = std::fmod(playheadPosition - startOffset_, lengthBeats_);
        if (clipPosition < 0.0) clipPosition += lengthBeats_;
        
        readPosition_ = clipPosition * samplesPerBeat;
        
        int numSamples = buffer.getNumSamples();
        int numChannels = std::min(buffer.getNumChannels(), audioBuffer_.getNumChannels());
        
        for (int i = 0; i < numSamples; ++i) {
            int srcIndex = (int)readPosition_;
            
            if (loopEnabled_) {
                srcIndex = srcIndex % audioBuffer_.getNumSamples();
            } else if (srcIndex >= audioBuffer_.getNumSamples()) {
                break;
            }
            
            // Simple linear interpolation
            float fraction = readPosition_ - srcIndex;
            int nextIndex = (srcIndex + 1) % audioBuffer_.getNumSamples();
            
            for (int ch = 0; ch < numChannels; ++ch) {
                float sample1 = audioBuffer_.getSample(ch, srcIndex);
                float sample2 = audioBuffer_.getSample(ch, nextIndex);
                float interpolated = sample1 + fraction * (sample2 - sample1);
                
                buffer.addSample(ch, i, interpolated);
            }
            
            readPosition_ += (audioSampleRate_ / playbackSampleRate_);
        }
    }
    
private:
    juce::AudioBuffer<float> audioBuffer_;
    double audioSampleRate_ { 44100.0 };
    double playbackSampleRate_ { 44100.0 };
    double readPosition_ { 0.0 };
};

/**
 * @class ClipSlot
 * @brief Slot que contiene un clip en la session view
 */
class ClipSlot {
public:
    ClipSlot() = default;
    
    void setClip(std::shared_ptr<Clip> clip) {
        clip_ = clip;
    }
    
    std::shared_ptr<Clip> getClip() { return clip_; }
    
    void trigger() {
        if (clip_ != nullptr) {
            isPlaying_ = true;
            playheadPosition_ = 0.0;
        }
    }
    
    void stop() {
        isPlaying_ = false;
    }
    
    bool isPlaying() const { return isPlaying_; }
    
    void process(juce::AudioBuffer<float>& buffer, 
                juce::MidiBuffer& midiMessages,
                double tempo) {
        if (isPlaying_ && clip_ != nullptr) {
            clip_->renderNextBlock(buffer, midiMessages, playheadPosition_, tempo);
            
            double samplesPerBeat = (60.0 / tempo) * 44100.0;  // TODO: Get actual sample rate
            playheadPosition_ += buffer.getNumSamples() / samplesPerBeat;
        }
    }
    
private:
    std::shared_ptr<Clip> clip_;
    bool isPlaying_ { false };
    double playheadPosition_ { 0.0 };
};

/**
 * @class Scene
 * @brief Escena en session view (fila horizontal)
 */
class Scene {
public:
    Scene(const juce::String& name) : name_(name) {}
    
    void setName(const juce::String& name) { name_ = name; }
    juce::String getName() const { return name_; }
    
    void addSlot(ClipSlot slot) {
        slots_.push_back(slot);
    }
    
    ClipSlot& getSlot(int index) {
        jassert(index >= 0 && index < (int)slots_.size());
        return slots_[index];
    }
    
    int getNumSlots() const { return slots_.size(); }
    
    void triggerAll() {
        for (auto& slot : slots_) {
            if (slot.getClip() != nullptr) {
                slot.trigger();
            }
        }
    }
    
    void stopAll() {
        for (auto& slot : slots_) {
            slot.stop();
        }
    }
    
private:
    juce::String name_;
    std::vector<ClipSlot> slots_;
};

/**
 * @class SessionView
 * @brief Vista de sesión (matriz de clips)
 */
class SessionView {
public:
    SessionView() {
        // Create 8 tracks x 8 scenes by default
        for (int i = 0; i < 8; ++i) {
            auto scene = std::make_unique<Scene>("Scene " + juce::String(i + 1));
            
            for (int j = 0; j < 8; ++j) {
                scene->addSlot(ClipSlot());
            }
            
            scenes_.push_back(std::move(scene));
        }
    }
    
    Scene* getScene(int index) {
        return (index >= 0 && index < (int)scenes_.size()) 
            ? scenes_[index].get() : nullptr;
    }
    
    int getNumScenes() const { return scenes_.size(); }
    
    ClipSlot* getClipSlot(int track, int scene) {
        if (scene >= 0 && scene < (int)scenes_.size()) {
            auto* sceneObj = scenes_[scene].get();
            if (track >= 0 && track < sceneObj->getNumSlots()) {
                return &sceneObj->getSlot(track);
            }
        }
        return nullptr;
    }
    
    void triggerScene(int sceneIndex) {
        if (sceneIndex >= 0 && sceneIndex < (int)scenes_.size()) {
            // Stop currently playing scene
            if (currentScene_ >= 0 && currentScene_ < (int)scenes_.size()) {
                scenes_[currentScene_]->stopAll();
            }
            
            scenes_[sceneIndex]->triggerAll();
            currentScene_ = sceneIndex;
        }
    }
    
    void stopAll() {
        for (auto& scene : scenes_) {
            scene->stopAll();
        }
        currentScene_ = -1;
    }
    
    int getCurrentScene() const { return currentScene_; }
    
private:
    std::vector<std::unique_ptr<Scene>> scenes_;
    int currentScene_ { -1 };
};

/**
 * @class ArrangementView
 * @brief Vista de arreglo (timeline)
 */
class ArrangementView {
public:
    struct ClipInstance {
        std::shared_ptr<Clip> clip;
        int trackIndex { 0 };
        double startBeat { 0.0 };
        
        double getEndBeat() const { 
            return startBeat + (clip ? clip->getLength() : 0.0); 
        }
    };
    
    void addClip(std::shared_ptr<Clip> clip, int track, double startBeat) {
        ClipInstance instance;
        instance.clip = clip;
        instance.trackIndex = track;
        instance.startBeat = startBeat;
        
        clipInstances_.push_back(instance);
        
        // Sort by start time
        std::sort(clipInstances_.begin(), clipInstances_.end(),
            [](const ClipInstance& a, const ClipInstance& b) {
                return a.startBeat < b.startBeat;
            });
    }
    
    void removeClip(int index) {
        if (index >= 0 && index < (int)clipInstances_.size()) {
            clipInstances_.erase(clipInstances_.begin() + index);
        }
    }
    
    const std::vector<ClipInstance>& getClips() const {
        return clipInstances_;
    }
    
    void process(juce::AudioBuffer<float>& buffer, 
                juce::MidiBuffer& midiMessages,
                double playheadPosition,
                double tempo) {
        
        double samplesPerBeat = (60.0 / tempo) * 44100.0;
        double blockLengthBeats = buffer.getNumSamples() / samplesPerBeat;
        
        for (auto& instance : clipInstances_) {
            if (instance.clip == nullptr) continue;
            
            double clipStart = instance.startBeat;
            double clipEnd = instance.getEndBeat();
            
            // Check if clip is active in this block
            if (playheadPosition + blockLengthBeats >= clipStart && 
                playheadPosition < clipEnd) {
                
                instance.clip->renderNextBlock(buffer, midiMessages, 
                                              playheadPosition, tempo);
            }
        }
    }
    
private:
    std::vector<ClipInstance> clipInstances_;
};

/**
 * @class PatternSystem
 * @brief Sistema completo de patterns/clips
 */
class PatternSystem {
public:
    enum class ViewMode { Session, Arrangement };
    
    PatternSystem() = default;
    
    void setViewMode(ViewMode mode) { viewMode_ = mode; }
    ViewMode getViewMode() const { return viewMode_; }
    
    SessionView& getSessionView() { return sessionView_; }
    ArrangementView& getArrangementView() { return arrangementView_; }
    
    void process(juce::AudioBuffer<float>& buffer, 
                juce::MidiBuffer& midiMessages,
                double playheadPosition,
                double tempo) {
        
        if (viewMode_ == ViewMode::Session) {
            // Process session view
            if (sessionView_.getCurrentScene() >= 0) {
                auto* scene = sessionView_.getScene(sessionView_.getCurrentScene());
                if (scene != nullptr) {
                    for (int i = 0; i < scene->getNumSlots(); ++i) {
                        scene->getSlot(i).process(buffer, midiMessages, tempo);
                    }
                }
            }
        } else {
            // Process arrangement view
            arrangementView_.process(buffer, midiMessages, playheadPosition, tempo);
        }
    }
    
    std::shared_ptr<MIDIClip> createMIDIClip(const juce::String& name) {
        auto clip = std::make_shared<MIDIClip>(name);
        clips_.push_back(clip);
        return clip;
    }
    
    std::shared_ptr<AudioClip> createAudioClip(const juce::String& name) {
        auto clip = std::make_shared<AudioClip>(name);
        clips_.push_back(clip);
        return clip;
    }
    
private:
    ViewMode viewMode_ { ViewMode::Session };
    SessionView sessionView_;
    ArrangementView arrangementView_;
    std::vector<std::shared_ptr<Clip>> clips_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternSystem)
};

} // namespace Sequencer
} // namespace OmegaStudio
