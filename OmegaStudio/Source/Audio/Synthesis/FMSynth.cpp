#include "FMSynth.h"
#include <cmath>

namespace OmegaStudio {

// Initialize static members
std::vector<FMSynth::Algorithm> FMSynth::algorithms;

//==============================================================================
// FMSynth Implementation
//==============================================================================
FMSynth::FMSynth() {
    // Initialize algorithms if not done
    if (algorithms.empty()) {
        initializeAlgorithms();
    }
    
    // Add voices
    for (int i = 0; i < 16; ++i) {
        addVoice(new FMVoice(*this));
    }
    
    // Simple sound class
    struct FMSound : public juce::SynthesiserSound {
        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };
    
    // Add dummy sound
    addSound(new FMSound());
    
    initializeFactoryPresets();
}

FMSynth::~FMSynth() = default;

void FMSynth::prepare(const juce::dsp::ProcessSpec& spec) {
    currentSpec = spec;
    setCurrentPlaybackSampleRate(spec.sampleRate);
}

void FMSynth::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                              const juce::MidiBuffer& midiMessages,
                              int startSample, int numSamples) {
    auto startTime = juce::Time::getCurrentTime();
    
    Synthesiser::renderNextBlock(outputBuffer, midiMessages, startSample, numSamples);
    
    outputBuffer.applyGain(params.masterVolume);
    
    auto elapsedMs = (juce::Time::getCurrentTime() - startTime).inMilliseconds();
    double blockTimeMs = (numSamples * 1000.0) / currentSpec.sampleRate;
    cpuUsage.store((elapsedMs / blockTimeMs) * 100.0);
}

void FMSynth::setParameters(const SynthParams& newParams) {
    params = newParams;
}

void FMSynth::setAlgorithm(int algorithmId) {
    if (algorithmId >= 0 && algorithmId < NUM_ALGORITHMS) {
        params.algorithmId = algorithmId;
    }
}

std::vector<FMSynth::Algorithm> FMSynth::getAllAlgorithms() {
    if (algorithms.empty()) {
        initializeAlgorithms();
    }
    return algorithms;
}

FMSynth::Algorithm FMSynth::getAlgorithm(int id) {
    if (algorithms.empty()) {
        initializeAlgorithms();
    }
    if (id >= 0 && id < (int)algorithms.size()) {
        return algorithms[id];
    }
    return algorithms[0];
}

void FMSynth::loadPreset(const Preset& preset) {
    params = preset.params;
}

FMSynth::Preset FMSynth::getCurrentPreset() const {
    Preset preset;
    preset.name = "Current";
    preset.category = "User";
    preset.params = params;
    return preset;
}

int FMSynth::getActiveVoiceCount() const {
    int count = 0;
    for (int i = 0; i < getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<FMVoice*>(getVoice(i))) {
            if (voice->isVoiceActive()) {
                ++count;
            }
        }
    }
    return count;
}

