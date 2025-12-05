#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace Audio {

/**
 * @class SpectrumAnalyzer
 * @brief Analizador de espectro FFT
 */
class SpectrumAnalyzer {
public:
    SpectrumAnalyzer() : fft_(10) {  // 1024-point FFT
        fftData_.resize(fftSize_ * 2, 0.0f);
    }
    
    void pushBuffer(const juce::AudioBuffer<float>& buffer) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        
        for (int i = 0; i < numSamples; ++i) {
            float sample = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch) {
                sample += buffer.getSample(ch, i);
            }
            sample /= numChannels;
            
            fifo_[fifoIndex_] = sample;
            fifoIndex_ = (fifoIndex_ + 1) % fftSize_;
            
            if (fifoIndex_ == 0) {
                // Copy to FFT buffer and apply window
                for (int j = 0; j < fftSize_; ++j) {
                    float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * j / fftSize_));
                    fftData_[j] = fifo_[j] * window;
                }
                
                // Perform FFT
                fft_.performRealOnlyForwardTransform(fftData_.data());
                
                // Calculate magnitudes
                for (int j = 0; j < fftSize_ / 2; ++j) {
                    float real = fftData_[j * 2];
                    float imag = fftData_[j * 2 + 1];
                    float magnitude = std::sqrt(real * real + imag * imag);
                    
                    // Smooth
                    spectrum_[j] = spectrum_[j] * 0.8f + magnitude * 0.2f;
                }
            }
        }
    }
    
    const std::array<float, 512>& getSpectrum() const {
        return spectrum_;
    }
    
private:
    juce::dsp::FFT fft_;
    static constexpr int fftSize_ = 1024;
    std::vector<float> fftData_;
    std::array<float, 1024> fifo_ {};
    std::array<float, 512> spectrum_ {};
    int fifoIndex_ { 0 };
};

/**
 * @class LUFSMeter
 * @brief Medidor de LUFS (loudness)
 */
class LUFSMeter {
public:
    void reset() {
        gatingBlocks_.clear();
        momentary_ = -70.0f;
        shortTerm_ = -70.0f;
        integrated_ = -70.0f;
    }
    
    void processBlock(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        // K-weighting filter (simplified)
        float sum = 0.0f;
        int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                sum += data[i] * data[i];
            }
        }
        
        float rms = std::sqrt(sum / (numSamples * buffer.getNumChannels()));
        float lufs = -0.691f + 10.0f * std::log10(rms + 1e-10f);
        
        // Momentary (400ms)
        momentary_ = momentary_ * 0.95f + lufs * 0.05f;
        
        // Short-term (3s)
        shortTerm_ = shortTerm_ * 0.99f + lufs * 0.01f;
        
        // Integrated (gated)
        if (lufs > -70.0f) {
            gatingBlocks_.push_back(lufs);
            if (gatingBlocks_.size() > 100) {
                gatingBlocks_.erase(gatingBlocks_.begin());
            }
            
            // Calculate gated loudness
            float sum = 0.0f;
            int count = 0;
            for (float block : gatingBlocks_) {
                if (block > -70.0f) {
                    sum += std::pow(10.0f, block / 10.0f);
                    count++;
                }
            }
            
            if (count > 0) {
                integrated_ = 10.0f * std::log10(sum / count);
            }
        }
    }
    
    float getMomentary() const { return momentary_; }
    float getShortTerm() const { return shortTerm_; }
    float getIntegrated() const { return integrated_; }
    
private:
    std::vector<float> gatingBlocks_;
    float momentary_ { -70.0f };
    float shortTerm_ { -70.0f };
    float integrated_ { -70.0f };
};

/**
 * @class ReferenceTrack
 * @brief Track de referencia comercial
 */
class ReferenceTrack {
public:
    ReferenceTrack() = default;
    
