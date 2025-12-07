#include "WavetableSynth.h"
#include <cmath>
#include <algorithm>

namespace OmegaStudio {

//==============================================================================
// Wavetable Implementation
//==============================================================================
void WavetableSynth::Wavetable::loadFromBuffer(const juce::AudioBuffer<float>& buffer) {
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Calculate number of frames
    frameCount = std::min(numSamples / WAVETABLE_SIZE, MAX_FRAMES);
    frames.resize(frameCount);
    
    for (int frame = 0; frame < frameCount; ++frame) {
        int startSample = frame * WAVETABLE_SIZE;
        for (int i = 0; i < WAVETABLE_SIZE && (startSample + i) < numSamples; ++i) {
            // Mix all channels
            float sample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                sample += buffer.getSample(ch, startSample + i);
            }
            frames[frame][i] = sample / (float)numChannels;
        }
    }
}

void WavetableSynth::Wavetable::generateBasicWaveforms(const juce::String& type) {
    frameCount = 1;
    frames.resize(1);
    
    if (type == "sine") {
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            frames[0][i] = std::sin(2.0f * juce::MathConstants<float>::pi * i / WAVETABLE_SIZE);
        }
    }
    else if (type == "saw") {
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            frames[0][i] = 2.0f * (float)i / WAVETABLE_SIZE - 1.0f;
        }
    }
    else if (type == "square") {
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            frames[0][i] = (i < WAVETABLE_SIZE / 2) ? 1.0f : -1.0f;
        }
    }
    else if (type == "triangle") {
        for (int i = 0; i < WAVETABLE_SIZE; ++i) {
            float phase = (float)i / WAVETABLE_SIZE;
            frames[0][i] = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        }
    }
}

//==============================================================================
// WavetableSynth Implementation
//==============================================================================
// Simple sound class for wavetable synth
struct WavetableSound : public juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

//==============================================================================
WavetableSynth::WavetableSynth() {
    // Add voices
    for (int i = 0; i < 16; ++i) {
        addVoice(new WavetableVoice(*this));
    }
    
    // Add a dummy sound
    addSound(new WavetableSound());
    
    initializeFactoryWavetables();
    initializeFactoryPresets();
}

WavetableSynth::~WavetableSynth() = default;

void WavetableSynth::prepare(const juce::dsp::ProcessSpec& spec) {
    currentSpec = spec;
    setCurrentPlaybackSampleRate(spec.sampleRate);
    
    chorus.prepare(spec.sampleRate, (int)spec.maximumBlockSize);
    distortion.prepare(spec.sampleRate);
}

void WavetableSynth::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                     const juce::MidiBuffer& midiMessages,
                                     int startSample, int numSamples) {
    auto startTime = juce::Time::getCurrentTime();
    
    // Process MIDI and render
    Synthesiser::renderNextBlock(outputBuffer, midiMessages, startSample, numSamples);
    
    // Apply effects
    if (params.chorusEnabled) {
        chorus.process(outputBuffer, params.chorusMix);
    }
    
    if (params.distortionEnabled && params.distortionAmount > 0.0f) {
        distortion.process(outputBuffer, params.distortionAmount);
    }
    
    // Apply master volume
    outputBuffer.applyGain(params.masterVolume);
    
    // Calculate CPU usage
    auto elapsedMs = (juce::Time::getCurrentTime() - startTime).inMilliseconds();
    double blockTimeMs = (numSamples * 1000.0) / currentSpec.sampleRate;
    cpuUsage.store((elapsedMs / blockTimeMs) * 100.0);
}

void WavetableSynth::setParameters(const SynthParams& newParams) {
    params = newParams;
}

