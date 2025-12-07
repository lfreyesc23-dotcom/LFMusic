#include "VirtualAnalogSynth.h"
#include <cmath>
#include <algorithm>

namespace OmegaStudio {

//==============================================================================
VirtualAnalogSynth::VirtualAnalogSynth() {
    for (int i = 0; i < 16; ++i) {
        addVoice(new AnalogVoice(*this));
    }
    
    struct AnalogSound : public juce::SynthesiserSound {
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };
    
    addSound(new AnalogSound());
    initializeFactoryPresets();
}

VirtualAnalogSynth::~VirtualAnalogSynth() = default;

void VirtualAnalogSynth::prepare(const juce::dsp::ProcessSpec& spec) {
    currentSpec = spec;
    setCurrentPlaybackSampleRate(spec.sampleRate);
}

void VirtualAnalogSynth::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                         const juce::MidiBuffer& midiMessages,
                                         int startSample, int numSamples) {
    auto startTime = juce::Time::getCurrentTime();
    Synthesiser::renderNextBlock(outputBuffer, midiMessages, startSample, numSamples);
    outputBuffer.applyGain(params.masterVolume);
    
    auto elapsedMs = (juce::Time::getCurrentTime() - startTime).inMilliseconds();
    cpuUsage.store((elapsedMs * currentSpec.sampleRate) / (numSamples * 10.0));
}

void VirtualAnalogSynth::setParameters(const SynthParams& newParams) {
    params = newParams;
}

void VirtualAnalogSynth::loadPreset(const Preset& preset) {
    params = preset.params;
}

VirtualAnalogSynth::Preset VirtualAnalogSynth::getCurrentPreset() const {
    Preset preset;
    preset.name = "Current";
    preset.params = params;
    return preset;
}

void VirtualAnalogSynth::initializeFactoryPresets() {
    // Supersaw preset
    {
        Preset preset;
        preset.name = "Supersaw Lead";
        preset.category = "Lead";
        preset.params.oscillators[0].type = OscType::Saw;
        preset.params.oscillators[0].unisonVoices = 8;
        preset.params.oscillators[0].unisonDetune = 0.3f;
        preset.params.filter.type = FilterType::LowPass24;
        preset.params.filter.cutoff = 3000.0f;
        preset.params.filter.resonance = 0.4f;
        preset.params.chorusEnabled = true;
        factoryPresets.push_back(preset);
    }
    
    // Bass preset
    {
        Preset preset;
        preset.name = "Analog Bass";
        preset.category = "Bass";
        preset.params.oscillators[0].type = OscType::Square;
        preset.params.subOsc.enabled = true;
        preset.params.filter.type = FilterType::LowPass24;
        preset.params.filter.cutoff = 800.0f;
        preset.params.filter.resonance = 0.6f;
        factoryPresets.push_back(preset);
    }
}

std::vector<VirtualAnalogSynth::Preset> VirtualAnalogSynth::getFactoryPresets() {
    VirtualAnalogSynth temp;
    return temp.factoryPresets;
}

int VirtualAnalogSynth::getActiveVoiceCount() const {
    int count = 0;
    for (int i = 0; i < getNumVoices(); ++i) {
        if (getVoice(i)->isVoiceActive()) ++count;
    }
    return count;
}

//==============================================================================
// AnalogVoice Implementation
//==============================================================================
VirtualAnalogSynth::AnalogVoice::AnalogVoice(VirtualAnalogSynth& owner) : synth(owner) {
    for (int osc = 0; osc < NUM_OSCILLATORS; ++osc) {
        for (int v = 0; v < 8; ++v) {
            float spread = (v / 7.0f) - 0.5f;
            oscStates[osc].detuneAmounts[v] = spread * 2.0f;
        }
    }
}

void VirtualAnalogSynth::AnalogVoice::startNote(int midiNoteNumber, float vel,
                                                juce::SynthesiserSound*, int) {
    noteNumber = midiNoteNumber;
    velocity = vel;
    sampleRate = getSampleRate();
    targetPitch = currentPitch = (float)midiNoteNumber;
    
    ampEnv.stage = EnvState::Attack;
    ampEnv.level = 0.0f;
    filterEnv.stage = EnvState::Attack;
    filterEnv.level = 0.0f;
    modEnv.stage = EnvState::Attack;
    modEnv.level = 0.0f;
    
    for (auto& lfo : lfoStates) {
        if (!synth.getParameters().lfos[&lfo - lfoStates.data()].freeRunning) {
            lfo.phase = 0.0f;
        }
    }
}

