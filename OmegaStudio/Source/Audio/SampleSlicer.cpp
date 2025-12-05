#include "SampleSlicer.h"
#include <algorithm>
#include <cmath>

namespace omega {

//==============================================================================
// SampleSlicer Implementation
//==============================================================================

SampleSlicer::SampleSlicer()
    : sampleRate_(44100.0)
    , transientSensitivity_(0.5f)
    , transientThreshold_(0.1f)
{
}

SampleSlicer::~SampleSlicer()
{
}

void SampleSlicer::loadAudioFile(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (!reader)
        return;
    
    sampleRate_ = reader->sampleRate;
    
    audioBuffer_ = std::make_unique<juce::AudioBuffer<float>>(
        static_cast<int>(reader->numChannels),
        static_cast<int>(reader->lengthInSamples)
    );
    
    reader->read(audioBuffer_.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    
    clearSlices();
}

void SampleSlicer::loadAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    audioBuffer_ = std::make_unique<juce::AudioBuffer<float>>(buffer);
    sampleRate_ = sampleRate;
    clearSlices();
}

void SampleSlicer::detectTransients(float sensitivity, float threshold)
{
    transientSensitivity_ = sensitivity;
    transientThreshold_ = threshold;
    
    if (!audioBuffer_ || audioBuffer_->getNumSamples() == 0)
        return;
    
    // Calculate envelope from audio
    const float* data = audioBuffer_->getReadPointer(0);
    int numSamples = audioBuffer_->getNumSamples();
    
    auto envelope = calculateEnvelope(data, numSamples);
    
    // Find peaks in envelope
    auto peaks = findPeaks(envelope, threshold);
    
    // Clear existing slices
    clearSlices();
    
    // Create slices from peaks
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        Slice slice;
        slice.startSample = peaks[i];
        slice.endSample = (i + 1 < peaks.size()) ? peaks[i + 1] : numSamples;
        slice.transientStrength = calculateTransientStrength(peaks[i], envelope);
        slice.name = juce::String("Slice ") + juce::String(i + 1);
        
        slices_.push_back(slice);
    }
}

std::vector<float> SampleSlicer::calculateEnvelope(const float* data, int numSamples)
{
    std::vector<float> envelope;
    envelope.reserve(numSamples);
    
    // Window size for envelope following (about 10ms)
    int windowSize = static_cast<int>(sampleRate_ * 0.01);
    
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        // Add new sample
        float absValue = std::abs(data[i]);
        sum += absValue;
        
        // Remove old sample
        if (i >= windowSize)
            sum -= std::abs(data[i - windowSize]);
        
        // Average
        float avg = sum / static_cast<float>(windowSize);
        envelope.push_back(avg);
    }
    
    return envelope;
}

std::vector<int> SampleSlicer::findPeaks(const std::vector<float>& envelope, float threshold)
{
    std::vector<int> peaks;
    
    // Find local maxima above threshold
    int lookAhead = static_cast<int>(sampleRate_ * 0.02 * transientSensitivity_);  // 20ms window scaled by sensitivity
    
    for (size_t i = lookAhead; i < envelope.size() - lookAhead; ++i)
    {
        bool isPeak = true;
        float currentValue = envelope[i];
        
        // Check if it's a local maximum
        for (int j = -lookAhead; j <= lookAhead; ++j)
        {
            if (j != 0 && envelope[i + j] >= currentValue)
            {
                isPeak = false;
                break;
            }
        }
        
        // Check if above threshold
        if (isPeak && currentValue > threshold)
        {
            peaks.push_back(static_cast<int>(i));
            
            // Skip ahead to avoid multiple peaks in same area
            i += lookAhead;
        }
    }
    
    return peaks;
}