//==============================================================================
// Algorithm Initialization
//==============================================================================
void FMSynth::initializeAlgorithms() {
    algorithms.clear();
    algorithms.resize(NUM_ALGORITHMS);
    
    // Algorithm 1: 6 carriers (no modulation) - Organ
    {
        auto& alg = algorithms[0];
        alg.id = 0;
        alg.name = "6 Carriers";
        alg.isCarrier = {true, true, true, true, true, true};
        alg.feedbackOp = -1;
    }
    
    // Algorithm 2: Simple stack 1->2->3->4->5->6
    {
        auto& alg = algorithms[1];
        alg.id = 1;
        alg.name = "Full Stack";
        alg.routing[1][0] = 1;  // Op 1 modulates Op 2
        alg.routing[2][1] = 1;  // Op 2 modulates Op 3
        alg.routing[3][2] = 1;  // Op 3 modulates Op 4
        alg.routing[4][3] = 1;  // Op 4 modulates Op 5
        alg.routing[5][4] = 1;  // Op 5 modulates Op 6
        alg.isCarrier = {false, false, false, false, false, true};
        alg.feedbackOp = 0;
    }
    
    // Algorithm 3: Two 3-op stacks (classic DX7)
    {
        auto& alg = algorithms[2];
        alg.id = 2;
        alg.name = "Two Stacks";
        alg.routing[1][0] = 1;  // Stack 1: 1->2->3
        alg.routing[2][1] = 1;
        alg.routing[4][3] = 1;  // Stack 2: 4->5->6
        alg.routing[5][4] = 1;
        alg.isCarrier = {false, false, true, false, false, true};
        alg.feedbackOp = 0;
    }
    
    // Algorithm 4: Bell sound (1+2->3, 4+5->6)
    {
        auto& alg = algorithms[3];
        alg.id = 3;
        alg.name = "Bell";
        alg.routing[2][0] = 1;  // Op 1 modulates Op 3
        alg.routing[2][1] = 1;  // Op 2 modulates Op 3
        alg.routing[5][3] = 1;  // Op 4 modulates Op 6
        alg.routing[5][4] = 1;  // Op 5 modulates Op 6
        alg.isCarrier = {false, false, true, false, false, true};
        alg.feedbackOp = -1;
    }
    
    // Algorithm 5: E.Piano (parallel modulators)
    {
        auto& alg = algorithms[4];
        alg.id = 4;
        alg.name = "E.Piano";
        alg.routing[3][0] = 1;  // Op 1 modulates Op 4
        alg.routing[3][1] = 1;  // Op 2 modulates Op 4
        alg.routing[3][2] = 1;  // Op 3 modulates Op 4
        alg.routing[5][4] = 1;  // Op 5 modulates Op 6
        alg.isCarrier = {false, false, false, true, false, true};
        alg.feedbackOp = 0;
    }
    
    // Algorithm 6: Bass (1->2, 3->4, 5->6 all carriers)
    {
        auto& alg = algorithms[5];
        alg.id = 5;
        alg.name = "Bass";
        alg.routing[1][0] = 1;
        alg.routing[3][2] = 1;
        alg.routing[5][4] = 1;
        alg.isCarrier = {false, true, false, true, false, true};
        alg.feedbackOp = 0;
    }
    
    // Fill remaining algorithms with variations
    for (int i = 6; i < NUM_ALGORITHMS; ++i) {
        auto& alg = algorithms[i];
        alg.id = i;
        alg.name = "Algorithm " + juce::String(i + 1);
        // Default: all carriers
        alg.isCarrier.fill(true);
        alg.feedbackOp = -1;
    }
}

//==============================================================================
// Factory Presets
//==============================================================================
void FMSynth::initializeFactoryPresets() {
    // Preset 1: E.Piano
    {
        Preset preset;
        preset.name = "Electric Piano";
        preset.category = "Keys";
        preset.params.algorithmId = 4;
        preset.params.feedback = 0.0f;
        
        // Configure operators for E.Piano sound
        for (int i = 0; i < 4; ++i) {
            auto& op = preset.params.operators[i];
            op.outputLevel = 90.0f - i * 10.0f;
            op.coarse = (i == 0) ? 1.0f : (float)(i + 1);
            op.envelope.rate1 = 99.0f;
            op.envelope.rate2 = 90.0f;
            op.envelope.rate3 = 70.0f;
            op.envelope.rate4 = 60.0f;
            op.envelope.level1 = 99.0f;
            op.envelope.level2 = 85.0f;
            op.envelope.level3 = 70.0f;
        }
        
        factoryPresets.push_back(preset);
    }
    
    // Preset 2: FM Bass
    {
        Preset preset;
        preset.name = "FM Bass";
        preset.category = "Bass";
        preset.params.algorithmId = 5;
        preset.params.feedback = 3.0f;
        
        for (int i = 0; i < 6; ++i) {
            auto& op = preset.params.operators[i];
            op.outputLevel = 99.0f;
            op.coarse = 1.0f;
            op.envelope.rate1 = 99.0f;
            op.envelope.rate2 = 80.0f;
            op.envelope.rate3 = 60.0f;
            op.envelope.rate4 = 50.0f;
            op.envelope.level1 = 99.0f;
            op.envelope.level2 = 90.0f;
            op.envelope.level3 = 80.0f;
        }
        
        factoryPresets.push_back(preset);
    }
    
    // Preset 3: Bell
    {
        Preset preset;
        preset.name = "Bell";
        preset.category = "Lead";
        preset.params.algorithmId = 3;
        preset.params.feedback = 0.0f;
        
        for (int i = 0; i < 6; ++i) {
            auto& op = preset.params.operators[i];
            op.outputLevel = 80.0f;
            op.coarse = (float)(i % 3 + 1);
            op.envelope.rate1 = 99.0f;
            op.envelope.rate2 = 50.0f;
            op.envelope.rate3 = 30.0f;
            op.envelope.rate4 = 20.0f;
            op.envelope.level1 = 99.0f;
            op.envelope.level2 = 70.0f;
            op.envelope.level3 = 40.0f;
        }
        
        factoryPresets.push_back(preset);
    }
}

std::vector<FMSynth::Preset> FMSynth::getFactoryPresets() {
    FMSynth temp;
    return temp.factoryPresets;
}

