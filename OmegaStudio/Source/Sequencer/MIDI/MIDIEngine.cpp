/*
  ==============================================================================
    MIDIEngine.cpp
    Implementation del sistema MIDI completo
  ==============================================================================
*/

#include "MIDIEngine.h"
#include <random>

namespace OmegaStudio {

//==============================================================================
// MIDINote Implementation
//==============================================================================

juce::var MIDINote::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("noteNumber", noteNumber);
    obj->setProperty("startBeat", startBeat);
    obj->setProperty("lengthBeats", lengthBeats);
    obj->setProperty("velocity", velocity);
    obj->setProperty("channel", channel);
    return juce::var(obj.get());
}

MIDINote MIDINote::fromVar(const juce::var& v) {
    MIDINote note;
    if (auto* obj = v.getDynamicObject()) {
        note.noteNumber = obj->getProperty("noteNumber");
        note.startBeat = obj->getProperty("startBeat");
        note.lengthBeats = obj->getProperty("lengthBeats");
        note.velocity = static_cast<uint8_t>(int(obj->getProperty("velocity")));
        note.channel = static_cast<uint8_t>(int(obj->getProperty("channel")));
    }
    return note;
}

//==============================================================================
// MIDICCEvent Implementation
//==============================================================================

juce::var MIDICCEvent::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("ccNumber", ccNumber);
    obj->setProperty("value", value);
    obj->setProperty("beat", beat);
    obj->setProperty("channel", channel);
    return juce::var(obj.get());
}

MIDICCEvent MIDICCEvent::fromVar(const juce::var& v) {
    MIDICCEvent event;
    if (auto* obj = v.getDynamicObject()) {
        event.ccNumber = obj->getProperty("ccNumber");
        event.value = static_cast<uint8_t>(int(obj->getProperty("value")));
        event.beat = obj->getProperty("beat");
        event.channel = static_cast<uint8_t>(int(obj->getProperty("channel")));
    }
    return event;
}

//==============================================================================
// MIDIClip Implementation
//==============================================================================

// Constructor moved to header

void MIDIClip::addNote(const MIDINote& note) {
    notes.push_back(note);
    sortNotes();
}

void MIDIClip::removeNote(int index) {
    if (index >= 0 && index < getNumNotes())
        notes.erase(notes.begin() + index);
}

void MIDIClip::removeNotesInRange(float start, float end) {
    notes.erase(
        std::remove_if(notes.begin(), notes.end(),
            [start, end](const MIDINote& note) {
                return note.startBeat >= start && note.startBeat < end;
            }),
        notes.end()
    );
}

void MIDIClip::clearNotes() {
    notes.clear();
}

std::vector<MIDINote> MIDIClip::getNotesInRange(float start, float end) const {
    std::vector<MIDINote> result;
    for (const auto& note : notes) {
        if (note.startBeat >= start && note.startBeat < end)
            result.push_back(note);
    }
    return result;
}

void MIDIClip::addCCEvent(const MIDICCEvent& event) {
    ccEvents.push_back(event);
    std::sort(ccEvents.begin(), ccEvents.end(),
        [](const MIDICCEvent& a, const MIDICCEvent& b) {
            return a.beat < b.beat;
        });
}

void MIDIClip::removeCCEvent(int index) {
    if (index >= 0 && index < getNumCCEvents())
        ccEvents.erase(ccEvents.begin() + index);
}

void MIDIClip::clearCCEvents() {
    ccEvents.clear();
}

void MIDIClip::transpose(int semitones) {
    for (auto& note : notes) {
        note.noteNumber = juce::jlimit(0, 127, note.noteNumber + semitones);
    }
}

void MIDIClip::shiftTiming(float beatOffset) {
    for (auto& note : notes)
        note.startBeat += beatOffset;
    
    for (auto& event : ccEvents)
        event.beat += beatOffset;
}

void MIDIClip::scaleVelocity(float multiplier) {
    for (auto& note : notes) {
        int newVel = static_cast<int>(note.velocity * multiplier);
        note.velocity = static_cast<uint8_t>(juce::jlimit(1, 127, newVel));
    }
}

void MIDIClip::quantize(float gridSize) {
    for (auto& note : notes) {
        note.startBeat = std::round(note.startBeat / gridSize) * gridSize;
        note.lengthBeats = std::round(note.lengthBeats / gridSize) * gridSize;
        if (note.lengthBeats < gridSize)
            note.lengthBeats = gridSize;
    }
    sortNotes();
}