void WavetableSynth::loadWavetable(int oscIndex, std::shared_ptr<Wavetable> wt) {
    if (oscIndex >= 0 && oscIndex < 3) {
        params.oscillators[oscIndex].wavetable = wt;
    }
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableSynth::createWavetable(const juce::String& type) {
    auto wt = std::make_shared<Wavetable>();
    wt->name = type.toStdString();
    wt->generateBasicWaveforms(type);
    return wt;
}

std::vector<std::string> WavetableSynth::getFactoryWavetableNames() {
    return {
        "Sine", "Sawtooth", "Square", "Triangle",
        "Basic64", "PWM", "Voicelike", "Digital", "Analog",
        "Harmonic Series", "Vowel Morph", "FM Bells"
    };
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableSynth::loadFactoryWavetable(const juce::String& name) {
    if (factoryWavetables.count(name)) {
        return factoryWavetables[name];
    }
    return createWavetable("sine");
}

void WavetableSynth::loadPreset(const Preset& preset) {
    params = preset.params;
}

WavetableSynth::Preset WavetableSynth::getCurrentPreset() const {
    Preset preset;
    preset.name = "Current";
    preset.category = "User";
    preset.params = params;
    return preset;
}

void WavetableSynth::setMaxPolyphony(int voices) {
    clearVoices();
    for (int i = 0; i < voices; ++i) {
        addVoice(new WavetableVoice(*this));
    }
}

int WavetableSynth::getActiveVoiceCount() const {
    int count = 0;
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<WavetableVoice*>(getVoice(i))) {
            if (voice->isVoiceActive()) {
                ++count;
            }
        }
    }
    return count;
}

//==============================================================================
// WavetableVoice Implementation
//==============================================================================
WavetableSynth::WavetableVoice::WavetableVoice(WavetableSynth& owner)
    : synth(owner) {
    // Initialize unison detune and pan
    for (int osc = 0; osc < 3; ++osc) {
        for (int v = 0; v < MAX_UNISON_VOICES; ++v) {
            float spread = (v / (float)(MAX_UNISON_VOICES - 1)) - 0.5f; // -0.5 to 0.5
            oscStates[osc].detuneAmount[v] = spread * 2.0f; // -1 to 1
            oscStates[osc].panAmount[v] = spread * 2.0f;
        }
    }
}

void WavetableSynth::WavetableVoice::startNote(int midiNoteNumber, float vel,
                                               juce::SynthesiserSound*, int) {
    noteNumber = midiNoteNumber;
    velocity = vel;
    currentPitch = (float)midiNoteNumber;
    sampleRate = getSampleRate();
    
    // Reset oscillator phases
    for (int osc = 0; osc < 3; ++osc) {
        for (int v = 0; v < MAX_UNISON_VOICES; ++v) {
            oscStates[osc].phase[v] = 0.0f;
        }
    }
    
    // Start amplitude envelope
    ampEnv.stage = EnvState::Attack;
    ampEnv.level = 0.0f;
    
    // Start filter envelope
    filterEnv.stage = EnvState::Attack;
    filterEnv.level = 0.0f;
    
    // Reset LFOs
    for (int i = 0; i < 2; ++i) {
        lfoStates[i].phase = synth.getParameters().lfos[i].phase;
        lfoStates[i].value = 0.0f;
    }
    
    // Reset filter state
    filterState.ic1eq = 0.0f;
    filterState.ic2eq = 0.0f;
}

void WavetableSynth::WavetableVoice::stopNote(float, bool allowTailOff) {
    if (allowTailOff) {
        ampEnv.stage = EnvState::Release;
        ampEnv.releaseLevel = ampEnv.level;
        filterEnv.stage = EnvState::Release;
        filterEnv.releaseLevel = filterEnv.level;
    } else {
        clearCurrentNote();
        ampEnv.stage = EnvState::Idle;
        filterEnv.stage = EnvState::Idle;
    }
}

void WavetableSynth::WavetableVoice::pitchWheelMoved(int newPitchWheelValue) {
    // Not implemented yet
}

void WavetableSynth::WavetableVoice::controllerMoved(int, int) {
    // Not implemented yet
}

