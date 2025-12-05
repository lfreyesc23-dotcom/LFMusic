#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace omega {

/**
 * @brief Sample slicer with transient detection and MIDI mapping
 */
class SampleSlicer {
public:
    struct Slice {
        int startSample;
        int endSample;
        float transientStrength;   // 0.0-1.0
        int midiNote;              // Mapped MIDI note
        juce::String name;
        
        Slice() : startSample(0), endSample(0), transientStrength(0.0f), midiNote(60) {}
        
        int getLength() const { return endSample - startSample; }
        float getDuration(double sampleRate) const {
            return static_cast<float>(getLength()) / static_cast<float>(sampleRate);
        }
    };
    
    SampleSlicer();
    ~SampleSlicer();
    
    // Load audio
    void loadAudioFile(const juce::File& file);
    void loadAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);
    
    // Transient detection
    void detectTransients(float sensitivity = 0.5f, float threshold = 0.1f);
    void setTransientSensitivity(float sensitivity);  // 0.0-1.0
    void setTransientThreshold(float threshold);      // 0.0-1.0
    
    // Manual slicing
    void addSlice(int position);
    void removeSlice(int index);
    void moveSlice(int index, int newPosition);
    void clearSlices();
    
    // Auto-slicing modes
    void autoSliceByTransients(int minSliceLength = 1000);
    void autoSliceByGrid(int numSlices);              // Equal divisions
    void autoSliceByBeats(double bpm, int beatsPerSlice = 1);
    
    // Slice access
    int getNumSlices() const { return static_cast<int>(slices_.size()); }
    const Slice& getSlice(int index) const;
    Slice& getSlice(int index);
    
    // MIDI mapping
    void autoMapToMidiNotes(int startNote = 60);     // C3
    void setSliceMidiNote(int sliceIndex, int midiNote);
    int getMidiNoteForSlice(int sliceIndex) const;
    
    // Playback
    void renderSlice(int sliceIndex, juce::AudioBuffer<float>& outputBuffer, 
                     int startSample, float gain = 1.0f);
    void renderSliceWithTimeStretch(int sliceIndex, juce::AudioBuffer<float>& outputBuffer,
                                    int startSample, float stretchFactor, float gain = 1.0f);
    
    // Export
    void exportSlice(int sliceIndex, const juce::File& outputFile);
    void exportAllSlices(const juce::File& outputDirectory, const juce::String& prefix);
    
    // Metadata
    double getSampleRate() const { return sampleRate_; }
    int getTotalSamples() const { return audioBuffer_ ? audioBuffer_->getNumSamples() : 0; }
    float getTotalDuration() const { return getTotalSamples() / static_cast<float>(sampleRate_); }
    
    // Audio access
    const juce::AudioBuffer<float>* getAudioBuffer() const { return audioBuffer_.get(); }
    
private:
    std::unique_ptr<juce::AudioBuffer<float>> audioBuffer_;
    double sampleRate_;
    
    std::vector<Slice> slices_;
    
    float transientSensitivity_;
    float transientThreshold_;
    
    // Transient detection helpers
    std::vector<float> calculateEnvelope(const float* data, int numSamples);
    std::vector<int> findPeaks(const std::vector<float>& envelope, float threshold);
    float calculateTransientStrength(int position, const std::vector<float>& envelope);
    
    // Time-stretching helpers
    void timeStretchSlice(const juce::AudioBuffer<float>& input, 
                         juce::AudioBuffer<float>& output,
                         float stretchFactor);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleSlicer)
};

/**
 * @brief Sample slicer GUI component
 */
class SampleSlicerComponent : public juce::Component,
                                private juce::Timer {
public:
    SampleSlicerComponent(SampleSlicer& slicer);
    ~SampleSlicerComponent() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    // Waveform display
    void setZoom(float zoom);  // 1.0 = fit all, >1.0 = zoomed in
    void setScrollPosition(float position);  // 0.0-1.0
    
    // Visual settings
    void setShowTransients(bool show);
    void setShowSliceMarkers(bool show);
    
private:
    SampleSlicer& slicer_;
    
    float zoom_;
    float scrollPosition_;
    bool showTransients_;
    bool showSliceMarkers_;
    
    int selectedSlice_;
    bool isDraggingSlice_;
    int draggedSlice_;
    
    // Waveform thumbnail
    juce::AudioThumbnailCache thumbnailCache_;
    std::unique_ptr<juce::AudioThumbnail> thumbnail_;
    
    // Helper methods
    void drawWaveform(juce::Graphics& g);
    void drawSliceMarkers(juce::Graphics& g);
    void drawTransients(juce::Graphics& g);
    
    int sampleToX(int sample) const;
    int xToSample(int x) const;
    
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleSlicerComponent)
};

} // namespace omega