void MIDIClip::humanize(float amountTiming, float amountVelocity) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> timingDist(-amountTiming, amountTiming);
    std::uniform_real_distribution<float> velocityDist(-amountVelocity, amountVelocity);
    
    for (auto& note : notes) {
        // Timing variation
        note.startBeat += timingDist(gen);
        
        // Velocity variation
        int velChange = static_cast<int>(note.velocity * velocityDist(gen));
        note.velocity = static_cast<uint8_t>(juce::jlimit(1, 127, note.velocity + velChange));
    }
    
    sortNotes();
}

juce::var MIDIClip::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("colour", colour.toString());
    obj->setProperty("startBeat", startBeat);
    obj->setProperty("lengthBeats", lengthBeats);
    
    juce::var notesArray;
    for (const auto& note : notes)
        notesArray.append(note.toVar());
    obj->setProperty("notes", notesArray);
    
    juce::var ccArray;
    for (const auto& event : ccEvents)
        ccArray.append(event.toVar());
    obj->setProperty("ccEvents", ccArray);
    
    return juce::var(obj.get());
}

MIDIClip MIDIClip::fromVar(const juce::var& v) {
    MIDIClip clip;
    
    if (auto* obj = v.getDynamicObject()) {
        clip.name = obj->getProperty("name").toString();
        clip.colour = juce::Colour::fromString(obj->getProperty("colour").toString());
        clip.startBeat = obj->getProperty("startBeat");
        clip.lengthBeats = obj->getProperty("lengthBeats");
        
        if (auto* notesArray = obj->getProperty("notes").getArray()) {
            for (const auto& noteVar : *notesArray)
                clip.notes.push_back(MIDINote::fromVar(noteVar));
        }
        
        if (auto* ccArray = obj->getProperty("ccEvents").getArray()) {
            for (const auto& eventVar : *ccArray)
                clip.ccEvents.push_back(MIDICCEvent::fromVar(eventVar));
        }
    }
    
    return clip;
}

void MIDIClip::sortNotes() {
    std::sort(notes.begin(), notes.end(),
        [](const MIDINote& a, const MIDINote& b) {
            return a.startBeat < b.startBeat;
        });
}

//==============================================================================
// MIDITrack Implementation
//==============================================================================

MIDITrack::MIDITrack(const juce::String& name) : name(name) {}

void MIDITrack::addClip(std::unique_ptr<MIDIClip> clip) {
    clips.push_back(std::move(clip));
}

void MIDITrack::removeClip(int index) {
    if (index >= 0 && index < getNumClips())
        clips.erase(clips.begin() + index);
}

void MIDITrack::clearClips() {
    clips.clear();
}

void MIDITrack::renderToMIDIBuffer(juce::MidiBuffer& buffer,
                                   double startTime, double endTime,
                                   double bpm, int sampleRate) const {
    if (muted) return;
    
    double beatsPerSecond = bpm / 60.0;
    double startBeat = startTime * beatsPerSecond;
    double endBeat = endTime * beatsPerSecond;
    
    for (const auto& clip : clips) {
        float clipStart = clip->getStartBeat();
        float clipEnd = clipStart + clip->getLengthBeats();
        
        // Check if clip is in render range
        if (clipEnd < startBeat || clipStart > endBeat)
            continue;
        
        // Render notes
        for (const auto& note : clip->getNotes()) {
            float noteAbsoluteBeat = clipStart + note.startBeat;
            
            if (noteAbsoluteBeat >= startBeat && noteAbsoluteBeat < endBeat) {
                // Note On
                double noteOnTime = noteAbsoluteBeat / beatsPerSecond;
                int noteOnSample = static_cast<int>((noteOnTime - startTime) * sampleRate);
                
                juce::MidiMessage noteOn = juce::MidiMessage::noteOn(
                    midiChannel, note.noteNumber, note.velocity
                );
                buffer.addEvent(noteOn, noteOnSample);
                
                // Note Off
                double noteOffTime = (noteAbsoluteBeat + note.lengthBeats) / beatsPerSecond;
                int noteOffSample = static_cast<int>((noteOffTime - startTime) * sampleRate);
                
                if (noteOffSample < (endTime - startTime) * sampleRate) {
                    juce::MidiMessage noteOff = juce::MidiMessage::noteOff(
                        midiChannel, note.noteNumber
                    );
                    buffer.addEvent(noteOff, noteOffSample);
                }
            }
        }
        
        // Render CC events
        for (const auto& event : clip->getCCEvents()) {
            float eventAbsoluteBeat = clipStart + event.beat;
            
            if (eventAbsoluteBeat >= startBeat && eventAbsoluteBeat < endBeat) {
                double eventTime = eventAbsoluteBeat / beatsPerSecond;
                int sample = static_cast<int>((eventTime - startTime) * sampleRate);
                
                juce::MidiMessage cc = juce::MidiMessage::controllerEvent(
                    midiChannel, event.ccNumber, event.value
                );
                buffer.addEvent(cc, sample);
            }
        }
    }
}

