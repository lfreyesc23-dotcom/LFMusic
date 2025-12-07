#include "CreativeEffects.h"
#include <cmath>
#include <algorithm>

namespace OmegaStudio {

//==============================================================================
// CreativeDelay Implementation
//==============================================================================
void CreativeDelay::prepare(double sr, int maxBlockSize) {
    sampleRate = sr;
    delayLineL.prepare({sr, (juce::uint32)maxBlockSize, 2});
    delayLineR.prepare({sr, (juce::uint32)maxBlockSize, 2});
    delayLineL.reset();
    delayLineR.reset();
}

void CreativeDelay::process(juce::AudioBuffer<float>& buffer, const Params& params, double bpm) {
    int numSamples = buffer.getNumSamples();
    
    // Calculate delay times
    float delayTimeL = params.delayTimeL;
    float delayTimeR = params.delayTimeR;
    
    if (params.tempoSync && bpm > 0) {
        float beatDuration = 60.0f / bpm;
        delayTimeL = beatDuration * params.syncRateL;
        delayTimeR = beatDuration * params.syncRateR;
    }
    
    int delaySamplesL = (int)(delayTimeL * sampleRate);
    int delaySamplesR = (int)(delayTimeR * sampleRate);
    
    delayLineL.setDelay((float)delaySamplesL);
    delayLineR.setDelay((float)delaySamplesR);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        float inL = buffer.getSample(0, sample);
        float inR = (buffer.getNumChannels() > 1) ? buffer.getSample(1, sample) : inL;
        
        // Get delayed samples
        float delayedL = delayLineL.popSample(0);
        float delayedR = delayLineR.popSample(0);
        
        // Apply feedback
        if (params.pingPong) {
            delayLineL.pushSample(0, inL + delayedR * params.feedback);
            delayLineR.pushSample(0, inR + delayedL * params.feedback);
        } else {
            delayLineL.pushSample(0, inL + delayedL * params.feedback);
            delayLineR.pushSample(0, inR + delayedR * params.feedback);
        }
        
        // Mix wet/dry
        float outL = inL * (1.0f - params.mix) + delayedL * params.mix;
        float outR = inR * (1.0f - params.mix) + delayedR * params.mix;
        
        buffer.setSample(0, sample, outL);
        if (buffer.getNumChannels() > 1) {
            buffer.setSample(1, sample, outR);
        }
    }
}

void CreativeDelay::reset() {
    delayLineL.reset();
    delayLineR.reset();
}

//==============================================================================
// AlgorithmicReverb Implementation
//==============================================================================
void AlgorithmicReverb::prepare(double sr, int maxBlockSize) {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = maxBlockSize;
    spec.numChannels = 2;
    
    reverb.prepare(spec);
    dryBuffer.setSize(2, maxBlockSize);
}

void AlgorithmicReverb::process(juce::AudioBuffer<float>& buffer, const Params& params) {
    dryBuffer.makeCopyOf(buffer, true);
    
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = params.roomSize;
    reverbParams.damping = params.damping;
    reverbParams.width = params.width;
    reverbParams.wetLevel = params.mix;
    reverbParams.dryLevel = 1.0f - params.mix;
    
    reverb.setParameters(reverbParams);
    
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}

void AlgorithmicReverb::reset() {
    reverb.reset();
}

//==============================================================================
// Flanger Implementation
//==============================================================================
void Flanger::prepare(double sr) {
    sampleRate = sr;
    delayLine.prepare({sr, 512, 2});
    delayLine.reset();
    lfoPhase = 0.0f;
}

void Flanger::process(juce::AudioBuffer<float>& buffer, const Params& params) {
    int numSamples = buffer.getNumSamples();
    float baseDelaySamples = params.delay * (float)sampleRate;
    float modulationRange = params.depth * baseDelaySamples;
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Calculate LFO value
        float lfoValue = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase);
        float currentDelay = baseDelaySamples + lfoValue * modulationRange;
        currentDelay = juce::jlimit(1.0f, (float)delayLine.getMaximumDelayInSamples() - 1.0f, currentDelay);
        
        delayLine.setDelay(currentDelay);
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float input = buffer.getSample(ch, sample);
            float delayed = delayLine.popSample(ch);
            
            // Feedback
            delayLine.pushSample(ch, input + delayed * params.feedback);
            
            // Mix
            float output = input * (1.0f - params.mix) + delayed * params.mix;
            buffer.setSample(ch, sample, output);
        }
        
        // Advance LFO
        lfoPhase += params.rate / (float)sampleRate;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }
}

void Flanger::reset() {
    delayLine.reset();
    lfoPhase = 0.0f;
}

