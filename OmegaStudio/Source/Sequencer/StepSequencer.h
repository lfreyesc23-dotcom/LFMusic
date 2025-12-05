#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>
#include <memory>
#include <functional>

namespace OmegaStudio {
namespace Sequencer {

/**
 * @class Step
 * @brief Representa un paso individual en el step sequencer
 * 
 * Cada step tiene:
 * - Velocity (0-127)
 * - Probability (0-100%)
 * - Ratcheting (subdivisiones)
 * - Slide/Tie
 * - Accent
 * - Mute
 */
struct Step {
    bool active { false };           // Step activado
    uint8_t velocity { 100 };        // Velocity MIDI (0-127)
    float probability { 1.0f };      // Probabilidad de trigger (0.0-1.0)
    int ratcheting { 1 };            // Subdivisiones (1=normal, 2=double, 4=quad)
    bool slide { false };            // Slide/glide a siguiente nota
    bool accent { false };           // Accent (velocity boost)
    bool mute { false };             // Mute individual
    int8_t microTiming { 0 };        // Micro-timing offset (-50 a +50 ticks)
    uint8_t noteNumber { 60 };       // Nota MIDI (C4 por defecto)
    
    // Chance modulation
    bool shouldTrigger() const {
        if (!active || mute) return false;
        return juce::Random::getSystemRandom().nextFloat() < probability;
    }
    
    uint8_t getVelocity() const {
        uint8_t vel = velocity;
        if (accent) vel = std::min(127, vel + 20);
        return vel;
    }
};

/**
 * @class StepPattern
 * @brief Un patrón de steps (generalmente 16 steps)
 */
class StepPattern {
public:
    StepPattern(int numSteps = 16, int numTracks = 8)
        : steps_(numTracks, std::vector<Step>(numSteps))
        , numSteps_(numSteps)
        , numTracks_(numTracks) {
    }
    
    // Getters/Setters
    Step& getStep(int track, int step) { 
        jassert(track >= 0 && track < numTracks_);
        jassert(step >= 0 && step < numSteps_);
        return steps_[track][step]; 
    }
    
    const Step& getStep(int track, int step) const {
        jassert(track >= 0 && track < numTracks_);
        jassert(step >= 0 && step < numSteps_);
        return steps_[track][step];
    }
    
    void setStep(int track, int step, const Step& s) {
        jassert(track >= 0 && track < numTracks_);
        jassert(step >= 0 && step < numSteps_);
        steps_[track][step] = s;
    }
    
    void toggleStep(int track, int step) {
        auto& s = getStep(track, step);
        s.active = !s.active;
    }
    
    void clearPattern() {
        for (auto& track : steps_) {
            for (auto& step : track) {
                step = Step{};
            }
        }
    }
    
    void clearTrack(int track) {
        for (auto& step : steps_[track]) {
            step = Step{};
        }
    }
    
    int getNumSteps() const { return numSteps_; }
    int getNumTracks() const { return numTracks_; }
    
    // Pattern manipulation
    void rotate(int track, int amount) {
        std::rotate(steps_[track].begin(), 
                   steps_[track].begin() + (amount % numSteps_), 
                   steps_[track].end());
    }
    
    void reverse(int track) {
        std::reverse(steps_[track].begin(), steps_[track].end());
    }
    
    void randomize(int track, float density = 0.5f) {
        auto& rand = juce::Random::getSystemRandom();
        for (auto& step : steps_[track]) {
            step.active = rand.nextFloat() < density;
            if (step.active) {
                step.velocity = rand.nextInt(juce::Range<int>(80, 127));
            }
        }
    }
    
    // Euclidean rhythm generator
    void generateEuclidean(int track, int pulses, int steps) {
        clearTrack(track);
        if (pulses <= 0 || steps <= 0) return;
        
        std::vector<int> pattern;
        int bucket = 0;
        for (int i = 0; i < steps; i++) {
            bucket += pulses;
            if (bucket >= steps) {
                bucket -= steps;
                pattern.push_back(1);
            } else {
                pattern.push_back(0);
            }
        }
        
        for (int i = 0; i < std::min((int)pattern.size(), numSteps_); i++) {
            if (pattern[i]) {
                steps_[track][i].active = true;
                steps_[track][i].velocity = 100;
            }
        }
    }
    