void VirtualAnalogSynth::AnalogVoice::stopNote(float, bool allowTailOff) {
    if (allowTailOff) {
        ampEnv.stage = EnvState::Release;
        ampEnv.releaseLevel = ampEnv.level;
        filterEnv.stage = EnvState::Release;
        modEnv.stage = EnvState::Release;
    } else {
        clearCurrentNote();
        ampEnv.stage = EnvState::Idle;
    }
}

void VirtualAnalogSynth::AnalogVoice::pitchWheelMoved(int value) {
    pitchBend = ((value / 8192.0f) - 1.0f);
}

void VirtualAnalogSynth::AnalogVoice::controllerMoved(int controller, int value) {
    if (controller == 1) modWheel = value / 127.0f;
}

void VirtualAnalogSynth::AnalogVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                                      int startSample, int numSamples) {
    if (ampEnv.stage == EnvState::Idle) {
        return;
    }
    
    auto& params = synth.getParameters();
    float dt = 1.0f / (float)sampleRate;
    
    for (int sample = 0; sample < numSamples; ++sample) {
        float ampLevel = processEnvelope(ampEnv, params.ampEnv, dt);
        float filterLevel = processEnvelope(filterEnv, params.filterEnv, dt);
        processEnvelope(modEnv, params.modEnv, dt);
        
        updateLFOs(dt);
        updatePortamento(dt);
        
        float output = 0.0f;
        
        // Mix oscillators
        for (int i = 0; i < NUM_OSCILLATORS; ++i) {
            if (params.oscillators[i].enabled) {
                output += renderOscillator(i) * params.oscMix[i];
            }
        }
        
        if (params.subOsc.enabled) {
            output += renderSubOscillator() * params.subMix;
        }
        
        output = processFilter(output);
        output *= ampLevel * velocity;
        
        if (outputBuffer.getNumChannels() > 0) {
            outputBuffer.addSample(0, startSample + sample, output * 0.3f);
        }
        if (outputBuffer.getNumChannels() > 1) {
            outputBuffer.addSample(1, startSample + sample, output * 0.3f);
        }
        
        if (ampEnv.stage == EnvState::Idle) {
            clearCurrentNote();
            break;
        }
    }
}

float VirtualAnalogSynth::AnalogVoice::renderOscillator(int oscIndex) {
    auto& params = synth.getParameters();
    auto& oscParams = params.oscillators[oscIndex];
    auto& oscState = oscStates[oscIndex];
    
    float basePitch = currentPitch + oscParams.octave * 12.0f + oscParams.semitone + 
                     oscParams.cents / 100.0f;
    float baseFreq = 440.0f * std::pow(2.0f, (basePitch - 69.0f) / 12.0f);
    
    float output = 0.0f;
    int voices = oscParams.unisonVoices;
    
    for (int v = 0; v < voices; ++v) {
        float detune = oscState.detuneAmounts[v] * oscParams.unisonDetune * 0.5f;
        float freq = baseFreq * std::pow(2.0f, detune / 12.0f);
        
        float sample = generateWaveform(oscParams.type, oscState.phases[v], oscParams.pulseWidth);
        output += sample / std::sqrt((float)voices);
        
        oscState.phases[v] += freq / (float)sampleRate;
        if (oscState.phases[v] >= 1.0f) oscState.phases[v] -= 1.0f;
    }
    
    return output * oscParams.level;
}

float VirtualAnalogSynth::AnalogVoice::renderSubOscillator() {
    auto& params = synth.getParameters();
    float pitch = currentPitch + params.subOsc.octave * 12.0f;
    float freq = 440.0f * std::pow(2.0f, (pitch - 69.0f) / 12.0f);
    
    OscType type = (params.subOsc.type == SubOscParams::Sine) ? OscType::Sine :
                   (params.subOsc.type == SubOscParams::Square) ? OscType::Square : OscType::Triangle;
    
    float sample = generateWaveform(type, subOscState.phases[0], 0.5f);
    
    subOscState.phases[0] += freq / (float)sampleRate;
    if (subOscState.phases[0] >= 1.0f) subOscState.phases[0] -= 1.0f;
    
    return sample * params.subOsc.level;
}