void WavetableSynth::WavetableVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                                     int startSample, int numSamples) {
    if (ampEnv.stage == EnvState::Idle) {
        return;
    }
    
    auto& params = synth.getParameters();
    float dt = 1.0f / (float)sampleRate;
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Update envelopes
        float ampLevel = processEnvelope(ampEnv, params.ampEnvelope, dt);
        float filterLevel = processEnvelope(filterEnv, params.filterEnvelope, dt);
        
        // Update LFOs
        updateLFOs(dt);
        
        // Generate oscillator signals
        float outputSample = 0.0f;
        
        for (int osc = 0; osc < 3; ++osc) {
            if (!params.oscEnabled[osc] || !params.oscillators[osc].wavetable) {
                continue;
            }
            
            float oscSample = renderOscillator(osc, currentPitch);
            outputSample += oscSample * params.oscillators[osc].gain;
        }
        
        // Apply filter
        float cutoff = params.filter.cutoff;
        
        // Filter envelope modulation
        cutoff *= std::pow(2.0f, filterLevel * params.filter.envAmount * 5.0f); // ±5 octaves
        
        // LFO modulation
        cutoff *= std::pow(2.0f, lfoStates[0].value * params.filter.lfoAmount * 3.0f);
        
        // Key tracking
        float keyTrack = (currentPitch - 60.0f) / 12.0f; // Octaves from C4
        cutoff *= std::pow(2.0f, keyTrack * params.filter.keyTracking);
        
        cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);
        
        outputSample = processFilter(outputSample, cutoff, params.filter.resonance);
        
        // Apply amplitude envelope and velocity
        outputSample *= ampLevel * velocity;
        
        // Write to output buffer (stereo)
        if (outputBuffer.getNumChannels() > 0) {
            outputBuffer.addSample(0, startSample + sample, outputSample);
        }
        if (outputBuffer.getNumChannels() > 1) {
            outputBuffer.addSample(1, startSample + sample, outputSample);
        }
        
        // Check if voice is finished
        if (ampEnv.stage == EnvState::Idle) {
            clearCurrentNote();
            break;
        }
    }
}

float WavetableSynth::WavetableVoice::renderOscillator(int oscIndex, float pitch) {
    auto& params = synth.getParameters();
    auto& oscParams = params.oscillators[oscIndex];
    auto& oscState = oscStates[oscIndex];
    
    if (!oscParams.wavetable) {
        return 0.0f;
    }
    
    // Calculate base frequency
    float basePitch = pitch + oscParams.octave * 12.0f + oscParams.semitone + oscParams.cents / 100.0f;
    float baseFreq = 440.0f * std::pow(2.0f, (basePitch - 69.0f) / 12.0f);
    
    float output = 0.0f;
    int voices = oscParams.unisonVoices;
    
    for (int v = 0; v < voices; ++v) {
        // Apply detune
        float detuneCents = oscState.detuneAmount[v] * oscParams.unisonDetune;
        float detuneRatio = std::pow(2.0f, detuneCents / 1200.0f);
        float freq = baseFreq * detuneRatio;
        
        // Get sample from wavetable
        float sample = getWavetableSample(oscParams.wavetable.get(), 
                                         oscState.phase[v], 
                                         oscParams.position);
        
        // Apply stereo spread (future: pan to left/right channels)
        float voiceGain = 1.0f / std::sqrt((float)voices); // Compensate for unison
        output += sample * voiceGain;
        
        // Advance phase
        oscState.phase[v] += freq / (float)sampleRate;
        if (oscState.phase[v] >= 1.0f) {
            oscState.phase[v] -= 1.0f;
        }
    }
    
    return output;
}

