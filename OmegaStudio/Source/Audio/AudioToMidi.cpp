#include "AudioToMidi.h"
#include <algorithm>
#include <cmath>

namespace omega {

//==============================================================================
// AudioToMidi Implementation
//==============================================================================

AudioToMidi::AudioToMidi()
    : sampleRate_(44100.0)
    , isMonophonic_(true)
    , minNoteLength_(0.05)
    , onsetThreshold_(0.3f)
    , pitchQuantization_(true)
    , vibratoTolerance_(30.0f)
    , minPitchHz_(80.0f)
    , maxPitchHz_(1200.0f)
    , maxPolyphony_(1)
    , fft_(11)  // 2048-point FFT
    , fftSize_(2048)
    , noteIsActive_(false)
{
    fftData_.resize(fftSize_ * 2, 0.0f);
    
    // Hanning window
    window_.resize(fftSize_);
    for (int i = 0; i < fftSize_; ++i)
    {
        window_[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize_ - 1)));
    }
}

AudioToMidi::~AudioToMidi()
{
}

void AudioToMidi::initialize(double sampleRate)
{
    sampleRate_ = sampleRate;
}

void AudioToMidi::setSampleRate(double sampleRate)
{
    sampleRate_ = sampleRate;
}

void AudioToMidi::setMonophonic(bool mono)
{
    isMonophonic_ = mono;
    maxPolyphony_ = mono ? 1 : 4;
}

void AudioToMidi::setMinNoteLength(double seconds)
{
    minNoteLength_ = juce::jlimit(0.01, 5.0, seconds);
}

void AudioToMidi::setOnsetThreshold(float threshold)
{
    onsetThreshold_ = juce::jlimit(0.0f, 1.0f, threshold);
}

void AudioToMidi::setPitchQuantization(bool enabled)
{
    pitchQuantization_ = enabled;
}

void AudioToMidi::setVibratoTolerance(float cents)
{
    vibratoTolerance_ = juce::jlimit(0.0f, 100.0f, cents);
}

void AudioToMidi::setMinPitchHz(float hz)
{
    minPitchHz_ = juce::jlimit(20.0f, 2000.0f, hz);
}

void AudioToMidi::setMaxPitchHz(float hz)
{
    maxPitchHz_ = juce::jlimit(100.0f, 5000.0f, hz);
}

void AudioToMidi::setPolyphonyLevel(int maxNotes)
{
    maxPolyphony_ = juce::jlimit(1, 8, maxNotes);
}

void AudioToMidi::analyzeAudio(const juce::AudioBuffer<float>& audioBuffer)
{
    detectedNotes_.clear();
    pitchFrames_.clear();
    
    // Detect onsets first
    detectOnsets(audioBuffer);
    
    // Analyze pitch over time
    const float* audioData = audioBuffer.getReadPointer(0);
    int numSamples = audioBuffer.getNumSamples();
    
    int hopSize = fftSize_ / 4;  // 75% overlap
    int numFrames = (numSamples - fftSize_) / hopSize;
    
    for (int frame = 0; frame < numFrames; ++frame)
    {
        int startSample = frame * hopSize;
        
        float frequency = 0.0f;
        float confidence = 0.0f;
        detectPitch(audioData + startSample, fftSize_, frequency, confidence);
        
        if (frequency >= minPitchHz_ && frequency <= maxPitchHz_ && confidence > 0.3f)
        {
            PitchFrame pf;
            pf.time = startSample / sampleRate_;
            pf.frequency = frequency;
            pf.confidence = confidence;
            pitchFrames_.push_back(pf);
        }
    }
    
    // Segment into notes
    segmentNotes();
    
    // Merge adjacent similar notes
    mergeNotes();
    
    // Detect rhythm
    rhythmInfo_.onsetTimes = onsetTimes_;
    // TODO: Implement tempo estimation
    rhythmInfo_.estimatedTempo = 120.0;
    rhythmInfo_.timeSignatureNum = 4;
    rhythmInfo_.timeSignatureDenom = 4;
}

void AudioToMidi::detectPitch(const float* audioData, int numSamples, float& frequency, float& confidence)
{
    // Use YIN algorithm for pitch detection
    float yinFreq = yin(audioData, numSamples);
    
    if (yinFreq > 0.0f)
    {
        frequency = yinFreq;
        confidence = 0.8f;  // TODO: Calculate actual confidence from YIN
    }
    else
    {
        frequency = 0.0f;
        confidence = 0.0f;
    }
}