//==============================================================================
// Phaser Implementation
//==============================================================================
void Phaser::prepare(double sr) {
    sampleRate = sr;
    for (auto& filter : filters) {
        filter.a1 = 0.0f;
        filter.zm1 = 0.0f;
    }
    lfoPhase = 0.0f;
}

void Phaser::process(juce::AudioBuffer<float>& buffer, const Params& params) {
    int numSamples = buffer.getNumSamples();
    int stages = std::min(params.stages, MAX_STAGES);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Calculate LFO
        float lfoValue = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase);
        float freq = params.centerFreq * std::pow(2.0f, lfoValue * params.depth);
        
        // Calculate allpass coefficient
        float tanPiF = std::tan(juce::MathConstants<float>::pi * freq / (float)sampleRate);
        float a1 = (tanPiF - 1.0f) / (tanPiF + 1.0f);
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float input = buffer.getSample(ch, sample);
            float output = input;
            
            // Process allpass stages
            for (int stage = 0; stage < stages; ++stage) {
                auto& filter = filters[stage];
                filter.a1 = a1;
                
                float allpassOut = filter.a1 * output + filter.zm1;
                filter.zm1 = output - filter.a1 * allpassOut;
                output = allpassOut;
            }
            
            // Mix with feedback
            output = input + output * params.feedback;
            output = input * (1.0f - params.mix) + output * params.mix;
            
            buffer.setSample(ch, sample, output);
        }
        
        // Advance LFO
        lfoPhase += params.rate / (float)sampleRate;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }
}

void Phaser::reset() {
    for (auto& filter : filters) {
        filter.zm1 = 0.0f;
    }
    lfoPhase = 0.0f;
}

//==============================================================================
// AdvancedChorus Implementation
//==============================================================================
void AdvancedChorus::prepare(double sr) {
    sampleRate = sr;
    for (int i = 0; i < MAX_VOICES; ++i) {
        delayLines[i].prepare({sr, 8192, 2});
        delayLines[i].reset();
        lfoPhases[i] = (float)i / MAX_VOICES; // Phase offset per voice
    }
}

void AdvancedChorus::process(juce::AudioBuffer<float>& buffer, const Params& params) {
    int numSamples = buffer.getNumSamples();
    int voices = std::min(params.voices, MAX_VOICES);
    
    juce::AudioBuffer<float> wetBuffer(buffer.getNumChannels(), numSamples);
    wetBuffer.clear();
    
    for (int sample = 0; sample < numSamples; ++sample) {
        for (int voice = 0; voice < voices; ++voice) {
            // Calculate modulated delay
            float lfoValue = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhases[voice]);
            float delayMs = 10.0f + lfoValue * params.depth * 30.0f; // 10-40ms
            float delaySamples = (delayMs / 1000.0f) * (float)sampleRate;
            
            delayLines[voice].setDelay(delaySamples);
            
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                float input = buffer.getSample(ch, sample);
                float delayed = delayLines[voice].popSample(ch);
                
                // Feedback
                delayLines[voice].pushSample(ch, input + delayed * params.feedback);
                
                // Stereo spread
                float pan = ((float)voice / (voices - 1.0f)) - 0.5f; // -0.5 to 0.5
                pan *= params.voiceSpread;
                
                float gain = 1.0f / std::sqrt((float)voices);
                if (ch == 0) gain *= (1.0f - std::max(0.0f, pan));
                if (ch == 1) gain *= (1.0f + std::min(0.0f, pan));
                
                wetBuffer.addSample(ch, sample, delayed * gain);
            }
            
            // Advance LFO
            lfoPhases[voice] += params.rate / (float)sampleRate;
            if (lfoPhases[voice] >= 1.0f) lfoPhases[voice] -= 1.0f;
        }
        
        // Mix wet/dry
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float dry = buffer.getSample(ch, sample);
            float wet = wetBuffer.getSample(ch, sample);
            buffer.setSample(ch, sample, dry * (1.0f - params.mix) + wet * params.mix);
        }
    }
}

void AdvancedChorus::reset() {
    for (auto& line : delayLines) {
        line.reset();
    }
    for (int i = 0; i < MAX_VOICES; ++i) {
        lfoPhases[i] = (float)i / MAX_VOICES;
    }
}

//==============================================================================
// Vocoder Implementation
//==============================================================================
void Vocoder::prepare(double sr) {
    sampleRate = sr;
    for (auto& band : bands) {
        band.envelope = 0.0f;
    }
}