juce::var MIDITrack::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("midiChannel", midiChannel);
    obj->setProperty("muted", muted);
    obj->setProperty("soloed", soloed);
    
    juce::var clipsArray;
    for (const auto& clip : clips)
        clipsArray.append(clip->toVar());
    obj->setProperty("clips", clipsArray);
    
    return juce::var(obj.get());
}

std::unique_ptr<MIDITrack> MIDITrack::fromVar(const juce::var& v) {
    auto track = std::make_unique<MIDITrack>();
    
    if (auto* obj = v.getDynamicObject()) {
        track->name = obj->getProperty("name").toString();
        track->midiChannel = static_cast<uint8_t>(int(obj->getProperty("midiChannel")));
        track->muted = obj->getProperty("muted");
        track->soloed = obj->getProperty("soloed");
        
        if (auto* clipsArray = obj->getProperty("clips").getArray()) {
            for (const auto& clipVar : *clipsArray) {
                auto clip = std::make_unique<MIDIClip>(MIDIClip::fromVar(clipVar));
                track->clips.push_back(std::move(clip));
            }
        }
    }
    
    return track;
}

//==============================================================================
// MIDIEngine Implementation
//==============================================================================

MIDIEngine::MIDIEngine() = default;
MIDIEngine::~MIDIEngine() = default;

void MIDIEngine::addTrack(std::unique_ptr<MIDITrack> track) {
    tracks.push_back(std::move(track));
}

void MIDIEngine::removeTrack(int index) {
    if (index >= 0 && index < getNumTracks())
        tracks.erase(tracks.begin() + index);
}

void MIDIEngine::clearTracks() {
    tracks.clear();
}

void MIDIEngine::renderMIDI(juce::MidiBuffer& buffer,
                            double startTime, double endTime,
                            double bpm, int sampleRate) const {
    buffer.clear();
    
    // Check for solo
    bool anySolo = false;
    for (const auto& track : tracks) {
        if (track->isSoloed()) {
            anySolo = true;
            break;
        }
    }
    
    // Render each track
    for (const auto& track : tracks) {
        if (anySolo && !track->isSoloed())
            continue;
        
        track->renderToMIDIBuffer(buffer, startTime, endTime, bpm, sampleRate);
    }
}

void MIDIEngine::startRecording(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= getNumTracks())
        return;
    
    recording = true;
    recordingTrackIndex = trackIndex;
    recordingClip = std::make_unique<MIDIClip>("Recorded Clip");
    recordStartTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
}

void MIDIEngine::stopRecording() {
    if (!recording || !recordingClip)
        return;
    
    recording = false;
    
    if (recordingClip->getNumNotes() > 0 && recordingTrackIndex >= 0) {
        tracks[recordingTrackIndex]->addClip(std::move(recordingClip));
    }
    
    recordingClip.reset();
    recordingTrackIndex = -1;
}

void MIDIEngine::recordMIDIMessage(const juce::MidiMessage& message, double timestamp) {
    if (!recording || !recordingClip)
        return;
    
    double relativeBeat = (timestamp - recordStartTime) * (120.0 / 60.0); // Assume 120 BPM
    
    if (message.isNoteOn()) {
        MIDINote note;
        note.noteNumber = message.getNoteNumber();
        note.velocity = static_cast<uint8_t>(message.getVelocity());
        note.startBeat = static_cast<float>(relativeBeat);
        note.lengthBeats = 0.25f; // Will be updated on note off
        note.channel = static_cast<uint8_t>(message.getChannel());
        
        recordingClip->addNote(note);
    }
    else if (message.isNoteOff()) {
        // Find corresponding note on and update length
        auto& notes = const_cast<std::vector<MIDINote>&>(recordingClip->getNotes());
        for (auto it = notes.rbegin(); it != notes.rend(); ++it) {
            if (it->noteNumber == message.getNoteNumber() && it->lengthBeats < 1.0f) {
                it->lengthBeats = static_cast<float>(relativeBeat - it->startBeat);
                break;
            }
        }
    }
    else if (message.isController()) {
        MIDICCEvent event;
        event.ccNumber = message.getControllerNumber();
        event.value = static_cast<uint8_t>(message.getControllerValue());
        event.beat = static_cast<float>(relativeBeat);
        event.channel = static_cast<uint8_t>(message.getChannel());
        
        recordingClip->addCCEvent(event);
    }
}