float VirtualAnalogSynth::AnalogVoice::generateWaveform(OscType type, float phase, float pw) {
    switch (type) {
        case OscType::Sine:
            return std::sin(2.0f * juce::MathConstants<float>::pi * phase);
        
        case OscType::Saw: {
            float naive = 2.0f * phase - 1.0f;
            float blep = polyBLEP(phase, 0.001f);
            return naive - blep;
        }
        
        case OscType::Square: {
            float naive = (phase < 0.5f) ? 1.0f : -1.0f;
            return naive;
        }
        
        case OscType::Triangle:
            return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        
        case OscType::PWM: {
            float pwm = (phase < pw) ? 1.0f : -1.0f;
            return pwm;
        }
        
        case OscType::Noise:
            return (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        
        default:
            return 0.0f;
    }
}

float VirtualAnalogSynth::AnalogVoice::polyBLEP(float phase, float phaseInc) {
    if (phase < phaseInc) {
        float t = phase / phaseInc;
        return t + t - t * t - 1.0f;
    } else if (phase > 1.0f - phaseInc) {
        float t = (phase - 1.0f) / phaseInc;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

float VirtualAnalogSynth::AnalogVoice::processFilter(float input) {
    auto& params = synth.getParameters();
    float cutoff = params.filter.cutoff;
    cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);
    
    float g = std::tan(juce::MathConstants<float>::pi * cutoff / (float)sampleRate);
    float k = 2.0f - 2.0f * params.filter.resonance;
    
    float output = input;
    for (int pole = 0; pole < 2; ++pole) {
        float v0 = output;
        float v1 = (filterState.ic1eq[pole] + g * (v0 - filterState.ic2eq[pole])) / 
                   (1.0f + g * (g + k));
        float v2 = filterState.ic2eq[pole] + g * v1;
        
        filterState.ic1eq[pole] = 2.0f * v1 - filterState.ic1eq[pole];
        filterState.ic2eq[pole] = 2.0f * v2 - filterState.ic2eq[pole];
        
        output = v2; // LP output
    }
    
    return output;
}

float VirtualAnalogSynth::AnalogVoice::processEnvelope(EnvState& env, 
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

void VirtualAnalogSynth::AnalogVoice::updateLFOs(float dt) {
    auto& params = synth.getParameters();
    
    for (int i = 0; i < 2; ++i) {
        auto& lfo = lfoStates[i];
        auto& lfoParams = params.lfos[i];
        
        float value = 0.0f;
        switch (lfoParams.waveform) {
            case LFOParams::Sine:
                value = std::sin(2.0f * juce::MathConstants<float>::pi * lfo.phase);
                break;
            case LFOParams::Triangle:
                value = (lfo.phase < 0.5f) ? (4.0f * lfo.phase - 1.0f) : (3.0f - 4.0f * lfo.phase);
                break;
            case LFOParams::Saw:
                value = 2.0f * lfo.phase - 1.0f;
                break;
            case LFOParams::Square:
                value = (lfo.phase < 0.5f) ? 1.0f : -1.0f;
                break;
            default:
                value = 0.0f;
        }
        
        if (!lfoParams.bipolar) {
            value = (value + 1.0f) * 0.5f;
        }
        
        lfo.value = value * lfoParams.amount;
        lfo.phase += dt * lfoParams.rate;
        if (lfo.phase >= 1.0f) lfo.phase -= 1.0f;
    }
}

void VirtualAnalogSynth::AnalogVoice::updatePortamento(float dt) {
    auto& params = synth.getParameters();
    if (params.portamento > 0.0f) {
        float rate = 1.0f / (params.portamento + 0.001f);
        if (currentPitch < targetPitch) {
            currentPitch += rate * dt;
            if (currentPitch > targetPitch) currentPitch = targetPitch;
        } else if (currentPitch > targetPitch) {
            currentPitch -= rate * dt;
            if (currentPitch < targetPitch) currentPitch = targetPitch;
        }
    } else {
        currentPitch = targetPitch;
    }
}

void VirtualAnalogSynth::AnalogVoice::updateModulation() {
    // Modulation matrix processing would go here
}

} // namespace OmegaStudio
