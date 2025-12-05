/*
  ==============================================================================

    StemExporter.cpp

  ==============================================================================
*/

#include "StemExporter.h"

namespace omega {

StemExporter::StemExporter() {}

void StemExporter::addGroup(const StemGroup& group) {
    groups_.push_back(group);
}

void StemExporter::autoGroupByInstrument() {
    groups_.clear();
    
    StemGroup drums("Drums");
    drums.color = juce::Colours::red;
    
    StemGroup bass("Bass");
    bass.color = juce::Colours::blue;
    
    StemGroup vocals("Vocals");
    vocals.color = juce::Colours::green;
    
    StemGroup synths("Synths");
    synths.color = juce::Colours::purple;
    
    groups_.push_back(drums);
    groups_.push_back(bass);
    groups_.push_back(vocals);
    groups_.push_back(synths);
}

void StemExporter::clearGroups() {
    groups_.clear();
}

bool StemExporter::exportStems(const juce::File& outputDirectory, StemFormat format) {
    if (!outputDirectory.exists()) {
        outputDirectory.createDirectory();
    }
    
    progress_ = 0.0f;
    
    for (size_t i = 0; i < groups_.size(); ++i) {
        juce::String filename = groups_[i].name;
        
        if (format == StemFormat::WAV) filename += ".wav";
        else if (format == StemFormat::FLAC) filename += ".flac";
        else if (format == StemFormat::MP3) filename += ".mp3";
        else if (format == StemFormat::AAC) filename += ".m4a";
        
        juce::File outputFile = outputDirectory.getChildFile(filename);
        
        if (!exportSingleStem(static_cast<int>(i), outputFile)) {
            return false;
        }
        
        progress_ = static_cast<float>(i + 1) / groups_.size();
    }
    
    return true;
}

bool StemExporter::exportSingleStem(int groupIndex, const juce::File& outputFile) {
    if (groupIndex < 0 || groupIndex >= static_cast<int>(groups_.size())) {
        return false;
    }
    
    // Create empty buffer as placeholder
    juce::AudioBuffer<float> stemBuffer(2, static_cast<int>(sampleRate_ * 10.0)); // 10 seconds
    stemBuffer.clear();
    
    // Write file
    juce::WavAudioFormat wavFormat;
    auto* writer = wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        sampleRate_,
        static_cast<unsigned int>(stemBuffer.getNumChannels()),
        bitDepth_,
        {},
        0
    );
    
    if (writer == nullptr) return false;
    
    writer->writeFromAudioSampleBuffer(stemBuffer, 0, stemBuffer.getNumSamples());
    delete writer;
    
    return true;
}

} // namespace omega