float SampleSlicer::calculateTransientStrength(int position, const std::vector<float>& envelope)
{
    if (position >= static_cast<int>(envelope.size()))
        return 0.0f;
    
    // Calculate strength as ratio of peak to surrounding average
    int windowSize = static_cast<int>(sampleRate_ * 0.05);  // 50ms window
    
    float peakValue = envelope[position];
    
    // Calculate average before peak
    float avgBefore = 0.0f;
    int count = 0;
    for (int i = std::max(0, position - windowSize); i < position; ++i)
    {
        avgBefore += envelope[i];
        ++count;
    }
    if (count > 0) avgBefore /= count;
    
    // Strength is ratio
    float strength = (avgBefore > 0.0f) ? (peakValue / avgBefore) : 1.0f;
    
    return juce::jlimit(0.0f, 1.0f, strength / 10.0f);  // Normalize to 0-1
}

void SampleSlicer::setTransientSensitivity(float sensitivity)
{
    transientSensitivity_ = juce::jlimit(0.0f, 1.0f, sensitivity);
}

void SampleSlicer::setTransientThreshold(float threshold)
{
    transientThreshold_ = juce::jlimit(0.0f, 1.0f, threshold);
}

void SampleSlicer::addSlice(int position)
{
    Slice slice;
    slice.startSample = position;
    slice.name = juce::String("Slice ") + juce::String(slices_.size() + 1);
    
    // Find insertion point
    auto it = std::lower_bound(slices_.begin(), slices_.end(), slice,
        [](const Slice& a, const Slice& b) { return a.startSample < b.startSample; });
    
    slices_.insert(it, slice);
    
    // Update end samples
    for (size_t i = 0; i < slices_.size(); ++i)
    {
        if (i + 1 < slices_.size())
            slices_[i].endSample = slices_[i + 1].startSample;
        else
            slices_[i].endSample = audioBuffer_ ? audioBuffer_->getNumSamples() : 0;
    }
}

void SampleSlicer::removeSlice(int index)
{
    if (index >= 0 && index < static_cast<int>(slices_.size()))
    {
        slices_.erase(slices_.begin() + index);
        
        // Update end samples
        for (size_t i = 0; i < slices_.size(); ++i)
        {
            if (i + 1 < slices_.size())
                slices_[i].endSample = slices_[i + 1].startSample;
            else
                slices_[i].endSample = audioBuffer_ ? audioBuffer_->getNumSamples() : 0;
        }
    }
}

void SampleSlicer::moveSlice(int index, int newPosition)
{
    if (index >= 0 && index < static_cast<int>(slices_.size()))
    {
        slices_[index].startSample = newPosition;
        
        // Re-sort slices
        std::sort(slices_.begin(), slices_.end(),
            [](const Slice& a, const Slice& b) { return a.startSample < b.startSample; });
        
        // Update end samples
        for (size_t i = 0; i < slices_.size(); ++i)
        {
            if (i + 1 < slices_.size())
                slices_[i].endSample = slices_[i + 1].startSample;
            else
                slices_[i].endSample = audioBuffer_ ? audioBuffer_->getNumSamples() : 0;
        }
    }
}

void SampleSlicer::clearSlices()
{
    slices_.clear();
}

void SampleSlicer::autoSliceByTransients(int minSliceLength)
{
    detectTransients(transientSensitivity_, transientThreshold_);
    
    // Remove slices that are too short
    slices_.erase(std::remove_if(slices_.begin(), slices_.end(),
        [minSliceLength](const Slice& slice) { return slice.getLength() < minSliceLength; }),
        slices_.end());
}

void SampleSlicer::autoSliceByGrid(int numSlices)
{
    if (!audioBuffer_ || numSlices <= 0)
        return;
    
    clearSlices();
    
    int totalSamples = audioBuffer_->getNumSamples();
    int samplesPerSlice = totalSamples / numSlices;
    
    for (int i = 0; i < numSlices; ++i)
    {
        Slice slice;
        slice.startSample = i * samplesPerSlice;
        slice.endSample = (i + 1) * samplesPerSlice;
        slice.name = juce::String("Slice ") + juce::String(i + 1);
        
        slices_.push_back(slice);
    }
    
    // Ensure last slice goes to end
    if (!slices_.empty())
        slices_.back().endSample = totalSamples;
}