float AudioToMidi::yin(const float* data, int length)
{
    std::vector<float> difference(length / 2);
    
    // Step 1: Calculate difference function
    for (int tau = 0; tau < length / 2; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < length / 2; ++j)
        {
            float delta = data[j] - data[j + tau];
            sum += delta * delta;
        }
        difference[tau] = sum;
    }
    
    // Step 2: Cumulative mean normalized difference
    difference[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau < length / 2; ++tau)
    {
        runningSum += difference[tau];
        difference[tau] *= tau / runningSum;
    }
    
    // Step 3: Find first minimum below threshold
    float threshold = 0.1f;
    for (int tau = 2; tau < length / 2; ++tau)
    {
        if (difference[tau] < threshold)
        {
            // Check if it's a local minimum
            if (tau + 1 < length / 2 && difference[tau] < difference[tau + 1])
            {
                // Parabolic interpolation for sub-sample accuracy
                float betterTau = tau;
                if (tau > 0 && tau < length / 2 - 1)
                {
                    float s0 = difference[tau - 1];
                    float s1 = difference[tau];
                    float s2 = difference[tau + 1];
                    betterTau = tau + (s2 - s0) / (2.0f * (2.0f * s1 - s2 - s0));
                }
                
                float frequency = static_cast<float>(sampleRate_) / betterTau;
                return frequency;
            }
        }
    }
    
    return 0.0f;  // No pitch detected
}

float AudioToMidi::autocorrelation(const float* data, int length, int lag)
{
    float sum = 0.0f;
    for (int i = 0; i < length - lag; ++i)
    {
        sum += data[i] * data[i + lag];
    }
    return sum;
}

void AudioToMidi::detectOnsets(const juce::AudioBuffer<float>& audioBuffer)
{
    onsetTimes_.clear();
    spectralFlux_.clear();
    
    const float* audioData = audioBuffer.getReadPointer(0);
    int numSamples = audioBuffer.getNumSamples();
    
    int hopSize = fftSize_ / 4;
    int numFrames = (numSamples - fftSize_) / hopSize;
    
    std::vector<float> prevSpectrum(fftSize_ / 2, 0.0f);
    
    for (int frame = 0; frame < numFrames; ++frame)
    {
        int startSample = frame * hopSize;
        
        // Apply window and copy to FFT buffer
        for (int i = 0; i < fftSize_; ++i)
        {
            fftData_[i] = audioData[startSample + i] * window_[i];
        }
        
        // Perform FFT
        fft_.performFrequencyOnlyForwardTransform(fftData_.data());
        
        // Calculate spectral flux
        std::vector<float> currentSpectrum(fftSize_ / 2);
        for (int i = 0; i < fftSize_ / 2; ++i)
        {
            currentSpectrum[i] = fftData_[i];
        }
        
        float flux = calculateSpectralFlux(prevSpectrum, currentSpectrum);
        spectralFlux_.push_back(flux);
        
        // Detect onset if flux exceeds threshold
        if (frame > 2)
        {
            float avgFlux = (spectralFlux_[frame - 2] + spectralFlux_[frame - 1] + flux) / 3.0f;
            if (flux > avgFlux * (1.0f + onsetThreshold_ * 3.0f))
            {
                double onsetTime = startSample / sampleRate_;
                onsetTimes_.push_back(onsetTime);
            }
        }
        
        prevSpectrum = currentSpectrum;
    }
}

float AudioToMidi::calculateSpectralFlux(const std::vector<float>& spectrum1, const std::vector<float>& spectrum2)
{
    float flux = 0.0f;
    for (size_t i = 0; i < spectrum1.size(); ++i)
    {
        float diff = spectrum2[i] - spectrum1[i];
        if (diff > 0.0f)
            flux += diff;
    }
    return flux;
}

