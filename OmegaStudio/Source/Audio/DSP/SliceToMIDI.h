#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace Audio {
namespace DSP {

/**
 * @struct AudioSlice
 * @brief Slice de audio con timing y pitch
 */
struct AudioSlice {
    int startSample { 0 };
    int endSample { 0 };
    float detectedPitch { 60.0f };  // MIDI note
    float energy { 0.0f };
    juce::AudioBuffer<float> buffer;
    
    int lengthSamples() const { return endSample - startSample; }
};

/**
 * @class TransientDetector
 * @brief Detecta transientes en audio
 */
class TransientDetector {
public:
    TransientDetector() = default;
    
    void setSensitivity(float sens) { 
        sensitivity_ = juce::jlimit(0.0f, 1.0f, sens);
        updateThreshold();
    }
    
    void setMinDistance(int samples) { minDistance_ = samples; }
    
    /**
     * Detecta transientes en buffer de audio
     */
    std::vector<int> detectTransients(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        std::vector<int> transients;
        
        const int hopSize = 256;
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        // Calculate spectral flux for transient detection
        std::vector<float> flux;
        std::vector<float> prevMagnitudes;
        
        juce::dsp::FFT fft(10);  // 1024-point FFT
        const int fftSize = 1024;
        std::vector<float> fftData(fftSize * 2, 0.0f);
        
        for (int pos = 0; pos < numSamples - fftSize; pos += hopSize) {
            // Mix down to mono and apply window
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            for (int ch = 0; ch < numChannels; ++ch) {
                const float* data = buffer.getReadPointer(ch, pos);
                for (int i = 0; i < fftSize; ++i) {
                    float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
                    fftData[i] += data[i] * window / numChannels;
                }
            }
            
            // Perform FFT
            fft.performRealOnlyForwardTransform(fftData.data());
            
            // Calculate magnitudes
            std::vector<float> magnitudes(fftSize / 2);
            for (int i = 0; i < fftSize / 2; ++i) {
                float real = fftData[i * 2];
                float imag = fftData[i * 2 + 1];
                magnitudes[i] = std::sqrt(real * real + imag * imag);
            }
            
            // Calculate spectral flux
            if (!prevMagnitudes.empty()) {
                float fluxValue = 0.0f;
                for (size_t i = 0; i < magnitudes.size(); ++i) {
                    float diff = magnitudes[i] - prevMagnitudes[i];
                    if (diff > 0.0f) {
                        fluxValue += diff;
                    }
                }
                flux.push_back(fluxValue);
            } else {
                flux.push_back(0.0f);
            }
            
            prevMagnitudes = magnitudes;
        }
        
        // Normalize flux
        float maxFlux = *std::max_element(flux.begin(), flux.end());
        if (maxFlux > 0.0f) {
            for (auto& f : flux) {
                f /= maxFlux;
            }
        }
        
        // Peak picking with threshold
        updateThreshold();
        int lastTransient = -minDistance_;
        
        for (size_t i = 1; i < flux.size() - 1; ++i) {
            int samplePos = i * hopSize;
            
            if (flux[i] > threshold_ &&
                flux[i] > flux[i - 1] &&
                flux[i] > flux[i + 1] &&
                samplePos - lastTransient >= minDistance_) {
                
                transients.push_back(samplePos);
                lastTransient = samplePos;
            }
        }
        
        return transients;
    }
    
private:
    void updateThreshold() {
        // Map sensitivity to threshold (inverted)
        threshold_ = 0.8f - (sensitivity_ * 0.6f);
    }
    
    float sensitivity_ { 0.5f };
    float threshold_ { 0.5f };
    int minDistance_ { 4410 };  // ~100ms at 44.1kHz
};

/**
 * @class PitchDetector
 * @brief Detecta pitch en slice de audio
 */
class PitchDetector {
public:
    /**
     * Detecta pitch usando autocorrelación (YIN algorithm simplified)
     */
    float detectPitch(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        if (buffer.getNumSamples() < 2048) return 60.0f;  // Default middle C
        
        const float* data = buffer.getReadPointer(0);
        const int numSamples = std::min(4096, buffer.getNumSamples());
        
        // Autocorrelation
        std::vector<float> correlation(numSamples / 2);
        
        for (int tau = 0; tau < numSamples / 2; ++tau) {
            float sum = 0.0f;
            for (int i = 0; i < numSamples / 2; ++i) {
                float diff = data[i] - data[i + tau];
                sum += diff * diff;
            }
            correlation[tau] = sum;
        }
        
        // Find minimum (best correlation)
        int minLag = 20;  // ~2.2kHz at 44.1kHz
        int maxLag = numSamples / 2 - 1;
        
        auto minIt = std::min_element(correlation.begin() + minLag, 
                                     correlation.begin() + maxLag);
        int lag = std::distance(correlation.begin(), minIt);
        
        // Convert lag to frequency
        float frequency = sampleRate / lag;
        
        // Convert to MIDI note
        float midiNote = 69.0f + 12.0f * std::log2(frequency / 440.0f);
        
        return juce::jlimit(0.0f, 127.0f, midiNote);
    }
};

/**
 * @class SliceToMIDI
 * @brief Sistema completo de slice to MIDI
 */
class SliceToMIDI {
public:
    SliceToMIDI() = default;
    