void SampleSlicer::autoSliceByBeats(double bpm, int beatsPerSlice)
{
    if (!audioBuffer_ || bpm <= 0.0)
        return;
    
    clearSlices();
    
    double samplesPerBeat = (60.0 / bpm) * sampleRate_;
    int samplesPerSlice = static_cast<int>(samplesPerBeat * beatsPerSlice);
    
    int totalSamples = audioBuffer_->getNumSamples();
    int position = 0;
    int sliceNum = 0;
    
    while (position < totalSamples)
    {
        Slice slice;
        slice.startSample = position;
        slice.endSample = std::min(position + samplesPerSlice, totalSamples);
        slice.name = juce::String("Slice ") + juce::String(sliceNum + 1);
        
        slices_.push_back(slice);
        
        position += samplesPerSlice;
        ++sliceNum;
    }
}

const SampleSlicer::Slice& SampleSlicer::getSlice(int index) const
{
    static Slice emptySlice;
    if (index >= 0 && index < static_cast<int>(slices_.size()))
        return slices_[index];
    return emptySlice;
}

SampleSlicer::Slice& SampleSlicer::getSlice(int index)
{
    static Slice emptySlice;
    if (index >= 0 && index < static_cast<int>(slices_.size()))
        return slices_[index];
    return emptySlice;
}

void SampleSlicer::autoMapToMidiNotes(int startNote)
{
    for (size_t i = 0; i < slices_.size(); ++i)
    {
        slices_[i].midiNote = startNote + static_cast<int>(i);
    }
}

void SampleSlicer::setSliceMidiNote(int sliceIndex, int midiNote)
{
    if (sliceIndex >= 0 && sliceIndex < static_cast<int>(slices_.size()))
        slices_[sliceIndex].midiNote = midiNote;
}

int SampleSlicer::getMidiNoteForSlice(int sliceIndex) const
{
    if (sliceIndex >= 0 && sliceIndex < static_cast<int>(slices_.size()))
        return slices_[sliceIndex].midiNote;
    return 60;
}

void SampleSlicer::renderSlice(int sliceIndex, juce::AudioBuffer<float>& outputBuffer,
                                int startSample, float gain)
{
    if (!audioBuffer_ || sliceIndex < 0 || sliceIndex >= static_cast<int>(slices_.size()))
        return;
    
    const auto& slice = slices_[sliceIndex];
    int sliceLength = slice.getLength();
    
    // Ensure we don't write past output buffer
    int samplesToWrite = std::min(sliceLength, outputBuffer.getNumSamples() - startSample);
    
    for (int ch = 0; ch < std::min(audioBuffer_->getNumChannels(), outputBuffer.getNumChannels()); ++ch)
    {
        outputBuffer.copyFrom(ch, startSample,
                             *audioBuffer_, ch, slice.startSample,
                             samplesToWrite);
        
        if (gain != 1.0f)
            outputBuffer.applyGain(ch, startSample, samplesToWrite, gain);
    }
}

void SampleSlicer::renderSliceWithTimeStretch(int sliceIndex, juce::AudioBuffer<float>& outputBuffer,
                                               int startSample, float stretchFactor, float gain)
{
    if (!audioBuffer_ || sliceIndex < 0 || sliceIndex >= static_cast<int>(slices_.size()))
        return;
    
    const auto& slice = slices_[sliceIndex];
    
    // Create temp buffer for slice
    int sliceLength = slice.getLength();
    juce::AudioBuffer<float> sliceBuffer(audioBuffer_->getNumChannels(), sliceLength);
    
    for (int ch = 0; ch < audioBuffer_->getNumChannels(); ++ch)
    {
        sliceBuffer.copyFrom(ch, 0, *audioBuffer_, ch, slice.startSample, sliceLength);
    }
    
    // Create stretched buffer
    int stretchedLength = static_cast<int>(sliceLength * stretchFactor);
    juce::AudioBuffer<float> stretchedBuffer(audioBuffer_->getNumChannels(), stretchedLength);
    
    timeStretchSlice(sliceBuffer, stretchedBuffer, stretchFactor);
    
    // Copy to output
    int samplesToWrite = std::min(stretchedLength, outputBuffer.getNumSamples() - startSample);
    for (int ch = 0; ch < std::min(stretchedBuffer.getNumChannels(), outputBuffer.getNumChannels()); ++ch)
    {
        outputBuffer.copyFrom(ch, startSample, stretchedBuffer, ch, 0, samplesToWrite);
        
        if (gain != 1.0f)
            outputBuffer.applyGain(ch, startSample, samplesToWrite, gain);
    }
}