    // Serialization
    juce::ValueTree toValueTree() const {
        juce::ValueTree tree("StepPattern");
        tree.setProperty("numSteps", numSteps_, nullptr);
        tree.setProperty("numTracks", numTracks_, nullptr);
        
        for (int t = 0; t < numTracks_; t++) {
            juce::ValueTree trackTree("Track");
            trackTree.setProperty("index", t, nullptr);
            
            for (int s = 0; s < numSteps_; s++) {
                const auto& step = steps_[t][s];
                if (step.active) {
                    juce::ValueTree stepTree("Step");
                    stepTree.setProperty("index", s, nullptr);
                    stepTree.setProperty("velocity", step.velocity, nullptr);
                    stepTree.setProperty("probability", step.probability, nullptr);
                    stepTree.setProperty("ratcheting", step.ratcheting, nullptr);
                    stepTree.setProperty("slide", step.slide, nullptr);
                    stepTree.setProperty("accent", step.accent, nullptr);
                    stepTree.setProperty("microTiming", step.microTiming, nullptr);
                    stepTree.setProperty("noteNumber", step.noteNumber, nullptr);
                    trackTree.appendChild(stepTree, nullptr);
                }
            }
            tree.appendChild(trackTree, nullptr);
        }
        return tree;
    }
    
    void fromValueTree(const juce::ValueTree& tree) {
        numSteps_ = tree.getProperty("numSteps");
        numTracks_ = tree.getProperty("numTracks");
        steps_.resize(numTracks_, std::vector<Step>(numSteps_));
        
        for (const auto& trackTree : tree) {
            int trackIdx = trackTree.getProperty("index");
            for (const auto& stepTree : trackTree) {
                int stepIdx = stepTree.getProperty("index");
                auto& step = steps_[trackIdx][stepIdx];
                step.active = true;
                step.velocity = (uint8_t)(int)stepTree.getProperty("velocity");
                step.probability = (float)(double)stepTree.getProperty("probability");
                step.ratcheting = (int)stepTree.getProperty("ratcheting");
                step.slide = (bool)stepTree.getProperty("slide");
                step.accent = (bool)stepTree.getProperty("accent");
                step.microTiming = (int8_t)(int)stepTree.getProperty("microTiming");
                step.noteNumber = (uint8_t)(int)stepTree.getProperty("noteNumber");
            }
        }
    }
    
private:
    std::vector<std::vector<Step>> steps_;
    int numSteps_;
    int numTracks_;
};

/**
 * @class StepSequencerEngine
 * @brief Motor del step sequencer con swing, humanización y playback
 */
class StepSequencerEngine {
public:
    StepSequencerEngine() = default;
    
    struct Config {
        double sampleRate { 44100.0 };
        int subdivision { 16 };        // 16th notes
        double tempo { 120.0 };
        int midiChannel { 1 };
    };
    
    void prepare(const Config& config) {
        config_ = config;
        calculateStepLength();
        reset();
    }
    
    void setPattern(std::shared_ptr<StepPattern> pattern) {
        pattern_ = pattern;
    }
    
    void setTempo(double tempo) {
        config_.tempo = tempo;
        calculateStepLength();
    }
    
    void setSwing(float swing) {
        swing_ = juce::jlimit(0.0f, 1.0f, swing);
    }
    
    void setHumanize(float amount) {
        humanize_ = juce::jlimit(0.0f, 1.0f, amount);
    }
    
    void setGate(float gate) {
        gate_ = juce::jlimit(0.0f, 2.0f, gate);
    }
    
    void reset() {
        currentStep_ = 0;
        samplePosition_ = 0;
        isPlaying_ = false;
    }
    
    void start() {
        isPlaying_ = true;
        currentStep_ = 0;
    }
    
    void stop() {
        isPlaying_ = false;
        sendAllNotesOff();
    }
    
    // Process audio block y generar MIDI events
    void process(juce::MidiBuffer& midiMessages, int numSamples) {
        if (!isPlaying_ || !pattern_) return;
        
        midiMessages.clear();
        
        for (int sample = 0; sample < numSamples; ++sample) {
            // Check if we hit a step
            if (samplePosition_ >= stepLengthSamples_) {
                triggerCurrentStep(midiMessages, sample);
                advanceStep();
                samplePosition_ = 0;
            }
            
            samplePosition_++;
        }
        
        // Note offs
        processNoteOffs(midiMessages, numSamples);
    }
    
    int getCurrentStep() const { return currentStep_; }
    bool isPlaying() const { return isPlaying_; }
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void stepChanged(int newStep) = 0;
    };
    
    void addListener(Listener* listener) {
        listeners_.add(listener);
    }
    