    bool loadFromFile(const juce::File& file) {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        
        auto* reader = formatManager.createReaderFor(file);
        if (reader == nullptr) return false;
        
        audioBuffer_.setSize(reader->numChannels, reader->lengthInSamples);
        reader->read(&audioBuffer_, 0, reader->lengthInSamples, 0, true, true);
        sampleRate_ = reader->sampleRate;
        
        delete reader;
        
        // Analyze
        analyzeTrack();
        
        return true;
    }
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        playbackSampleRate_ = sampleRate;
        readPosition_ = 0.0;
    }
    
    void getNextAudioBlock(juce::AudioBuffer<float>& outputBuffer) {
        if (audioBuffer_.getNumSamples() == 0 || !isPlaying_) {
            outputBuffer.clear();
            return;
        }
        
        int numSamples = outputBuffer.getNumSamples();
        int numChannels = std::min(outputBuffer.getNumChannels(), audioBuffer_.getNumChannels());
        
        for (int i = 0; i < numSamples; ++i) {
            int srcIndex = (int)readPosition_;
            
            if (srcIndex >= audioBuffer_.getNumSamples()) {
                if (looping_) {
                    readPosition_ = 0.0;
                    srcIndex = 0;
                } else {
                    break;
                }
            }
            
            float fraction = readPosition_ - srcIndex;
            int nextIndex = (srcIndex + 1) % audioBuffer_.getNumSamples();
            
            for (int ch = 0; ch < numChannels; ++ch) {
                float sample1 = audioBuffer_.getSample(ch, srcIndex);
                float sample2 = audioBuffer_.getSample(ch, nextIndex);
                float interpolated = sample1 + fraction * (sample2 - sample1);
                
                outputBuffer.setSample(ch, i, interpolated * gain_);
            }
            
            readPosition_ += (sampleRate_ / playbackSampleRate_);
        }
    }
    
    void setGain(float gainDb) {
        gain_ = juce::Decibels::decibelsToGain(gainDb);
    }
    
    void setPlaying(bool playing) { isPlaying_ = playing; }
    void setLooping(bool looping) { looping_ = looping; }
    
    float getTargetLUFS() const { return targetLUFS_; }
    const std::array<float, 512>& getSpectrum() const { return spectrum_; }
    
    juce::String getName() const { return name_; }
    void setName(const juce::String& name) { name_ = name; }
    
private:
    void analyzeTrack() {
        // Calculate LUFS
        LUFSMeter meter;
        meter.reset();
        
        const int blockSize = 4410;  // ~100ms at 44.1kHz
        for (int pos = 0; pos < audioBuffer_.getNumSamples(); pos += blockSize) {
            int samplesThisBlock = std::min(blockSize, audioBuffer_.getNumSamples() - pos);
            
            juce::AudioBuffer<float> block(audioBuffer_.getNumChannels(), samplesThisBlock);
            for (int ch = 0; ch < audioBuffer_.getNumChannels(); ++ch) {
                block.copyFrom(ch, 0, audioBuffer_, ch, pos, samplesThisBlock);
            }
            
            meter.processBlock(block, sampleRate_);
        }
        
        targetLUFS_ = meter.getIntegrated();
        
        // Calculate average spectrum
        SpectrumAnalyzer analyzer;
        for (int pos = 0; pos < audioBuffer_.getNumSamples(); pos += 1024) {
            int samplesThisBlock = std::min(1024, audioBuffer_.getNumSamples() - pos);
            
            juce::AudioBuffer<float> block(audioBuffer_.getNumChannels(), samplesThisBlock);
            for (int ch = 0; ch < audioBuffer_.getNumChannels(); ++ch) {
                block.copyFrom(ch, 0, audioBuffer_, ch, pos, samplesThisBlock);
            }
            
            analyzer.pushBuffer(block);
        }
        
        spectrum_ = analyzer.getSpectrum();
    }
    
    juce::AudioBuffer<float> audioBuffer_;
    juce::String name_ { "Reference" };
    double sampleRate_ { 44100.0 };
    double playbackSampleRate_ { 44100.0 };
    double readPosition_ { 0.0 };
    float gain_ { 1.0f };
    bool isPlaying_ { false };
    bool looping_ { true };
    
    float targetLUFS_ { -14.0f };
    std::array<float, 512> spectrum_ {};
};

/**
 * @class ReferenceTrackSystem
 * @brief Sistema de A/B comparison
 */