    void setSensitivity(float sens) {
        transientDetector_.setSensitivity(sens);
    }
    
    void setMinSliceLength(double seconds, double sampleRate) {
        int samples = seconds * sampleRate;
        transientDetector_.setMinDistance(samples);
    }
    
    /**
     * Detecta slices en audio
     */
    std::vector<AudioSlice> sliceAudio(const juce::AudioBuffer<float>& buffer, 
                                       double sampleRate,
                                       bool detectPitch = true) {
        std::vector<AudioSlice> slices;
        
        // Detect transients
        auto transients = transientDetector_.detectTransients(buffer, sampleRate);
        
        if (transients.empty()) {
            // No transients, treat whole buffer as one slice
            AudioSlice slice;
            slice.startSample = 0;
            slice.endSample = buffer.getNumSamples();
            slice.buffer.makeCopyOf(buffer);
            slices.push_back(slice);
            return slices;
        }
        
        // Create slices from transients
        for (size_t i = 0; i < transients.size(); ++i) {
            AudioSlice slice;
            slice.startSample = transients[i];
            
            // End is next transient or buffer end
            if (i + 1 < transients.size()) {
                slice.endSample = transients[i + 1];
            } else {
                slice.endSample = buffer.getNumSamples();
            }
            
            // Copy audio data
            int length = slice.lengthSamples();
            slice.buffer.setSize(buffer.getNumChannels(), length);
            
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                slice.buffer.copyFrom(ch, 0, 
                                     buffer, ch, 
                                     slice.startSample, length);
            }
            
            // Calculate energy
            float energy = 0.0f;
            for (int ch = 0; ch < slice.buffer.getNumChannels(); ++ch) {
                const float* data = slice.buffer.getReadPointer(ch);
                for (int i = 0; i < length; ++i) {
                    energy += std::abs(data[i]);
                }
            }
            slice.energy = energy / (length * slice.buffer.getNumChannels());
            
            // Detect pitch if requested
            if (detectPitch) {
                slice.detectedPitch = pitchDetector_.detectPitch(slice.buffer, sampleRate);
            }
            
            slices.push_back(slice);
        }
        
        return slices;
    }
    
    /**
     * Mapea slices a notas MIDI (para pads)
     */
    juce::MidiBuffer mapSlicesToMIDI(const std::vector<AudioSlice>& slices,
                                     int baseNote = 36,  // C1
                                     int velocity = 100) {
        juce::MidiBuffer buffer;
        
        for (size_t i = 0; i < slices.size() && i < 16; ++i) {  // Max 16 pads
            int midiNote = baseNote + i;
            
            // Note on at slice start
            buffer.addEvent(
                juce::MidiMessage::noteOn(1, midiNote, (juce::uint8)velocity),
                slices[i].startSample
            );
            
            // Note off at slice end
            buffer.addEvent(
                juce::MidiMessage::noteOff(1, midiNote),
                slices[i].endSample
            );
        }
        
        return buffer;
    }
    
    /**
     * Exporta slices como samples individuales
     */
    void exportSlices(const std::vector<AudioSlice>& slices,
                     const juce::File& outputDirectory,
                     const juce::String& baseName,
                     double sampleRate) {
        outputDirectory.createDirectory();
        
        for (size_t i = 0; i < slices.size(); ++i) {
            juce::String filename = baseName + "_slice_" + juce::String(i + 1) + ".wav";
            juce::File outputFile = outputDirectory.getChildFile(filename);
            
            std::unique_ptr<juce::AudioFormatWriter> writer;
            juce::WavAudioFormat format;
            
            auto* fileStream = outputFile.createOutputStream();
            if (fileStream != nullptr) {
                writer.reset(format.createWriterFor(
                    fileStream, 
                    sampleRate,
                    slices[i].buffer.getNumChannels(),
                    24,  // 24-bit
                    {},
                    0
                ));
                
                if (writer != nullptr) {
                    writer->writeFromAudioSampleBuffer(slices[i].buffer, 0, 
                                                      slices[i].buffer.getNumSamples());
                }
            }
        }
    }
    
    /**
     * Auto-quantize slices to tempo grid
     */
    void quantizeSlices(std::vector<AudioSlice>& slices, 
                       double sampleRate,
                       double tempo,
                       int subdivision = 16) {  // 16th notes
        double samplesPerBeat = (60.0 / tempo) * sampleRate;
        double samplesPerStep = samplesPerBeat / (subdivision / 4.0);
        
        for (auto& slice : slices) {
            // Quantize start
            int gridStep = std::round(slice.startSample / samplesPerStep);
            slice.startSample = gridStep * samplesPerStep;
            
            // Quantize end
            gridStep = std::round(slice.endSample / samplesPerStep);
            slice.endSample = gridStep * samplesPerStep;
        }
    }
    
private:
    TransientDetector transientDetector_;
    PitchDetector pitchDetector_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliceToMIDI)
};