void SampleSlicer::timeStretchSlice(const juce::AudioBuffer<float>& input,
                                    juce::AudioBuffer<float>& output,
                                    float stretchFactor)
{
    // Simple linear interpolation time-stretch (replace with better algorithm for production)
    int inputLength = input.getNumSamples();
    int outputLength = output.getNumSamples();
    
    for (int ch = 0; ch < input.getNumChannels(); ++ch)
    {
        const float* inputData = input.getReadPointer(ch);
        float* outputData = output.getWritePointer(ch);
        
        for (int i = 0; i < outputLength; ++i)
        {
            float sourcePos = i / stretchFactor;
            int index1 = static_cast<int>(sourcePos);
            int index2 = std::min(index1 + 1, inputLength - 1);
            float frac = sourcePos - index1;
            
            if (index1 < inputLength)
                outputData[i] = inputData[index1] * (1.0f - frac) + inputData[index2] * frac;
            else
                outputData[i] = 0.0f;
        }
    }
}

void SampleSlicer::exportSlice(int sliceIndex, const juce::File& outputFile)
{
    if (!audioBuffer_ || sliceIndex < 0 || sliceIndex >= static_cast<int>(slices_.size()))
        return;
    
    const auto& slice = slices_[sliceIndex];
    int sliceLength = slice.getLength();
    
    juce::AudioBuffer<float> sliceBuffer(audioBuffer_->getNumChannels(), sliceLength);
    
    for (int ch = 0; ch < audioBuffer_->getNumChannels(); ++ch)
    {
        sliceBuffer.copyFrom(ch, 0, *audioBuffer_, ch, slice.startSample, sliceLength);
    }
    
    // Write to file
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset(wavFormat.createWriterFor(new juce::FileOutputStream(outputFile),
                                           sampleRate_,
                                           audioBuffer_->getNumChannels(),
                                           24,
                                           {},
                                           0));
    
    if (writer)
        writer->writeFromAudioSampleBuffer(sliceBuffer, 0, sliceLength);
}

void SampleSlicer::exportAllSlices(const juce::File& outputDirectory, const juce::String& prefix)
{
    outputDirectory.createDirectory();
    
    for (size_t i = 0; i < slices_.size(); ++i)
    {
        juce::String filename = prefix + juce::String("_") + juce::String(i + 1) + juce::String(".wav");
        juce::File outputFile = outputDirectory.getChildFile(filename);
        
        exportSlice(static_cast<int>(i), outputFile);
    }
}

//==============================================================================
// SampleSlicerComponent Implementation
//==============================================================================

SampleSlicerComponent::SampleSlicerComponent(SampleSlicer& slicer)
    : slicer_(slicer)
    , zoom_(1.0f)
    , scrollPosition_(0.0f)
    , showTransients_(true)
    , showSliceMarkers_(true)
    , selectedSlice_(-1)
    , isDraggingSlice_(false)
    , draggedSlice_(-1)
    , thumbnailCache_(5)
{
    // Thumbnail will be initialized when needed
    startTimerHz(30);
}

SampleSlicerComponent::~SampleSlicerComponent()
{
}

void SampleSlicerComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A1A));
    
    drawWaveform(g);
    
    if (showSliceMarkers_)
        drawSliceMarkers(g);
    
    if (showTransients_)
        drawTransients(g);
}

void SampleSlicerComponent::resized()
{
}