//==============================================================================
// FMVoice Implementation
//==============================================================================
FMSynth::FMVoice::FMVoice(FMSynth& owner) : synth(owner) {}

void FMSynth::FMVoice::startNote(int midiNoteNumber, float vel,
                                 juce::SynthesiserSound*, int) {
    noteNumber = midiNoteNumber;
    velocity = vel;
    sampleRate = getSampleRate();
    
    auto& params = synth.getParameters();
    
    // Initialize operators
    for (int i = 0; i < NUM_OPERATORS; ++i) {
        auto& op = opStates[i];
        op.phase = 0.0f;
        op.output = 0.0f;
        op.feedbackSample = 0.0f;
        
        if (params.operators[i].enabled) {
            op.envStage = OpState::R1;
            op.envLevel = 0.0f;
            op.envTarget = params.operators[i].envelope.level1 / 99.0f;
            op.isActive = true;
        } else {
            op.envStage = OpState::Off;
            op.isActive = false;
        }
    }
    
    // Initialize LFO
    if (params.lfo.sync) {
        lfoState.phase = 0.0f;
    }
    lfoState.delayCounter = 0.0f;
    lfoState.active = false;
    
    // Initialize pitch envelope
    pitchEnvState.stage = PitchEnvState::R1;
    pitchEnvState.level = params.pitchEnv.level1 / 99.0f;
}

void FMSynth::FMVoice::stopNote(float, bool allowTailOff) {
    if (allowTailOff) {
        // Trigger release stage for all operators
        for (auto& op : opStates) {
            if (op.isActive && op.envStage != OpState::Off) {
                op.envStage = OpState::R4;
            }
        }
        pitchEnvState.stage = PitchEnvState::R4;
    } else {
        clearCurrentNote();
        for (auto& op : opStates) {
            op.envStage = OpState::Off;
            op.isActive = false;
        }
    }
}

void FMSynth::FMVoice::pitchWheelMoved(int newPitchWheelValue) {
    // Convert MIDI pitch wheel (0-16383) to -1.0 to +1.0
    pitchBend = ((newPitchWheelValue / 8192.0f) - 1.0f);
}

void FMSynth::FMVoice::controllerMoved(int, int) {
    // Handle MIDI CC if needed
}

void FMSynth::FMVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                                       int startSample, int numSamples) {
    // Check if any operator is active
    bool anyActive = false;
    for (const auto& op : opStates) {
        if (op.isActive) {
            anyActive = true;
            break;
        }
    }
    
    if (!anyActive) {
        clearCurrentNote();
        return;
    }
    
    auto& params = synth.getParameters();
    const auto& algorithm = FMSynth::getAlgorithm(params.algorithmId);
    float dt = 1.0f / (float)sampleRate;
    
    for (int sample = 0; sample < numSamples; ++sample) {
        // Update LFO
        updateLFO(dt);
        
        // Update pitch envelope
        updatePitchEnvelope(dt);
        
        // Calculate base pitch with pitch bend
        float basePitch = (float)noteNumber + (pitchBend * params.pitchBendRange);
        basePitch += params.transpose;
        
        // Process operators in dependency order (reverse for modulators->carriers)
        std::array<float, NUM_OPERATORS> operatorOutputs;
        operatorOutputs.fill(0.0f);
        
        // Process in reverse order (modulators first)
        for (int i = NUM_OPERATORS - 1; i >= 0; --i) {
            if (!opStates[i].isActive) {
                continue;
            }
            
            // Calculate modulation input from other operators
            float modulation = 0.0f;
            for (int j = 0; j < NUM_OPERATORS; ++j) {
                if (algorithm.routing[i][j] != 0) {
                    modulation += operatorOutputs[j];
                }
            }
            
            // Add feedback if this operator has it
            if (algorithm.feedbackOp == i) {
                float fbAmount = params.feedback / 7.0f;
                modulation += opStates[i].feedbackSample * fbAmount;
            }
            
            // Process operator
            operatorOutputs[i] = processOperator(i, modulation, dt);
            
            // Store for feedback
            if (algorithm.feedbackOp == i) {
                opStates[i].feedbackSample = operatorOutputs[i];
            }
        }
        
        // Sum carrier operators to output
        float outputSample = 0.0f;
        int carrierCount = 0;
        for (int i = 0; i < NUM_OPERATORS; ++i) {
            if (algorithm.isCarrier[i] && opStates[i].isActive) {
                outputSample += operatorOutputs[i];
                ++carrierCount;
            }
        }
        
        // Normalize by carrier count
        if (carrierCount > 0) {
            outputSample /= std::sqrt((float)carrierCount);
        }
        
        // Apply velocity
        outputSample *= velocity;
        
        // Write to output (mono for now)
        if (outputBuffer.getNumChannels() > 0) {
            outputBuffer.addSample(0, startSample + sample, outputSample * 0.3f);
        }
        if (outputBuffer.getNumChannels() > 1) {
            outputBuffer.addSample(1, startSample + sample, outputSample * 0.3f);
        }
    }
}

