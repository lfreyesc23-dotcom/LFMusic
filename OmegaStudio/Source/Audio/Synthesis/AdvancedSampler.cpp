#include "AdvancedSampler.h"
#include <cmath>
#include <algorithm>

namespace OmegaStudio {

//==============================================================================
// SamplerVoice Implementation
//==============================================================================
class AdvancedSampler::SamplerVoice : public juce::SynthesiserVoice {
public:
    SamplerVoice(AdvancedSampler& owner) : sampler(owner) {}
    
    bool canPlaySound(juce::SynthesiserSound*) override { return true; }
    
    void startNote(int midiNoteNumber, float velocity,
                  juce::SynthesiserSound*, int) override {
        noteNumber = midiNoteNumber;
        noteVelocity = velocity;
        sampleRate = getSampleRate();
        
        // Find matching sample
        currentSample = findMatchingSample(midiNoteNumber, velocity);
        if (!currentSample || !currentSample->loaded) {
            clearCurrentNote();
            return;
        }
        
        // Initialize playback
        playbackPosition = 0.0;
        direction = 1.0; // Forward
        isLooping = (currentSample->loopMode != LoopMode::None);
        
        // Start envelopes
        ampEnv.stage = EnvStage::Attack;
        ampEnv.level = 0.0f;
        filterEnv.stage = EnvStage::Attack;
        filterEnv.level = 0.0f;
        
        // Calculate pitch ratio
        int pitchDiff = midiNoteNumber - currentSample->rootNote;
        pitchDiff += currentSample->transpose;
        float cents = currentSample->fineTune / 100.0f;
        pitchRatio = std::pow(2.0f, (pitchDiff + cents) / 12.0f);
    }
    
    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) {
            ampEnv.stage = EnvStage::Release;
            ampEnv.releaseLevel = ampEnv.level;
            filterEnv.stage = EnvStage::Release;
        } else {
            clearCurrentNote();
            ampEnv.stage = EnvStage::Idle;
        }
    }
    
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override {
        if (!currentSample || ampEnv.stage == EnvStage::Idle) {
            return;
        }
        
        auto* layer = getCurrentLayer();
        if (!layer) return;
        
        float dt = 1.0f / (float)sampleRate;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Update envelopes
            float ampLevel = processEnvelope(ampEnv, layer->ampEnv, dt);
            float filterLevel = processEnvelope(filterEnv, layer->filterEnv, dt);
            
            // Get sample value
            float sampleValue = getSampleValue();
            
            // Apply filter if enabled
            if (layer->filterEnabled) {
                sampleValue = processFilter(sampleValue, layer->filterCutoff * 
                    std::pow(2.0f, filterLevel * 5.0f), layer->filterResonance);
            }
            
            // Apply envelope and velocity
            sampleValue *= ampLevel * noteVelocity * layer->volume;
            
            // Write to output (stereo)
            float leftGain = (1.0f - std::max(0.0f, layer->pan)) * 0.7f;
            float rightGain = (1.0f + std::min(0.0f, layer->pan)) * 0.7f;
            
            if (outputBuffer.getNumChannels() > 0) {
                outputBuffer.addSample(0, startSample + sample, sampleValue * leftGain);
            }
            if (outputBuffer.getNumChannels() > 1) {
                outputBuffer.addSample(1, startSample + sample, sampleValue * rightGain);
            }
            
            // Advance playback
            advancePlayback();
            
            // Check if sample ended
            if (playbackEnded && !isLooping) {
                if (ampEnv.stage != EnvStage::Release) {
                    ampEnv.stage = EnvStage::Release;
                    ampEnv.releaseLevel = ampEnv.level;
                }
            }
            
            if (ampEnv.stage == EnvStage::Idle) {
                clearCurrentNote();
                break;
            }
        }
    }
    