void SampleSlicerComponent::drawWaveform(juce::Graphics& g)
{
    const auto* buffer = slicer_.getAudioBuffer();
    if (!buffer || buffer->getNumSamples() == 0)
        return;
    
    g.setColour(juce::Colours::cyan);
    
    // Simple waveform drawing
    int numSamples = buffer->getNumSamples();
    const float* data = buffer->getReadPointer(0);
    
    int startSample = static_cast<int>(scrollPosition_ * numSamples);
    int samplesVisible = static_cast<int>(numSamples / zoom_);
    
    juce::Path waveformPath;
    
    for (int x = 0; x < getWidth(); ++x)
    {
        int sampleIndex = startSample + (x * samplesVisible / getWidth());
        if (sampleIndex >= numSamples) break;
        
        float sampleValue = data[sampleIndex];
        int y = getHeight() / 2 - static_cast<int>(sampleValue * getHeight() / 2);
        
        if (x == 0)
            waveformPath.startNewSubPath(static_cast<float>(x), static_cast<float>(y));
        else
            waveformPath.lineTo(static_cast<float>(x), static_cast<float>(y));
    }
    
    g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
}

void SampleSlicerComponent::drawSliceMarkers(juce::Graphics& g)
{
    int numSlices = slicer_.getNumSlices();
    
    for (int i = 0; i < numSlices; ++i)
    {
        const auto& slice = slicer_.getSlice(i);
        int x = sampleToX(slice.startSample);
        
        if (i == selectedSlice_)
            g.setColour(juce::Colours::yellow);
        else
            g.setColour(juce::Colours::orange);
        
        g.drawLine(static_cast<float>(x), 0.0f, static_cast<float>(x), static_cast<float>(getHeight()), 2.0f);
    }
}

void SampleSlicerComponent::drawTransients(juce::Graphics& /* g */)
{
    // TODO: Draw transient markers
}

int SampleSlicerComponent::sampleToX(int sample) const
{
    int numSamples = slicer_.getTotalSamples();
    if (numSamples == 0) return 0;
    
    int startSample = static_cast<int>(scrollPosition_ * numSamples);
    int samplesVisible = static_cast<int>(numSamples / zoom_);
    
    return static_cast<int>((sample - startSample) * getWidth() / static_cast<float>(samplesVisible));
}

int SampleSlicerComponent::xToSample(int x) const
{
    int numSamples = slicer_.getTotalSamples();
    int startSample = static_cast<int>(scrollPosition_ * numSamples);
    int samplesVisible = static_cast<int>(numSamples / zoom_);
    
    return startSample + (x * samplesVisible / getWidth());
}

void SampleSlicerComponent::mouseDown(const juce::MouseEvent& event)
{
    int sample = xToSample(event.x);
    
    // Check if clicking near existing slice
    int numSlices = slicer_.getNumSlices();
    for (int i = 0; i < numSlices; ++i)
    {
        const auto& slice = slicer_.getSlice(i);
        int x = sampleToX(slice.startSample);
        
        if (std::abs(event.x - x) < 5)
        {
            selectedSlice_ = i;
            isDraggingSlice_ = true;
            draggedSlice_ = i;
            repaint();
            return;
        }
    }
    
    // Add new slice
    slicer_.addSlice(sample);
    repaint();
}

void SampleSlicerComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDraggingSlice_ && draggedSlice_ >= 0)
    {
        int newSample = xToSample(event.x);
        slicer_.moveSlice(draggedSlice_, newSample);
        repaint();
    }
}

void SampleSlicerComponent::mouseUp(const juce::MouseEvent& /* event */)
{
    isDraggingSlice_ = false;
    draggedSlice_ = -1;
}

void SampleSlicerComponent::setZoom(float zoom)
{
    zoom_ = juce::jlimit(1.0f, 100.0f, zoom);
    repaint();
}

void SampleSlicerComponent::setScrollPosition(float position)
{
    scrollPosition_ = juce::jlimit(0.0f, 1.0f, position);
    repaint();
}

void SampleSlicerComponent::setShowTransients(bool show)
{
    showTransients_ = show;
    repaint();
}

void SampleSlicerComponent::setShowSliceMarkers(bool show)
{
    showSliceMarkers_ = show;
    repaint();
}

void SampleSlicerComponent::timerCallback()
{
    repaint();
}

} // namespace omega