    void removeListener(Listener* listener) {
        listeners_.remove(listener);
    }
    
private:
    void calculateStepLength() {
        // Samples per quarter note
        double samplesPerQuarter = (60.0 / config_.tempo) * config_.sampleRate;
        // Samples per 16th note
        stepLengthSamples_ = samplesPerQuarter / 4.0;
    }
    
    void advanceStep() {
        currentStep_ = (currentStep_ + 1) % pattern_->getNumSteps();
        listeners_.call([this](Listener& l) { l.stepChanged(currentStep_); });
    }
    
    void triggerCurrentStep(juce::MidiBuffer& buffer, int sampleOffset) {
        for (int track = 0; track < pattern_->getNumTracks(); ++track) {
            const auto& step = pattern_->getStep(track, currentStep_);
            
            if (step.shouldTrigger()) {
                triggerStep(buffer, track, step, sampleOffset);
            }
        }
    }
    
    void triggerStep(juce::MidiBuffer& buffer, int track, const Step& step, int offset) {
        // Apply swing
        int swingOffset = 0;
        if (currentStep_ % 2 == 1) {  // Odd steps get swing
            swingOffset = swing_ * stepLengthSamples_ * 0.5;
        }
        
        // Apply humanize
        int humanizeOffset = 0;
        if (humanize_ > 0.0f) {
            auto& rand = juce::Random::getSystemRandom();
            humanizeOffset = rand.nextInt(juce::Range<int>(
                -humanize_ * stepLengthSamples_ * 0.1,
                 humanize_ * stepLengthSamples_ * 0.1
            ));
        }
        
        // Apply micro-timing
        int microOffset = step.microTiming;
        
        int finalOffset = offset + swingOffset + humanizeOffset + microOffset;
        finalOffset = std::max(0, finalOffset);
        
        // Note On
        uint8_t velocity = step.getVelocity();
        buffer.addEvent(juce::MidiMessage::noteOn(
            config_.midiChannel, 
            step.noteNumber, 
            velocity
        ), finalOffset);
        
        // Schedule Note Off
        int noteLength = stepLengthSamples_ * gate_;
        if (step.ratcheting > 1) {
            // Ratcheting: trigger múltiples veces
            int ratchetLength = stepLengthSamples_ / step.ratcheting;
            for (int r = 0; r < step.ratcheting; ++r) {
                int ratchetOffset = finalOffset + r * ratchetLength;
                buffer.addEvent(juce::MidiMessage::noteOn(
                    config_.midiChannel,
                    step.noteNumber,
                    velocity * (1.0f - r * 0.1f)  // Decay velocity
                ), ratchetOffset);
                
                scheduleNoteOff(step.noteNumber, ratchetOffset + ratchetLength * gate_);
            }
        } else {
            scheduleNoteOff(step.noteNumber, finalOffset + noteLength);
        }
    }
    
    void scheduleNoteOff(uint8_t noteNumber, int timeStamp) {
        activeNotes_.push_back({noteNumber, timeStamp});
    }
    