class ReferenceTrackSystem : public juce::Component,
                             private juce::Timer {
public:
    ReferenceTrackSystem() {
        addAndMakeVisible(loadButton_);
        loadButton_.setButtonText("Load Reference");
        loadButton_.onClick = [this] { loadReference(); };
        
        addAndMakeVisible(playButton_);
        playButton_.setButtonText("Play");
        playButton_.setToggleState(false, juce::dontSendNotification);
        playButton_.onClick = [this] {
            if (referenceTrack_) {
                bool playing = playButton_.getToggleState();
                referenceTrack_->setPlaying(playing);
                if (onPlayStateChanged) onPlayStateChanged(playing);
            }
        };
        
        addAndMakeVisible(abToggle_);
        abToggle_.setButtonText("A/B");
        abToggle_.setToggleState(false, juce::dontSendNotification);
        abToggle_.onClick = [this] {
            bool showingB = abToggle_.getToggleState();
            if (onABToggle) onABToggle(showingB);
        };
        
        addAndMakeVisible(gainSlider_);
        gainSlider_.setRange(-12.0, 12.0, 0.1);
        gainSlider_.setValue(0.0);
        gainSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        gainSlider_.onValueChange = [this] {
            if (referenceTrack_) {
                referenceTrack_->setGain(gainSlider_.getValue());
            }
        };
        
        addAndMakeVisible(gainLabel_);
        gainLabel_.setText("Gain:", juce::dontSendNotification);
        
        addAndMakeVisible(lufsLabel_);
        lufsLabel_.setText("Target LUFS: --", juce::dontSendNotification);
        
        startTimer(50);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Draw spectrum comparison
        auto spectrumBounds = juce::Rectangle<float>(10, 120, getWidth() - 20, 200);
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRect(spectrumBounds);
        
        if (referenceTrack_) {
            // Draw reference spectrum
            g.setColour(juce::Colours::cyan.withAlpha(0.7f));
            juce::Path path;
            
            const auto& spectrum = referenceTrack_->getSpectrum();
            float x = spectrumBounds.getX();
            float y = spectrumBounds.getBottom();
            path.startNewSubPath(x, y);
            
            for (int i = 0; i < 512; i += 2) {
                float magnitude = spectrum[i];
                float db = juce::Decibels::gainToDecibels(magnitude, -80.0f);
                float normDb = juce::jmap(db, -80.0f, 0.0f, 0.0f, 1.0f);
                
                x = spectrumBounds.getX() + (i / 512.0f) * spectrumBounds.getWidth();
                y = spectrumBounds.getBottom() - normDb * spectrumBounds.getHeight();
                
                path.lineTo(x, y);
            }
            
            g.strokePath(path, juce::PathStrokeType(2.0f));
        }
        
        // Draw current mix spectrum (would come from audio engine)
        g.setColour(juce::Colours::orange.withAlpha(0.7f));
        // TODO: Draw current mix spectrum
        
        g.setColour(juce::Colours::white);
        g.drawRect(spectrumBounds, 1.0f);
        g.setFont(12.0f);
        g.drawText("Spectrum Comparison", spectrumBounds.removeFromTop(20), 
                  juce::Justification::centred);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        
        auto topRow = bounds.removeFromTop(30);
        loadButton_.setBounds(topRow.removeFromLeft(120));
        topRow.removeFromLeft(10);
        playButton_.setBounds(topRow.removeFromLeft(80));
        topRow.removeFromLeft(10);
        abToggle_.setBounds(topRow.removeFromLeft(60));
        
        bounds.removeFromTop(10);
        auto sliderRow = bounds.removeFromTop(30);
        gainLabel_.setBounds(sliderRow.removeFromLeft(50));
        gainSlider_.setBounds(sliderRow.removeFromLeft(200));
        
        bounds.removeFromTop(10);
        auto infoRow = bounds.removeFromTop(30);
        lufsLabel_.setBounds(infoRow);
    }
    
    void process(juce::AudioBuffer<float>& buffer) {
        if (referenceTrack_ && abToggle_.getToggleState()) {
            referenceTrack_->getNextAudioBlock(buffer);
        }
    }
    
    std::function<void(bool)> onPlayStateChanged;
    std::function<void(bool)> onABToggle;
    
private:
    void loadReference() {
        juce::FileChooser chooser("Select reference track", {}, "*.wav;*.mp3;*.aif;*.flac");
        
        if (chooser.browseForFileToOpen()) {
            auto file = chooser.getResult();
            
            referenceTrack_ = std::make_unique<ReferenceTrack>();
            if (referenceTrack_->loadFromFile(file)) {
                referenceTrack_->setName(file.getFileNameWithoutExtension());
                
                lufsLabel_.setText(
                    "Target LUFS: " + juce::String(referenceTrack_->getTargetLUFS(), 1) + " LUFS",
                    juce::dontSendNotification
                );
                
                repaint();
            }
        }
    }
    
    void timerCallback() override {
        repaint();
    }
    
    std::unique_ptr<ReferenceTrack> referenceTrack_;
    
    juce::TextButton loadButton_;
    juce::TextButton playButton_;
    juce::TextButton abToggle_;
    juce::Slider gainSlider_;
    juce::Label gainLabel_;
    juce::Label lufsLabel_;
};

} // namespace Audio
} // namespace OmegaStudio