float WavetableSynth::WavetableVoice::getWavetableSample(const Wavetable* wt, 
                                                         float phase, float position) {
    if (!wt || wt->frames.empty()) {
        return 0.0f;
    }
    
    // Morphing between frames
    float framePos = position * (wt->frameCount - 1);
    int frame1 = (int)framePos;
    int frame2 = std::min(frame1 + 1, wt->frameCount - 1);
    float frameMix = framePos - frame1;
    
    // Linear interpolation within wavetable
    float samplePos = phase * (WAVETABLE_SIZE - 1);
    int sample1 = (int)samplePos;
    int sample2 = (sample1 + 1) % WAVETABLE_SIZE;
    float sampleMix = samplePos - sample1;
    
    // Get samples from both frames
    float value1 = wt->frames[frame1][sample1] * (1.0f - sampleMix) + 
                   wt->frames[frame1][sample2] * sampleMix;
    float value2 = wt->frames[frame2][sample1] * (1.0f - sampleMix) + 
                   wt->frames[frame2][sample2] * sampleMix;
    
    // Morph between frames
    return value1 * (1.0f - frameMix) + value2 * frameMix;
}

float WavetableSynth::WavetableVoice::processEnvelope(EnvState& env, 
                                                      const EnvelopeParams& params, float dt) {
    switch (env.stage) {
        case EnvState::Attack:
            if (params.attack > 0.0001f) {
                env.level += dt / params.attack;
                if (env.level >= 1.0f) {
                    env.level = 1.0f;
                    env.stage = EnvState::Decay;
                }
            } else {
                env.level = 1.0f;
                env.stage = EnvState::Decay;
            }
            break;
            
        case EnvState::Decay:
            if (params.decay > 0.0001f) {
                env.level -= (1.0f - params.sustain) * dt / params.decay;
                if (env.level <= params.sustain) {
                    env.level = params.sustain;
                    env.stage = EnvState::Sustain;
                }
            } else {
                env.level = params.sustain;
                env.stage = EnvState::Sustain;
            }
            break;
            
        case EnvState::Sustain:
            env.level = params.sustain;
            break;
            
        case EnvState::Release:
            if (params.release > 0.0001f) {
                env.level -= env.releaseLevel * dt / params.release;
                if (env.level <= 0.0f) {
                    env.level = 0.0f;
                    env.stage = EnvState::Idle;
                }
            } else {
                env.level = 0.0f;
                env.stage = EnvState::Idle;
            }
            break;
            
        case EnvState::Idle:
            env.level = 0.0f;
            break;
    }
    
    return env.level;
}