private:
    AdvancedSampler& sampler;
    std::shared_ptr<Sample> currentSample;
    
    int noteNumber = 0;
    float noteVelocity = 0.0f;
    double sampleRate = 44100.0;
    
    // Playback state
    double playbackPosition = 0.0;
    double pitchRatio = 1.0;
    double direction = 1.0;
    bool isLooping = false;
    bool playbackEnded = false;
    
    // Envelope state
    enum class EnvStage { Attack, Decay, Sustain, Release, Idle };
    struct EnvState {
        EnvStage stage = EnvStage::Idle;
        float level = 0.0f;
        float releaseLevel = 0.0f;
    };
    EnvState ampEnv, filterEnv;
    
    // Filter state
    struct FilterState {
        float ic1eq = 0.0f, ic2eq = 0.0f;
    } filterState;
    
    std::shared_ptr<Sample> findMatchingSample(int note, float velocity) {
        auto& params = sampler.getParameters();
        int vel = (int)(velocity * 127.0f);
        
        for (auto& layer : params.layers) {
            for (auto& sample : layer.samples) {
                if (!sample->loaded) continue;
                if (note >= sample->keyLow && note <= sample->keyHigh &&
                    vel >= sample->velLow && vel <= sample->velHigh) {
                    return sample;
                }
            }
        }
        return nullptr;
    }
    
    Layer* getCurrentLayer() {
        auto& params = sampler.getParameters();
        for (auto& layer : params.layers) {
            for (auto& sample : layer.samples) {
                if (sample == currentSample) {
                    return &layer;
                }
            }
        }
        return nullptr;
    }
    
    float getSampleValue() {
        if (!currentSample || playbackPosition < 0) return 0.0f;
        
        auto& buffer = currentSample->buffer;
        int numSamples = buffer.getNumSamples();
        if (numSamples == 0) return 0.0f;
        
        // Linear interpolation
        int pos = (int)playbackPosition;
        float frac = playbackPosition - pos;
        
        if (pos >= numSamples - 1) {
            playbackEnded = true;
            return 0.0f;
        }
        
        float sample1 = buffer.getSample(0, pos);
        float sample2 = buffer.getSample(0, std::min(pos + 1, numSamples - 1));
        
        return sample1 * (1.0f - frac) + sample2 * frac;
    }
    
    void advancePlayback() {
        if (!currentSample) return;
        
        playbackPosition += pitchRatio * direction;
        
        int loopStart = currentSample->loopStart;
        int loopEnd = (currentSample->loopEnd < 0) ? 
            currentSample->buffer.getNumSamples() : currentSample->loopEnd;
        
        switch (currentSample->loopMode) {
            case LoopMode::Forward:
                if (playbackPosition >= loopEnd) {
                    playbackPosition = loopStart + (playbackPosition - loopEnd);
                }
                break;
            
            case LoopMode::Reverse:
                if (playbackPosition <= loopStart) {
                    playbackPosition = loopEnd - (loopStart - playbackPosition);
                }
                break;
            
            case LoopMode::PingPong:
                if (direction > 0 && playbackPosition >= loopEnd) {
                    direction = -1.0;
                    playbackPosition = loopEnd - (playbackPosition - loopEnd);
                } else if (direction < 0 && playbackPosition <= loopStart) {
                    direction = 1.0;
                    playbackPosition = loopStart + (loopStart - playbackPosition);
                }
                break;
            
            case LoopMode::OneShot:
            case LoopMode::None:
                if (playbackPosition >= currentSample->buffer.getNumSamples()) {
                    playbackEnded = true;
                }
                break;
        }
    }
    
    float processEnvelope(EnvState& env, const Layer::Envelope& params, float dt) {
        switch (env.stage) {
            case EnvStage::Attack:
                if (params.attack > 0.0001f) {
                    env.level += dt / params.attack;
                    if (env.level >= 1.0f) {
                        env.level = 1.0f;
                        env.stage = EnvStage::Decay;
                    }
                } else {
                    env.level = 1.0f;
                    env.stage = EnvStage::Decay;
                }
                break;
            
            case EnvStage::Decay:
                if (params.decay > 0.0001f) {
                    env.level -= (1.0f - params.sustain) * dt / params.decay;
                    if (env.level <= params.sustain) {
                        env.level = params.sustain;
                        env.stage = EnvStage::Sustain;
                    }
                } else {
                    env.level = params.sustain;
                    env.stage = EnvStage::Sustain;
                }
                break;
            
            case EnvStage::Sustain:
                env.level = params.sustain;
                break;
            
            case EnvStage::Release:
                if (params.release > 0.0001f) {
                    env.level -= env.releaseLevel * dt / params.release;
                    if (env.level <= 0.0f) {
                        env.level = 0.0f;
                        env.stage = EnvStage::Idle;
                    }
                } else {
                    env.level = 0.0f;
                    env.stage = EnvStage::Idle;
                }
                break;
            
            case EnvStage::Idle:
                env.level = 0.0f;
                break;
        }
        return env.level;
    }
    
    float processFilter(float input, float cutoff, float resonance) {
        cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);
        float g = std::tan(juce::MathConstants<float>::pi * cutoff / (float)sampleRate);
        float k = 2.0f - 2.0f * resonance;
        
        float v0 = input;
        float v1 = (filterState.ic1eq + g * (v0 - filterState.ic2eq)) / (1.0f + g * (g + k));
        float v2 = filterState.ic2eq + g * v1;
        
        filterState.ic1eq = 2.0f * v1 - filterState.ic1eq;
        filterState.ic2eq = 2.0f * v2 - filterState.ic2eq;
        
        return v2; // LP output
    }
};

//==============================================================================
// AdvancedSampler Implementation
//==============================================================================
AdvancedSampler::AdvancedSampler() {
    for (int i = 0; i < 64; ++i) {
        addVoice(new SamplerVoice(*this));
    }
    
    struct SamplerSound : public juce::SynthesiserSound {
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };
    
    addSound(new SamplerSound());
}

AdvancedSampler::~AdvancedSampler() = default;

void AdvancedSampler::prepare(const juce::dsp::ProcessSpec& spec) {
    currentSpec = spec;
    setCurrentPlaybackSampleRate(spec.sampleRate);
}

void AdvancedSampler::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                     const juce::MidiBuffer& midiMessages,
                                     int startSample, int numSamples) {
    Synthesiser::renderNextBlock(outputBuffer, midiMessages, startSample, numSamples);
    outputBuffer.applyGain(params.masterVolume);
}

bool AdvancedSampler::loadSample(const juce::File& file, int layerIndex) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) {
        return false;
    }
    
    auto sample = std::make_shared<Sample>();
    sample->name = file.getFileNameWithoutExtension();
    sample->sampleRate = reader->sampleRate;
    sample->buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&sample->buffer, 0, (int)reader->lengthInSamples, 0, true, true);
    sample->loaded = true;
    
    // Default mapping
    sample->keyLow = 0;
    sample->keyHigh = 127;
    sample->velLow = 0;
    sample->velHigh = 127;
    
    // Ensure layer exists
    if (layerIndex >= params.layers.size()) {
        Layer layer;
        layer.name = "Layer " + juce::String(layerIndex + 1);
        params.layers.push_back(layer);
    }
    
    params.layers[layerIndex].samples.push_back(sample);
    return true;
}

void AdvancedSampler::addLayer(const Layer& layer) {
    params.layers.push_back(layer);
}

void AdvancedSampler::clearAllSamples() {
    params.layers.clear();
}

void AdvancedSampler::setParameters(const SamplerParams& newParams) {
    params = newParams;
}

} // namespace OmegaStudio