void Vocoder::process(juce::AudioBuffer<float>& modulator,
                     juce::AudioBuffer<float>& carrier,
                     juce::AudioBuffer<float>& output,
                     const Params& params) {
    calculateBandFrequencies(params);
    
    int numSamples = modulator.getNumSamples();
    output.setSize(carrier.getNumChannels(), numSamples);
    output.clear();
    
    int numBands = std::min(params.bands, MAX_BANDS);
    
    for (int sample = 0; sample < numSamples; ++sample) {
        for (int bandIdx = 0; bandIdx < numBands; ++bandIdx) {
            auto& band = bands[bandIdx];
            
            // Process modulator (voice)
            float modSample = modulator.getSample(0, sample);
            // Apply band filter to modulator (simplified - would use proper IIR)
            float modFiltered = modSample; // TODO: Apply actual filter
            
            // Envelope follower
            float rectified = std::abs(modFiltered);
            float attackCoeff = 1.0f - std::exp(-1.0f / (params.attack * (float)sampleRate));
            float releaseCoeff = 1.0f - std::exp(-1.0f / (params.release * (float)sampleRate));
            
            if (rectified > band.envelope) {
                band.envelope += (rectified - band.envelope) * attackCoeff;
            } else {
                band.envelope += (rectified - band.envelope) * releaseCoeff;
            }
            
            // Process carrier (synth)
            for (int ch = 0; ch < carrier.getNumChannels(); ++ch) {
                float carrierSample = carrier.getSample(ch, sample);
                // Apply band filter to carrier (simplified)
                float carrierFiltered = carrierSample;
                
                // Apply modulator envelope to carrier
                float vocoded = carrierFiltered * band.envelope;
                
                output.addSample(ch, sample, vocoded / std::sqrt((float)numBands));
            }
        }
    }
    
    // Mix
    for (int ch = 0; ch < output.getNumChannels(); ++ch) {
        for (int sample = 0; sample < numSamples; ++sample) {
            float dry = carrier.getSample(ch, sample);
            float wet = output.getSample(ch, sample);
            output.setSample(ch, sample, dry * (1.0f - params.mix) + wet * params.mix);
        }
    }
}

void Vocoder::reset() {
    for (auto& band : bands) {
        band.envelope = 0.0f;
    }
}

void Vocoder::calculateBandFrequencies(const Params& params) {
    // Calculate logarithmically spaced band frequencies
    int numBands = std::min(params.bands, MAX_BANDS);
    float logLow = std::log(params.lowFreq);
    float logHigh = std::log(params.highFreq);
    float logRange = logHigh - logLow;
    
    for (int i = 0; i < numBands; ++i) {
        float ratio = (float)i / (numBands - 1);
        float freq = std::exp(logLow + ratio * logRange);
        // Band frequency calculated, would configure IIR filters here
    }
}

//==============================================================================
// DistortionSuite Implementation
//==============================================================================
void DistortionSuite::prepare(double sr) {
    sampleRate = sr;
    lastSample = 0.0f;
}

void DistortionSuite::process(juce::AudioBuffer<float>& buffer, const Params& params) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float input = buffer.getSample(ch, sample);
            float processed = processSample(input, params);
            
            // Mix
            float output = input * (1.0f - params.mix) + processed * params.mix;
            output *= params.outputGain;
            
            buffer.setSample(ch, sample, output);
        }
    }
}

float DistortionSuite::processSample(float input, const Params& params) {
    float driven = input * params.drive;
    
    switch (params.type) {
        case Type::SoftClip:
            return softClip(driven);
        
        case Type::HardClip:
            return hardClip(driven);
        
        case Type::Tube:
            return tubeDistortion(driven);
        
        case Type::Foldback:
            return foldback(driven);
        
        case Type::Bitcrush: {
            float levels = std::pow(2.0f, (float)params.bitDepth);
            float quantized = std::floor(input * levels + 0.5f) / levels;
            
            // Sample rate reduction
            if (params.sampleRateReduction > 1.0f) {
                float mod = std::fmod((float)sampleRate, params.sampleRateReduction);
                if (mod < 1.0f) {
                    lastSample = quantized;
                }
                return lastSample;
            }
            return quantized;
        }
        
        case Type::Waveshaper: {
            // Polynomial waveshaper
            float x = juce::jlimit(-1.0f, 1.0f, driven);
            return x - (x * x * x) / 3.0f;
        }
        
        default:
            return input;
    }
}

float DistortionSuite::softClip(float x) {
    return std::tanh(x);
}

float DistortionSuite::hardClip(float x) {
    return juce::jlimit(-1.0f, 1.0f, x);
}

float DistortionSuite::tubeDistortion(float x) {
    // Tube-like saturation curve
    if (x > 0.0f) {
        return 1.0f - std::exp(-x);
    } else {
        return -1.0f + std::exp(x);
    }
}

float DistortionSuite::foldback(float x) {
    // Wave folding
    while (x > 1.0f) x = 2.0f - x;
    while (x < -1.0f) x = -2.0f - x;
    return x;
}

} // namespace OmegaStudio