    void processNoteOffs(juce::MidiBuffer& buffer, int numSamples) {
        for (auto it = activeNotes_.begin(); it != activeNotes_.end();) {
            it->timeRemaining -= numSamples;
            if (it->timeRemaining <= 0) {
                buffer.addEvent(juce::MidiMessage::noteOff(
                    config_.midiChannel,
                    it->noteNumber
                ), 0);
                it = activeNotes_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void sendAllNotesOff() {
        for (const auto& note : activeNotes_) {
            // Send note off immediately
        }
        activeNotes_.clear();
    }
    
    struct ActiveNote {
        uint8_t noteNumber;
        int timeRemaining;
    };
    
    Config config_;
    std::shared_ptr<StepPattern> pattern_;
    
    int currentStep_ { 0 };
    int64_t samplePosition_ { 0 };
    double stepLengthSamples_ { 0.0 };
    bool isPlaying_ { false };
    
    float swing_ { 0.0f };
    float humanize_ { 0.0f };
    float gate_ { 0.8f };
    
    std::vector<ActiveNote> activeNotes_;
    juce::ListenerList<Listener> listeners_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencerEngine)
};

/**
 * @class StepSequencerComponent
 * @brief UI Component para el step sequencer (16-pad grid)
 */
class StepSequencerComponent : public juce::Component,
                               public StepSequencerEngine::Listener {
public:
    StepSequencerComponent(StepSequencerEngine& engine)
        : engine_(engine) {
        
        pattern_ = std::make_shared<StepPattern>(16, 8);
        engine_.setPattern(pattern_);
        engine_.addListener(this);
        
        setSize(800, 400);
    }
    
    ~StepSequencerComponent() override {
        engine_.removeListener(this);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        drawGrid(g);
        drawSteps(g);
        drawCurrentStepIndicator(g);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        auto pos = e.getPosition();
        auto [track, step] = getGridPosition(pos);
        
        if (track >= 0 && step >= 0) {
            if (e.mods.isRightButtonDown()) {
                // Right click: open step editor
                showStepEditor(track, step);
            } else {
                // Left click: toggle step
                pattern_->toggleStep(track, step);
                repaint();
            }
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        auto pos = e.getPosition();
        auto [track, step] = getGridPosition(pos);
        
        if (track >= 0 && step >= 0) {
            auto& s = pattern_->getStep(track, step);
            if (!s.active) {
                s.active = true;
                repaint();
            }
        }
    }
    
    void stepChanged(int newStep) override {
        currentStep_ = newStep;
        repaint();
    }
    
private:
    void drawGrid(juce::Graphics& g) {
        g.setColour(juce::Colour(0xff2a2a2a));
        
        int numSteps = pattern_->getNumSteps();
        int numTracks = pattern_->getNumTracks();
        
        float cellWidth = getWidth() / (float)numSteps;
        float cellHeight = getHeight() / (float)numTracks;
        
        // Vertical lines
        for (int i = 0; i <= numSteps; i++) {
            float x = i * cellWidth;
            float thickness = (i % 4 == 0) ? 2.0f : 1.0f;
            g.drawLine(x, 0, x, getHeight(), thickness);
        }
        
        // Horizontal lines
        for (int i = 0; i <= numTracks; i++) {
            float y = i * cellHeight;
            g.drawLine(0, y, getWidth(), y, 1.0f);
        }
    }
    
    void drawSteps(juce::Graphics& g) {
        int numSteps = pattern_->getNumSteps();
        int numTracks = pattern_->getNumTracks();
        
        float cellWidth = getWidth() / (float)numSteps;
        float cellHeight = getHeight() / (float)numTracks;
        
        for (int t = 0; t < numTracks; t++) {
            for (int s = 0; s < numSteps; s++) {
                const auto& step = pattern_->getStep(t, s);
                
                if (step.active) {
                    float x = s * cellWidth + 2;
                    float y = t * cellHeight + 2;
                    float w = cellWidth - 4;
                    float h = cellHeight - 4;
                    
                    // Color based on velocity
                    float brightness = step.velocity / 127.0f;
                    juce::Colour col = juce::Colour::fromHSV(0.55f, 0.7f, brightness, 1.0f);
                    
                    if (step.accent) {
                        col = juce::Colours::orange;
                    }
                    
                    g.setColour(col);
                    g.fillRoundedRectangle(x, y, w, h, 4.0f);
                    
                    // Probability indicator
                    if (step.probability < 1.0f) {
                        g.setColour(juce::Colours::white.withAlpha(0.3f));
                        g.drawText(juce::String((int)(step.probability * 100)) + "%",
                                  x, y, w, h, juce::Justification::centred);
                    }
                    
                    // Ratcheting indicator
                    if (step.ratcheting > 1) {
                        g.setColour(juce::Colours::white);
                        g.drawText("x" + juce::String(step.ratcheting),
                                  x, y + h - 15, w, 12, juce::Justification::centred);
                    }
                }
            }
        }
    }
    
    void drawCurrentStepIndicator(juce::Graphics& g) {
        if (!engine_.isPlaying()) return;
        
        float cellWidth = getWidth() / (float)pattern_->getNumSteps();
        float x = currentStep_ * cellWidth;
        
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.fillRect(x, 0.0f, cellWidth, (float)getHeight());
    }
    
    std::pair<int, int> getGridPosition(juce::Point<int> pos) {
        int numSteps = pattern_->getNumSteps();
        int numTracks = pattern_->getNumTracks();
        
        float cellWidth = getWidth() / (float)numSteps;
        float cellHeight = getHeight() / (float)numTracks;
        
        int step = pos.x / cellWidth;
        int track = pos.y / cellHeight;
        
        if (step >= 0 && step < numSteps && track >= 0 && track < numTracks) {
            return {track, step};
        }
        return {-1, -1};
    }
    
    void showStepEditor(int track, int step) {
        // TODO: Open popup editor para ajustar velocity, probability, etc.
    }
    
    StepSequencerEngine& engine_;
    std::shared_ptr<StepPattern> pattern_;
    int currentStep_ { 0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencerComponent)
};

} // namespace Sequencer
} // namespace OmegaStudio