void MIDIEngine::quantizeNotes(const std::vector<MIDINote*>& notes, const QuantizeSettings& settings) {
    float gridSize = settings.getGridSizeBeats();
    
    for (auto* note : notes) {
        if (settings.quantizeStartTime) {
            float quantized = std::round(note->startBeat / gridSize) * gridSize;
            note->startBeat = note->startBeat + (quantized - note->startBeat) * settings.strength;
        }
        
        if (settings.quantizeLength) {
            float quantized = std::round(note->lengthBeats / gridSize) * gridSize;
            if (quantized < gridSize) quantized = gridSize;
            note->lengthBeats = note->lengthBeats + (quantized - note->lengthBeats) * settings.strength;
        }
    }
}

void MIDIEngine::humanizeNotes(std::vector<MIDINote*>& notes, const HumanizeSettings& settings) {
    std::mt19937 gen(settings.seed);
    std::uniform_real_distribution<float> timingDist(-settings.timingVariation, settings.timingVariation);
    std::uniform_real_distribution<float> velocityDist(-settings.velocityVariation, settings.velocityVariation);
    
    for (auto* note : notes) {
        note->startBeat += timingDist(gen);
        
        int velChange = static_cast<int>(note->velocity * velocityDist(gen));
        note->velocity = static_cast<uint8_t>(juce::jlimit(1, 127, note->velocity + velChange));
    }
}

void MIDIEngine::transposeNotes(const std::vector<MIDINote*>& notes, int semitones) {
    for (auto* note : notes)
        note->noteNumber = juce::jlimit(0, 127, note->noteNumber + semitones);
}

void MIDIEngine::scaleVelocities(const std::vector<MIDINote*>& notes, float multiplier) {
    for (auto* note : notes) {
        int newVel = static_cast<int>(note->velocity * multiplier);
        note->velocity = static_cast<uint8_t>(juce::jlimit(1, 127, newVel));
    }
}

juce::var MIDIEngine::toVar() const {
    juce::var tracksArray;
    for (const auto& track : tracks)
        tracksArray.append(track->toVar());
    return tracksArray;
}

void MIDIEngine::loadFromVar(const juce::var& v) {
    tracks.clear();
    
    if (auto* tracksArray = v.getArray()) {
        for (const auto& trackVar : *tracksArray) {
            auto track = MIDITrack::fromVar(trackVar);
            if (track)
                tracks.push_back(std::move(track));
        }
    }
}

//==============================================================================
// MIDIInputManager Implementation
//==============================================================================

MIDIInputManager::MIDIInputManager() = default;

MIDIInputManager::~MIDIInputManager() {
    closeDevice();
}

juce::StringArray MIDIInputManager::getAvailableDevices() const {
    juce::StringArray deviceNames;
    auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : devices)
        deviceNames.add(device.name);
    return deviceNames;
}

bool MIDIInputManager::openDevice(const juce::String& deviceName) {
    closeDevice();
    
    auto devices = juce::MidiInput::getAvailableDevices();
    for (const auto& device : devices) {
        if (device.name == deviceName) {
            currentDevice = juce::MidiInput::openDevice(device.identifier, this);
            if (currentDevice) {
                currentDevice->start();
                return true;
            }
        }
    }
    
    return false;
}

void MIDIInputManager::closeDevice() {
    if (currentDevice) {
        currentDevice->stop();
        currentDevice.reset();
    }
}

juce::String MIDIInputManager::getCurrentDeviceName() const {
    return currentDevice ? currentDevice->getName() : "";
}

void MIDIInputManager::addListener(Listener* listener) {
    listeners.add(listener);
}

void MIDIInputManager::removeListener(Listener* listener) {
    listeners.remove(listener);
}

void MIDIInputManager::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) {
    listeners.call([&](Listener& l) { l.handleIncomingMidiMessage(message); });
}

//==============================================================================
// PianoRollModel Implementation
//==============================================================================

PianoRollModel::PianoRollModel() = default;

void PianoRollModel::setVerticalZoom(float zoom) {
    verticalZoom = juce::jlimit(0.1f, 5.0f, zoom);
}

void PianoRollModel::setHorizontalZoom(float zoom) {
    horizontalZoom = juce::jlimit(0.1f, 10.0f, zoom);
}

void PianoRollModel::setViewPosition(float beat, int lowestNote) {
    viewStartBeat = juce::jmax(0.0f, beat);
    lowestVisibleNote = juce::jlimit(0, 108, lowestNote);
}

} // namespace OmegaStudio