/**
 * @class SliceToMIDIComponent
 * @brief UI para slice to MIDI
 */
class SliceToMIDIComponent : public juce::Component,
                             private juce::Timer {
public:
    SliceToMIDIComponent() {
        addAndMakeVisible(loadButton_);
        loadButton_.setButtonText("Load Audio");
        loadButton_.onClick = [this] { loadAudio(); };
        
        addAndMakeVisible(sensitivitySlider_);
        sensitivitySlider_.setRange(0.0, 1.0, 0.01);
        sensitivitySlider_.setValue(0.5);
        sensitivitySlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        sensitivitySlider_.onValueChange = [this] { 
            sliceEngine_.setSensitivity(sensitivitySlider_.getValue());
            detectSlices();
        };
        
        addAndMakeVisible(sensitivityLabel_);
        sensitivityLabel_.setText("Sensitivity:", juce::dontSendNotification);
        
        addAndMakeVisible(detectButton_);
        detectButton_.setButtonText("Detect Slices");
        detectButton_.onClick = [this] { detectSlices(); };
        
        addAndMakeVisible(exportButton_);
        exportButton_.setButtonText("Export Slices");
        exportButton_.onClick = [this] { exportSlices(); };
        
        startTimer(50);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Draw waveform
        if (audioBuffer_.getNumSamples() > 0) {
            drawWaveform(g);
        }
        
        // Draw slice markers
        g.setColour(juce::Colours::red);
        for (const auto& slice : slices_) {
            float x = juce::jmap((float)slice.startSample, 0.0f, 
                               (float)audioBuffer_.getNumSamples(),
                               0.0f, (float)getWidth());
            g.drawVerticalLine(x, 80.0f, 180.0f);
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        
        auto topBar = bounds.removeFromTop(30);
        loadButton_.setBounds(topBar.removeFromLeft(100));
        topBar.removeFromLeft(10);
        detectButton_.setBounds(topBar.removeFromLeft(100));
        topBar.removeFromLeft(10);
        exportButton_.setBounds(topBar.removeFromLeft(100));
        
        bounds.removeFromTop(10);
        auto sliderRow = bounds.removeFromTop(30);
        sensitivityLabel_.setBounds(sliderRow.removeFromLeft(80));
        sensitivitySlider_.setBounds(sliderRow.removeFromLeft(200));
    }
    
private:
    void loadAudio() {
        juce::FileChooser chooser("Select audio file", {}, "*.wav;*.mp3;*.aif;*.flac");
        
        if (chooser.browseForFileToOpen()) {
            auto file = chooser.getResult();
            
            juce::AudioFormatManager formatManager;
            formatManager.registerBasicFormats();
            
            auto* reader = formatManager.createReaderFor(file);
            if (reader != nullptr) {
                audioBuffer_.setSize(reader->numChannels, reader->lengthInSamples);
                reader->read(&audioBuffer_, 0, reader->lengthInSamples, 0, true, true);
                sampleRate_ = reader->sampleRate;
                delete reader;
                
                detectSlices();
            }
        }
    }
    
    void detectSlices() {
        if (audioBuffer_.getNumSamples() == 0) return;
        
        slices_ = sliceEngine_.sliceAudio(audioBuffer_, sampleRate_, true);
        repaint();
    }
    
    void exportSlices() {
        if (slices_.empty()) return;
        
        juce::FileChooser chooser("Select output directory", {}, "");
        if (chooser.browseForDirectory()) {
            auto dir = chooser.getResult();
            sliceEngine_.exportSlices(slices_, dir, "slice", sampleRate_);
        }
    }
    
    void drawWaveform(juce::Graphics& g) {
        auto bounds = juce::Rectangle<float>(0, 80, getWidth(), 100);
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(bounds);
        
        g.setColour(juce::Colours::cyan);
        
        const int numSamples = audioBuffer_.getNumSamples();
        const float* data = audioBuffer_.getReadPointer(0);
        
        juce::Path path;
        float x = 0.0f;
        float y = bounds.getCentreY();
        path.startNewSubPath(x, y);
        
        const int downsample = std::max(1, numSamples / (int)bounds.getWidth());
        
        for (int i = 0; i < numSamples; i += downsample) {
            float sample = data[i];
            x = juce::jmap((float)i, 0.0f, (float)numSamples, bounds.getX(), bounds.getRight());
            y = bounds.getCentreY() - sample * bounds.getHeight() * 0.4f;
            path.lineTo(x, y);
        }
        
        g.strokePath(path, juce::PathStrokeType(1.0f));
    }
    
    void timerCallback() override {
        repaint();
    }
    
    SliceToMIDI sliceEngine_;
    juce::AudioBuffer<float> audioBuffer_;
    std::vector<AudioSlice> slices_;
    double sampleRate_ { 44100.0 };
    
    juce::TextButton loadButton_;
    juce::TextButton detectButton_;
    juce::TextButton exportButton_;
    juce::Slider sensitivitySlider_;
    juce::Label sensitivityLabel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliceToMIDIComponent)
};

} // namespace DSP
} // namespace Audio
} // namespace OmegaStudio