float WavetableSynth::WavetableVoice::processLFO(int lfoIndex, float dt) {
    auto& params = synth.getParameters();
    auto& lfoParams = params.lfos[lfoIndex];
    auto& lfoState = lfoStates[lfoIndex];
    
    float value = 0.0f;
    
    switch (lfoParams.waveform) {
        case LFOWaveform::Sine:
            value = std::sin(lfoState.phase * 2.0f * juce::MathConstants<float>::pi);
            break;
        case LFOWaveform::Triangle:
            value = (lfoState.phase < 0.5f) ? (4.0f * lfoState.phase - 1.0f) 
                                            : (3.0f - 4.0f * lfoState.phase);
            break;
        case LFOWaveform::Sawtooth:
            value = 2.0f * lfoState.phase - 1.0f;
            break;
        case LFOWaveform::Square:
            value = (lfoState.phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case LFOWaveform::Random:
            // Simple random (should be improved with proper noise)
            value = (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            break;
        case LFOWaveform::SampleAndHold:
            if (lfoState.phase < dt * lfoParams.rate) {
                value = (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            } else {
                value = lfoState.value;
            }
            break;
    }
    
    // Convert to unipolar if needed
    if (!lfoParams.bipolar) {
        value = (value + 1.0f) * 0.5f;
    }
    
    lfoState.value = value * lfoParams.depth;
    
    // Advance phase
    lfoState.phase += dt * lfoParams.rate;
    if (lfoState.phase >= 1.0f) {
        lfoState.phase -= 1.0f;
    }
    
    return lfoState.value;
}

void WavetableSynth::WavetableVoice::updateLFOs(float dt) {
    for (int i = 0; i < 2; ++i) {
        processLFO(i, dt);
    }
}

float WavetableSynth::WavetableVoice::processFilter(float input, float cutoff, float resonance) {
    auto& params = synth.getParameters();
    
    // State-variable filter (Chamberlin)
    float sampleRate = (float)this->sampleRate;
    float g = std::tan(juce::MathConstants<float>::pi * cutoff / sampleRate);
    float k = 2.0f - 2.0f * resonance;
    
    float v0 = input;
    float v1 = (filterState.ic1eq + g * (v0 - filterState.ic2eq)) / (1.0f + g * (g + k));
    float v2 = filterState.ic2eq + g * v1;
    
    filterState.ic1eq = 2.0f * v1 - filterState.ic1eq;
    filterState.ic2eq = 2.0f * v2 - filterState.ic2eq;
    
    // Select filter type output
    float output = input;
    switch (params.filter.type) {
        case FilterType::LowPass12dB:
        case FilterType::LowPass24dB:
            output = v2;
            break;
        case FilterType::HighPass12dB:
        case FilterType::HighPass24dB:
            output = v0 - k * v1 - v2;
            break;
        case FilterType::BandPass12dB:
        case FilterType::BandPass24dB:
            output = v1;
            break;
        case FilterType::Notch:
            output = v0 - k * v1;
            break;
        case FilterType::AllPass:
            output = v0 - 2.0f * k * v1;
            break;
    }
    
    // Apply 24dB by running filter twice
    if (params.filter.type == FilterType::LowPass24dB ||
        params.filter.type == FilterType::HighPass24dB ||
        params.filter.type == FilterType::BandPass24dB) {
        input = output;
        v0 = input;
        v1 = (filterState.ic1eq + g * (v0 - filterState.ic2eq)) / (1.0f + g * (g + k));
        v2 = filterState.ic2eq + g * v1;
        filterState.ic1eq = 2.0f * v1 - filterState.ic1eq;
        filterState.ic2eq = 2.0f * v2 - filterState.ic2eq;
        
        if (params.filter.type == FilterType::LowPass24dB) output = v2;
        else if (params.filter.type == FilterType::HighPass24dB) output = v0 - k * v1 - v2;
        else output = v1;
    }
    
    return output;
}

//==============================================================================
// Effects Implementation
//==============================================================================
void WavetableSynth::ChorusEffect::prepare(double sampleRate, int maxBlockSize) {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maxBlockSize;
    spec.numChannels = 2;
    
    chorus.prepare(spec);
    chorus.setRate(0.5f);
    chorus.setDepth(0.3f);
    chorus.setFeedback(0.2f);
    chorus.setMix(0.5f);
    
    dryBuffer.setSize(2, maxBlockSize);
}

void WavetableSynth::ChorusEffect::process(juce::AudioBuffer<float>& buffer, float mix) {
    // Store dry signal
    dryBuffer.makeCopyOf(buffer, true);
    
    // Process chorus
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chorus.process(context);
    
    // Mix dry and wet
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float dry = dryBuffer.getSample(ch, i);
            float wet = buffer.getSample(ch, i);
            buffer.setSample(ch, i, dry * (1.0f - mix) + wet * mix);
        }
    }
}

void WavetableSynth::ChorusEffect::reset() {
    chorus.reset();
}

void WavetableSynth::DistortionEffect::prepare(double sampleRate) {
    // Nothing to prepare
}

void WavetableSynth::DistortionEffect::process(juce::AudioBuffer<float>& buffer, float amount) {
    float drive = 1.0f + amount * 9.0f; // 1-10x
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = buffer.getSample(ch, i);
            float processed = processSample(sample, drive);
            buffer.setSample(ch, i, processed);
        }
    }
}

float WavetableSynth::DistortionEffect::processSample(float input, float drive) {
    // Soft clipping with tanh
    float driven = input * drive;
    return std::tanh(driven) / std::tanh(drive);
}