void AudioToMidi::segmentNotes()
{
    if (pitchFrames_.empty())
        return;
    
    DetectedNote currentNote;
    currentNote.startTime = pitchFrames_[0].time;
    currentNote.midiNote = quantizePitch(frequencyToMidi(pitchFrames_[0].frequency));
    currentNote.confidence = pitchFrames_[0].confidence;
    currentNote.velocity = 0.8f;
    
    for (size_t i = 1; i < pitchFrames_.size(); ++i)
    {
        int newMidi = quantizePitch(frequencyToMidi(pitchFrames_[i].frequency));
        
        // Check if pitch changed significantly (outside vibrato tolerance)
        if (newMidi != currentNote.midiNote)
        {
            // End current note
            currentNote.duration = pitchFrames_[i].time - currentNote.startTime;
            
            if (currentNote.duration >= minNoteLength_)
            {
                detectedNotes_.push_back(currentNote);
            }
            
            // Start new note
            currentNote.startTime = pitchFrames_[i].time;
            currentNote.midiNote = newMidi;
            currentNote.confidence = pitchFrames_[i].confidence;
        }
        else
        {
            // Update confidence (running average)
            currentNote.confidence = (currentNote.confidence + pitchFrames_[i].confidence) / 2.0f;
        }
    }
    
    // Add final note
    if (!pitchFrames_.empty())
    {
        currentNote.duration = pitchFrames_.back().time - currentNote.startTime;
        if (currentNote.duration >= minNoteLength_)
        {
            detectedNotes_.push_back(currentNote);
        }
    }
}

void AudioToMidi::mergeNotes()
{
    if (detectedNotes_.size() < 2)
        return;
    
    std::vector<DetectedNote> mergedNotes;
    mergedNotes.push_back(detectedNotes_[0]);
    
    for (size_t i = 1; i < detectedNotes_.size(); ++i)
    {
        auto& prev = mergedNotes.back();
        const auto& current = detectedNotes_[i];
        
        // If same pitch and very close in time, merge
        double gap = current.startTime - (prev.startTime + prev.duration);
        if (current.midiNote == prev.midiNote && gap < 0.05)
        {
            prev.duration = (current.startTime + current.duration) - prev.startTime;
            prev.confidence = (prev.confidence + current.confidence) / 2.0f;
        }
        else
        {
            mergedNotes.push_back(current);
        }
    }
    
    detectedNotes_ = mergedNotes;
}

float AudioToMidi::frequencyToMidi(float frequency)
{
    return 69.0f + 12.0f * std::log2(frequency / 440.0f);
}

int AudioToMidi::quantizePitch(float midiFloat)
{
    if (pitchQuantization_)
        return static_cast<int>(std::round(midiFloat));
    else
        return static_cast<int>(midiFloat);
}

juce::MidiMessageSequence AudioToMidi::convertToMidiSequence() const
{
    juce::MidiMessageSequence sequence;
    
    for (const auto& note : detectedNotes_)
    {
        int velocity = static_cast<int>(note.velocity * 127.0f);
        velocity = juce::jlimit(1, 127, velocity);
        
        // Note on
        juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, note.midiNote, static_cast<juce::uint8>(velocity));
        sequence.addEvent(noteOn, note.startTime);
        
        // Note off
        juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, note.midiNote);
        sequence.addEvent(noteOff, note.startTime + note.duration);
    }
    
    return sequence;
}

void AudioToMidi::exportToMidiFile(const juce::File& outputFile) const
{
    juce::MidiFile midiFile;
    auto sequence = convertToMidiSequence();
    
    midiFile.addTrack(sequence);
    midiFile.setTicksPerQuarterNote(480);
    
    juce::FileOutputStream stream(outputFile);
    if (stream.openedOk())
    {
        midiFile.writeTo(stream);
    }
}

void AudioToMidi::processBlock(const float* inputData, int numSamples)
{
    // Real-time pitch detection for live conversion
    float frequency = 0.0f;
    float confidence = 0.0f;
    
    if (numSamples >= fftSize_)
    {
        detectPitch(inputData, fftSize_, frequency, confidence);
        
        if (frequency >= minPitchHz_ && frequency <= maxPitchHz_ && confidence > 0.5f)
        {
            int midiNote = quantizePitch(frequencyToMidi(frequency));
            
            if (!noteIsActive_ || midiNote != currentNote_.midiNote)
            {
                // End previous note if active
                if (noteIsActive_)
                {
                    newNotes_.push_back(currentNote_);
                }
                
                // Start new note
                currentNote_.midiNote = midiNote;
                currentNote_.velocity = 0.8f;
                currentNote_.confidence = confidence;
                currentNote_.startTime = 0.0;  // Set by caller
                currentNote_.duration = 0.0;
                noteIsActive_ = true;
            }
        }
        else if (noteIsActive_)
        {
            // No pitch detected, end note
            newNotes_.push_back(currentNote_);
            noteIsActive_ = false;
        }
    }
}

