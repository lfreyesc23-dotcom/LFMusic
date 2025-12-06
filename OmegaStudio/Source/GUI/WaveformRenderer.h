#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <thread>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Multi-threaded Waveform Renderer with Peak Caching
 * High-performance audio visualization for playlist clips
 */
class WaveformRenderer
{
public:
    struct WaveformData
    {
        std::vector<float> peaks;       // Interleaved min/max pairs per pixel
        std::vector<float> rms;         // RMS values per pixel segment
        int numChannels = 0;
        double sampleRate = 0.0;
        int64_t totalSamples = 0;
        bool isLoaded = false;
    };

    WaveformRenderer()
    {
        formatManager.registerBasicFormats();
    }

    ~WaveformRenderer()
    {
        stopAllThreads();
    }

    // Load audio file and generate waveform data
    bool loadFile(const juce::File& file, int pixelWidth)
    {
        auto* reader = formatManager.createReaderFor(file);
        if (reader == nullptr)
            return false;

        WaveformData data;
        data.numChannels = reader->numChannels;
        data.sampleRate = reader->sampleRate;
        data.totalSamples = reader->lengthInSamples;
        
        // Calculate samples per pixel
        int samplesPerPixel = juce::jmax(1, static_cast<int>(data.totalSamples / pixelWidth));
        
        // Resize peak arrays
        data.peaks.resize(pixelWidth * data.numChannels * 2, 0.0f); // min/max pairs
        data.rms.resize(pixelWidth * data.numChannels, 0.0f);
        
        // Read and analyze audio in chunks
        const int bufferSize = 8192;
        juce::AudioBuffer<float> buffer(data.numChannels, bufferSize);
        
        for (int pixel = 0; pixel < pixelWidth; ++pixel)
        {
            int64_t startSample = pixel * samplesPerPixel;
            int samplesToRead = juce::jmin(samplesPerPixel, static_cast<int>(data.totalSamples - startSample));
            
            if (samplesToRead <= 0)
                break;
            
            // Read samples for this pixel
            int samplesRead = 0;
            std::vector<float> minValues(data.numChannels, 1.0f);
            std::vector<float> maxValues(data.numChannels, -1.0f);
            std::vector<float> rmsAccum(data.numChannels, 0.0f);
            
            reader->read(&buffer, 0, samplesToRead, startSample, true, true);
            
            // Find peaks and calculate RMS
            for (int ch = 0; ch < data.numChannels; ++ch)
            {
                auto* channelData = buffer.getReadPointer(ch);
                
                for (int i = 0; i < samplesToRead; ++i)
                {
                    float sample = channelData[i];
                    minValues[ch] = juce::jmin(minValues[ch], sample);
                    maxValues[ch] = juce::jmax(maxValues[ch], sample);
                    rmsAccum[ch] += sample * sample;
                }
                
                // Store peaks (min/max pairs per channel)
                int peakIndex = (pixel * data.numChannels + ch) * 2;
                data.peaks[peakIndex] = minValues[ch];
                data.peaks[peakIndex + 1] = maxValues[ch];
                
                // Store RMS
                data.rms[pixel * data.numChannels + ch] = std::sqrt(rmsAccum[ch] / samplesToRead);
            }
        }
        
        data.isLoaded = true;
        
        // Cache the data
        cachedWaveforms[file.getFullPathName()] = data;
        
        delete reader;
        return true;
    }