float FMSynth::FMVoice::processOperator(int opIndex, float modulation, float dt) {
    auto& params = synth.getParameters();
    auto& opParams = params.operators[opIndex];
    auto& opState = opStates[opIndex];
    
    // Update envelope
    updateEnvelope(opIndex, dt);
    
    if (opState.envStage == OpState::Off) {
        opState.isActive = false;
        return 0.0f;
    }
    
    // Calculate frequency
    float freq = getOperatorFrequency(opIndex, (float)noteNumber);
    
    // Apply LFO pitch modulation
    if (lfoState.active) {
        float pitchMod = lfoState.value * (params.lfo.pitchModDepth / 99.0f) * 
                        (opParams.ampModSens / 3.0f);
        freq *= std::pow(2.0f, pitchMod / 12.0f); // Convert to octaves
    }
    
    // Phase modulation (FM synthesis core)
    float modulatedPhase = opState.phase + modulation;
    
    // Generate sine wave
    float output = std::sin(2.0f * juce::MathConstants<float>::pi * modulatedPhase);
    
    // Apply envelope
    output *= opState.envLevel;
    
    // Apply output level
    output *= dxLevelToLinear(opParams.outputLevel);
    
    // Apply LFO amplitude modulation
    if (lfoState.active) {
        float ampMod = 1.0f + (lfoState.value * (params.lfo.ampModDepth / 99.0f) * 
                              (opParams.ampModSens / 3.0f));
        output *= ampMod;
    }
    
    // Advance phase
    opState.phase += freq / (float)sampleRate;
    if (opState.phase >= 1.0f) {
        opState.phase -= 1.0f;
    }
    
    opState.output = output;
    return output;
}

void FMSynth::FMVoice::updateEnvelope(int opIndex, float dt) {
    auto& params = synth.getParameters();
    auto& opParams = params.operators[opIndex];
    auto& opState = opStates[opIndex];
    auto& env = opParams.envelope;
    
    // Calculate keyboard scaling
    float keyScale = 1.0f;
    if (opParams.rateScaling > 0.0f) {
        float keyDiff = (noteNumber - opParams.breakpoint) / 12.0f; // Octaves
        keyScale = 1.0f + (keyDiff * opParams.rateScaling / 7.0f);
        keyScale = std::max(0.1f, keyScale);
    }
    
    switch (opState.envStage) {
        case OpState::R1: // Attack
        {
            float rate = dxRateToTime(env.rate1, keyScale);
            if (rate > 0.0001f) {
                float target = dxLevelToLinear(env.level1);
                opState.envLevel += (target / rate) * dt;
                if (opState.envLevel >= target) {
                    opState.envLevel = target;
                    opState.envStage = OpState::R2;
                }
            } else {
                opState.envLevel = dxLevelToLinear(env.level1);
                opState.envStage = OpState::R2;
            }
            break;
        }
        
        case OpState::R2: // Decay 1
        {
            float rate = dxRateToTime(env.rate2, keyScale);
            if (rate > 0.0001f) {
                float target = dxLevelToLinear(env.level2);
                float delta = (opState.envLevel - target) / rate * dt;
                opState.envLevel -= delta;
                if (opState.envLevel <= target) {
                    opState.envLevel = target;
                    opState.envStage = OpState::R3;
                }
            } else {
                opState.envLevel = dxLevelToLinear(env.level2);
                opState.envStage = OpState::R3;
            }
            break;
        }
        
        case OpState::R3: // Sustain/Decay 2
        {
            float rate = dxRateToTime(env.rate3, keyScale);
            if (rate > 0.0001f) {
                float target = dxLevelToLinear(env.level3);
                float delta = (opState.envLevel - target) / rate * dt;
                opState.envLevel -= delta;
                if (opState.envLevel <= target) {
                    opState.envLevel = target;
                }
            } else {
                opState.envLevel = dxLevelToLinear(env.level3);
            }
            break;
        }
        
        case OpState::R4: // Release
        {
            float rate = dxRateToTime(env.rate4, keyScale);
            if (rate > 0.0001f) {
                opState.envLevel -= (opState.envLevel / rate) * dt;
                if (opState.envLevel <= 0.001f) {
                    opState.envLevel = 0.0f;
                    opState.envStage = OpState::Off;
                }
            } else {
                opState.envLevel = 0.0f;
                opState.envStage = OpState::Off;
            }
            break;
        }
        
        case OpState::Off:
            opState.envLevel = 0.0f;
            break;
    }
}