bool AudioToMidi::hasNewNote() const
{
    return !newNotes_.empty();
}

DetectedNote AudioToMidi::getLatestNote()
{
    if (!newNotes_.empty())
    {
        auto note = newNotes_.front();
        newNotes_.erase(newNotes_.begin());
        return note;
    }
    return DetectedNote();
}

void AudioToMidi::quantizeNotes(double gridSize)
{
    for (auto& note : detectedNotes_)
    {
        // Quantize start time to grid
        double quantizedStart = std::round(note.startTime / gridSize) * gridSize;
        note.duration += (note.startTime - quantizedStart);  // Adjust duration
        note.startTime = quantizedStart;
        
        // Quantize duration to grid
        note.duration = std::round(note.duration / gridSize) * gridSize;
    }
}

void AudioToMidi::quantizeToScale(const std::vector<int>& scaleNotes)
{
    for (auto& note : detectedNotes_)
    {
        // Find closest scale note
        int closestNote = scaleNotes[0];
        int minDistance = 127;
        
        for (int scaleNote : scaleNotes)
        {
            int distance = std::abs(note.midiNote % 12 - scaleNote % 12);
            if (distance < minDistance)
            {
                minDistance = distance;
                closestNote = scaleNote;
            }
        }
        
        // Adjust to correct octave
        int octave = note.midiNote / 12;
        note.midiNote = octave * 12 + (closestNote % 12);
    }
}

//==============================================================================
// AudioToMidiComponent Implementation
//==============================================================================

AudioToMidiComponent::AudioToMidiComponent(AudioToMidi& converter)
    : converter_(converter)
    , showWaveform_(true)
    , showPitchTrack_(true)
    , showOnsets_(true)
    , showQuantized_(false)
{
    startTimerHz(30);
}

AudioToMidiComponent::~AudioToMidiComponent()
{
}

void AudioToMidiComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A1A));
    
    if (showWaveform_)
        drawWaveform(g);
    
    if (showPitchTrack_)
        drawPitchTrack(g);
    
    if (showOnsets_)
        drawOnsets(g);
    
    drawDetectedNotes(g);
}

void AudioToMidiComponent::resized()
{
}

void AudioToMidiComponent::drawWaveform(juce::Graphics& /* g */)
{
    // TODO: Implement waveform visualization
}

void AudioToMidiComponent::drawPitchTrack(juce::Graphics& /* g */)
{
    // TODO: Implement pitch track visualization
}

void AudioToMidiComponent::drawOnsets(juce::Graphics& g)
{
    const auto& onsetTimes = converter_.getRhythmInfo().onsetTimes;
    
    g.setColour(juce::Colours::orange);
    for (double time : onsetTimes)
    {
        int x = static_cast<int>(time * 100);  // Simple time-to-pixel conversion
        g.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), static_cast<float>(getHeight()), 2.0f);
    }
}

void AudioToMidiComponent::drawDetectedNotes(juce::Graphics& g)
{
    const auto& notes = converter_.getDetectedNotes();
    
    g.setColour(juce::Colours::cyan);
    
    for (const auto& note : notes)
    {
        int x = static_cast<int>(note.startTime * 100);
        int width = static_cast<int>(note.duration * 100);
        int y = getHeight() - (note.midiNote - 40) * 3;  // Simple pitch-to-y mapping
        
        g.fillRect(x, y, width, 10);
    }
}

void AudioToMidiComponent::setShowWaveform(bool show)
{
    showWaveform_ = show;
    repaint();
}

void AudioToMidiComponent::setShowPitchTrack(bool show)
{
    showPitchTrack_ = show;
    repaint();
}

void AudioToMidiComponent::setShowOnsets(bool show)
{
    showOnsets_ = show;
    repaint();
}

void AudioToMidiComponent::setShowQuantized(bool show)
{
    showQuantized_ = show;
    repaint();
}

void AudioToMidiComponent::timerCallback()
{
    if (converter_.hasNewNote())
        repaint();
}

} // namespace omega