//==============================================================================
// Factory Content
//==============================================================================
void WavetableSynth::initializeFactoryWavetables() {
    factoryWavetables["Sine"] = WavetableGenerator::createSine();
    factoryWavetables["Sawtooth"] = WavetableGenerator::createSawtooth();
    factoryWavetables["Square"] = WavetableGenerator::createSquare();
    factoryWavetables["Triangle"] = WavetableGenerator::createTriangle();
    factoryWavetables["Basic64"] = WavetableGenerator::createBasic64();
    factoryWavetables["PWM"] = WavetableGenerator::createPWM();
    factoryWavetables["Voicelike"] = WavetableGenerator::createVoicelike();
    factoryWavetables["Digital"] = WavetableGenerator::createDigital();
    factoryWavetables["Analog"] = WavetableGenerator::createAnalog();
}

void WavetableSynth::initializeFactoryPresets() {
    // Preset 1: Classic Lead
    {
        Preset preset;
        preset.name = "Classic Lead";
        preset.category = "Lead";
        preset.params.oscEnabled = {true, false, false};
        preset.params.oscillators[0].wavetable = factoryWavetables["Sawtooth"];
        preset.params.oscillators[0].unisonVoices = 4;
        preset.params.oscillators[0].unisonDetune = 0.15f;
        preset.params.filter.type = FilterType::LowPass24dB;
        preset.params.filter.cutoff = 2000.0f;
        preset.params.filter.resonance = 0.4f;
        preset.params.filter.envAmount = 0.6f;
        preset.params.ampEnvelope.attack = 0.01f;
        preset.params.ampEnvelope.decay = 0.2f;
        preset.params.ampEnvelope.sustain = 0.7f;
        preset.params.ampEnvelope.release = 0.5f;
        factoryPresets.push_back(preset);
    }
    
    // Preset 2: Supersaw
    {
        Preset preset;
        preset.name = "Supersaw";
        preset.category = "Pad";
        preset.params.oscEnabled = {true, true, false};
        preset.params.oscillators[0].wavetable = factoryWavetables["Sawtooth"];
        preset.params.oscillators[0].unisonVoices = 8;
        preset.params.oscillators[0].unisonDetune = 0.25f;
        preset.params.oscillators[1].wavetable = factoryWavetables["Sawtooth"];
        preset.params.oscillators[1].semitone = 12;
        preset.params.oscillators[1].unisonVoices = 6;
        preset.params.filter.cutoff = 3000.0f;
        preset.params.ampEnvelope.attack = 0.2f;
        preset.params.ampEnvelope.decay = 0.3f;
        preset.params.ampEnvelope.sustain = 0.8f;
        preset.params.ampEnvelope.release = 1.0f;
        preset.params.chorusEnabled = true;
        factoryPresets.push_back(preset);
    }
}

std::vector<WavetableSynth::Preset> WavetableSynth::getFactoryPresets() {
    // Return factory presets (would be loaded from file in real implementation)
    std::vector<Preset> presets;
    
    // Add some basic presets
    Preset lead;
    lead.name = "Classic Lead";
    lead.category = "Lead";
    presets.push_back(lead);
    
    Preset pad;
    pad.name = "Warm Pad";
    pad.category = "Pad";
    presets.push_back(pad);
    
    return presets;
}