void FMSynth::FMVoice::updateLFO(float dt) {
    auto& params = synth.getParameters();
    
    // Handle LFO delay
    if (!lfoState.active) {
        lfoState.delayCounter += dt;
        float delayTime = (params.lfo.delay / 99.0f) * 2.0f; // 0-2 seconds
        if (lfoState.delayCounter >= delayTime) {
            lfoState.active = true;
        } else {
            lfoState.value = 0.0f;
            return;
        }
    }
    
    // Calculate LFO frequency (0.1 - 20 Hz)
    float lfoFreq = 0.1f + (params.lfo.speed / 99.0f) * 19.9f;
    
    // Generate LFO waveform
    float value = 0.0f;
    switch (params.lfo.waveform) {
        case LFOParams::Sine:
            value = std::sin(2.0f * juce::MathConstants<float>::pi * lfoState.phase);
            break;
        case LFOParams::Triangle:
            value = (lfoState.phase < 0.5f) ? (4.0f * lfoState.phase - 1.0f)
                                            : (3.0f - 4.0f * lfoState.phase);
            break;
        case LFOParams::SawDown:
            value = 1.0f - 2.0f * lfoState.phase;
            break;
        case LFOParams::SawUp:
            value = 2.0f * lfoState.phase - 1.0f;
            break;
        case LFOParams::Square:
            value = (lfoState.phase < 0.5f) ? 1.0f : -1.0f;
            break;
        case LFOParams::SampleHold:
            if (lfoState.phase < dt * lfoFreq) {
                value = (std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            } else {
                value = lfoState.value;
            }
            break;
    }
    
    lfoState.value = value;
    
    // Advance phase
    lfoState.phase += dt * lfoFreq;
    if (lfoState.phase >= 1.0f) {
        lfoState.phase -= 1.0f;
    }
}

void FMSynth::FMVoice::updatePitchEnvelope(float dt) {
    // Simplified pitch envelope (not fully implemented)
    // Would modulate the pitch based on envelope stages
}

float FMSynth::FMVoice::getOperatorFrequency(int opIndex, float basePitch) {
    auto& params = synth.getParameters();
    auto& opParams = params.operators[opIndex];
    
    if (opParams.fixed) {
        return opParams.fixedFreq;
    }
    
    // Calculate base frequency from MIDI note
    float baseFreq = 440.0f * std::pow(2.0f, (basePitch - 69.0f) / 12.0f);
    
    // Apply coarse tuning
    float freq = baseFreq * opParams.coarse;
    
    // Apply fine tuning (cents)
    freq *= std::pow(2.0f, opParams.fine / 1200.0f);
    
    return freq;
}

float FMSynth::FMVoice::dxLevelToLinear(float dxLevel) {
    return FMHelpers::dxLevelToLinear(dxLevel);
}

float FMSynth::FMVoice::dxRateToTime(float rate, float keyScale) {
    float baseTime = FMHelpers::dxRateToSeconds(rate);
    return baseTime / keyScale;
}

//==============================================================================
// FMHelpers Implementation
//==============================================================================
float FMHelpers::dxLevelToLinear(float dxLevel) {
    // DX7 uses logarithmic levels (0-99)
    // Approximate conversion
    return std::pow(dxLevel / 99.0f, 2.0f);
}

float FMHelpers::dxRateToSeconds(float rate) {
    // DX7 rate conversion (approximate)
    // Rate 99 = ~0.001s, Rate 0 = ~10s
    if (rate >= 99.0f) return 0.001f;
    if (rate <= 0.0f) return 10.0f;
    
    return 10.0f * std::pow(0.0001f, rate / 99.0f);
}

float FMHelpers::linearToDxLevel(float linear) {
    return std::sqrt(linear) * 99.0f;
}

float FMHelpers::secondsToDxRate(float seconds) {
    if (seconds <= 0.001f) return 99.0f;
    if (seconds >= 10.0f) return 0.0f;
    
    return 99.0f * std::log(seconds / 10.0f) / std::log(0.0001f);
}

juce::String FMHelpers::getAlgorithmDescription(int algorithmId) {
    auto alg = FMSynth::getAlgorithm(algorithmId);
    return alg.name;
}

juce::String FMHelpers::getAlgorithmDiagram(int algorithmId) {
    // Return ASCII diagram of algorithm routing
    return "Algorithm " + juce::String(algorithmId + 1);
}

} // namespace OmegaStudio