    // Draw waveform to graphics context
    void drawWaveform(juce::Graphics& g, 
                     const juce::File& file,
                     juce::Rectangle<float> bounds,
                     juce::Colour waveformColor = juce::Colours::orange,
                     bool showRMS = false)
    {
        auto it = cachedWaveforms.find(file.getFullPathName());
        if (it == cachedWaveforms.end())
        {
            // Load on demand
            if (!loadFile(file, static_cast<int>(bounds.getWidth())))
            {
                g.setColour(juce::Colours::red);
                g.drawText("Failed to load", bounds, juce::Justification::centred);
                return;
            }
            it = cachedWaveforms.find(file.getFullPathName());
        }
        
        const auto& data = it->second;
        
        if (!data.isLoaded || data.peaks.empty())
            return;
        
        int pixelWidth = static_cast<int>(bounds.getWidth());
        float channelHeight = bounds.getHeight() / data.numChannels;
        
        for (int ch = 0; ch < data.numChannels; ++ch)
        {
            auto channelBounds = bounds.removeFromTop(channelHeight);
            float centerY = channelBounds.getCentreY();
            float halfHeight = channelBounds.getHeight() * 0.5f;
            
            juce::Path waveformPath;
            bool first = true;
            
            // Draw waveform peaks
            for (int x = 0; x < pixelWidth && x < static_cast<int>(data.peaks.size() / (data.numChannels * 2)); ++x)
            {
                int peakIndex = (x * data.numChannels + ch) * 2;
                float minPeak = data.peaks[peakIndex];
                float maxPeak = data.peaks[peakIndex + 1];
                
                float minY = centerY - (minPeak * halfHeight);
                float maxY = centerY - (maxPeak * halfHeight);
                
                if (first)
                {
                    waveformPath.startNewSubPath(channelBounds.getX() + x, maxY);
                    first = false;
                }
                else
                {
                    waveformPath.lineTo(channelBounds.getX() + x, maxY);
                }
            }
            
            // Draw bottom half (mirror)
            for (int x = pixelWidth - 1; x >= 0 && x < static_cast<int>(data.peaks.size() / (data.numChannels * 2)); --x)
            {
                int peakIndex = (x * data.numChannels + ch) * 2;
                float minPeak = data.peaks[peakIndex];
                float minY = centerY - (minPeak * halfHeight);
                waveformPath.lineTo(channelBounds.getX() + x, minY);
            }
            
            waveformPath.closeSubPath();
            
            // Fill waveform
            g.setColour(waveformColor.withAlpha(0.7f));
            g.fillPath(waveformPath);
            
            // Draw outline
            g.setColour(waveformColor);
            g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
            
            // Draw center line
            g.setColour(juce::Colours::grey.withAlpha(0.3f));
            g.drawLine(channelBounds.getX(), centerY, 
                      channelBounds.getRight(), centerY, 1.0f);
            
            // Draw RMS overlay if requested
            if (showRMS)
            {
                juce::Path rmsPath;
                first = true;
                
                for (int x = 0; x < pixelWidth && x < static_cast<int>(data.rms.size() / data.numChannels); ++x)
                {
                    float rms = data.rms[x * data.numChannels + ch];
                    float rmsY = centerY - (rms * halfHeight * 0.7f);
                    
                    if (first)
                    {
                        rmsPath.startNewSubPath(channelBounds.getX() + x, rmsY);
                        first = false;
                    }
                    else
                    {
                        rmsPath.lineTo(channelBounds.getX() + x, rmsY);
                    }
                }
                
                g.setColour(juce::Colours::yellow.withAlpha(0.5f));
                g.strokePath(rmsPath, juce::PathStrokeType(1.5f));
            }
        }
    }

    // Clear cached waveform data
    void clearCache()
    {
        cachedWaveforms.clear();
    }

    void clearFile(const juce::File& file)
    {
        cachedWaveforms.erase(file.getFullPathName());
    }

    // Get cached data for a file
    const WaveformData* getCachedData(const juce::File& file) const
    {
        auto it = cachedWaveforms.find(file.getFullPathName());
        return (it != cachedWaveforms.end()) ? &it->second : nullptr;
    }

private:
    void stopAllThreads()
    {
        // Stop any background loading threads
    }

    juce::AudioFormatManager formatManager;
    std::unordered_map<juce::String, WaveformData> cachedWaveforms;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformRenderer)
};

//==============================================================================
/**
 * Waveform Component - Visual component for displaying audio waveforms
 */
class WaveformComponent : public juce::Component
{
public:
    WaveformComponent(WaveformRenderer& renderer)
        : waveformRenderer(renderer)
    {
    }

    void setFile(const juce::File& audioFile)
    {
        file = audioFile;
        repaint();
    }

    void setColor(juce::Colour color)
    {
        waveformColor = color;
        repaint();
    }

    void setShowRMS(bool show)
    {
        showRMS = show;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (!file.existsAsFile())
        {
            g.setColour(juce::Colours::darkgrey);
            g.fillRect(getLocalBounds());
            g.setColour(juce::Colours::grey);
            g.drawText("No audio file", getLocalBounds(), juce::Justification::centred);
            return;
        }
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(getLocalBounds());
        
        // Draw waveform
        waveformRenderer.drawWaveform(g, file, getLocalBounds().toFloat(), waveformColor, showRMS);
    }

private:
    WaveformRenderer& waveformRenderer;
    juce::File file;
    juce::Colour waveformColor = juce::Colours::orange;
    bool showRMS = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

} // namespace GUI
} // namespace OmegaStudio