//==============================================================================
// WavetableGenerator Implementation
//==============================================================================
std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createSine() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Sine";
    wt->generateBasicWaveforms("sine");
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createSawtooth() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Sawtooth";
    wt->generateBasicWaveforms("saw");
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createSquare() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Square";
    wt->generateBasicWaveforms("square");
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createTriangle() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Triangle";
    wt->generateBasicWaveforms("triangle");
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createBasic64() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Basic64";
    wt->frameCount = 64;
    wt->frames.resize(64);
    
    // Morph from sine to sawtooth
    for (int frame = 0; frame < 64; ++frame) {
        float morph = frame / 63.0f;
        for (int i = 0; i < WavetableSynth::WAVETABLE_SIZE; ++i) {
            float phase = (float)i / WavetableSynth::WAVETABLE_SIZE;
            float sine = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
            float saw = 2.0f * phase - 1.0f;
            wt->frames[frame][i] = sine * (1.0f - morph) + saw * morph;
        }
    }
    
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createPWM() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "PWM";
    wt->frameCount = 64;
    wt->frames.resize(64);
    
    // Pulse width modulation
    for (int frame = 0; frame < 64; ++frame) {
        float pulseWidth = 0.05f + (frame / 63.0f) * 0.9f; // 5% to 95%
        for (int i = 0; i < WavetableSynth::WAVETABLE_SIZE; ++i) {
            float phase = (float)i / WavetableSynth::WAVETABLE_SIZE;
            wt->frames[frame][i] = (phase < pulseWidth) ? 1.0f : -1.0f;
        }
    }
    
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createVoicelike() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Voicelike";
    wt->frameCount = 32;
    wt->frames.resize(32);
    
    // Harmonic series with formant-like peaks
    for (int frame = 0; frame < 32; ++frame) {
        float formantShift = frame / 31.0f;
        for (int i = 0; i < WavetableSynth::WAVETABLE_SIZE; ++i) {
            float phase = (float)i / WavetableSynth::WAVETABLE_SIZE;
            float sample = 0.0f;
            
            // Add harmonics with formant peaks
            for (int h = 1; h <= 16; ++h) {
                float formantPeak = std::exp(-std::pow((h - 3.0f * (1.0f + formantShift)), 2.0f) / 4.0f);
                float amplitude = (1.0f / h) * formantPeak;
                sample += amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * phase * h);
            }
            
            wt->frames[frame][i] = sample * 0.3f;
        }
    }
    
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createDigital() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Digital";
    wt->frameCount = 64;
    wt->frames.resize(64);
    
    // Digital/aliased waveforms
    for (int frame = 0; frame < 64; ++frame) {
        int steps = 4 + frame / 2; // Increasing resolution
        for (int i = 0; i < WavetableSynth::WAVETABLE_SIZE; ++i) {
            float phase = (float)i / WavetableSynth::WAVETABLE_SIZE;
            int step = (int)(phase * steps);
            float quantized = (step / (float)steps) * 2.0f - 1.0f;
            wt->frames[frame][i] = quantized;
        }
    }
    
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createAnalog() {
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Analog";
    wt->frameCount = 1;
    wt->frames.resize(1);
    
    // Bandlimited sawtooth (sum of harmonics)
    for (int i = 0; i < WavetableSynth::WAVETABLE_SIZE; ++i) {
        float phase = (float)i / WavetableSynth::WAVETABLE_SIZE;
        float sample = 0.0f;
        
        // Sum first 64 harmonics with 1/n amplitude
        for (int h = 1; h <= 64; ++h) {
            sample += (1.0f / h) * std::sin(2.0f * juce::MathConstants<float>::pi * phase * h);
        }
        
        wt->frames[0][i] = sample * 0.5f;
    }
    
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::createHarmonic(
    const std::vector<float>& harmonicAmplitudes) {
    
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = "Harmonic";
    wt->frameCount = 1;
    wt->frames.resize(1);
    
    for (int i = 0; i < WavetableSynth::WAVETABLE_SIZE; ++i) {
        float phase = (float)i / WavetableSynth::WAVETABLE_SIZE;
        float sample = 0.0f;
        
        for (size_t h = 0; h < harmonicAmplitudes.size(); ++h) {
            sample += harmonicAmplitudes[h] * 
                     std::sin(2.0f * juce::MathConstants<float>::pi * phase * (h + 1));
        }
        
        wt->frames[0][i] = sample;
    }
    
    return wt;
}

std::shared_ptr<WavetableSynth::Wavetable> WavetableGenerator::loadFromAudioFile(
    const juce::File& file) {
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader == nullptr) {
        return nullptr;
    }
    
    juce::AudioBuffer<float> buffer((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);
    
    auto wt = std::make_shared<WavetableSynth::Wavetable>();
    wt->name = file.getFileNameWithoutExtension().toStdString();
    wt->loadFromBuffer(buffer);
    
    return wt;
}

} // namespace OmegaStudio
