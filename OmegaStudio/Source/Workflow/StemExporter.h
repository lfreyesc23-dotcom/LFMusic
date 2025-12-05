/*
  ==============================================================================

    StemExporter.h
    Multi-track stem export with auto-grouping

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>

namespace omega {

enum class StemFormat {
    WAV,
    FLAC,
    MP3,
    AAC
};

struct StemGroup {
    juce::String name;
    std::vector<int> trackIndices;
    juce::Colour color;
    
    StemGroup() = default;
    StemGroup(const juce::String& n) : name(n), color(juce::Colours::grey) {}
};

class StemExporter {
public:
    StemExporter();
    
    // Grouping
    void addGroup(const StemGroup& group);
    void autoGroupByInstrument();
    void clearGroups();
    
    // Export
    bool exportStems(const juce::File& outputDirectory, StemFormat format = StemFormat::WAV);
    bool exportSingleStem(int groupIndex, const juce::File& outputFile);
    
    // Settings
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setBitDepth(int bits) { bitDepth_ = bits; }
    void setNormalize(bool normalize) { normalize_ = normalize; }
    void setNamingConvention(const juce::String& pattern) { namingPattern_ = pattern; }
    
    // Progress
    float getProgress() const { return progress_; }
    
private:
    std::vector<StemGroup> groups_;
    double sampleRate_ = 48000.0;
    int bitDepth_ = 24;
    bool normalize_ = true;
    juce::String namingPattern_ = "{trackName}";
    float progress_ = 0.0f;
};

} // namespace omega
