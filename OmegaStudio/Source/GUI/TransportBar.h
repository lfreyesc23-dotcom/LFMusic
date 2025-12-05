#pragma once

#include <JuceHeader.h>
#include <atomic>

namespace OmegaStudio {
namespace GUI {

/**
 * @class CPUMeter
 * @brief Medidor de CPU
 */
class CPUMeter : public juce::Component,
                 private juce::Timer {
public:
    CPUMeter() {
        startTimer(100);  // Update every 100ms
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRect(bounds);
        
        // CPU bar
        float cpuLevel = cpuUsage_.load();
        float barWidth = bounds.getWidth() * cpuLevel;
        
        juce::Colour barColour = cpuLevel > 0.8f ? juce::Colours::red :
                                cpuLevel > 0.6f ? juce::Colours::orange :
                                juce::Colours::green;
        
        g.setColour(barColour);
        g.fillRect(bounds.withWidth(barWidth));
        
        // Border
        g.setColour(juce::Colours::white);
        g.drawRect(bounds, 1.0f);
        
        // Text
        g.setFont(10.0f);
        juce::String text = juce::String((int)(cpuLevel * 100)) + "%";
        g.drawText(text, bounds, juce::Justification::centred);
    }
    
    void setCPUUsage(float usage) {
        cpuUsage_.store(juce::jlimit(0.0f, 1.0f, usage));
    }
    
private:
    void timerCallback() override {
        repaint();
    }
    
    std::atomic<float> cpuUsage_ { 0.0f };
};

/**
 * @class DiskMeter
 * @brief Medidor de uso de disco
 */
class DiskMeter : public juce::Component,
                  private juce::Timer {
public:
    DiskMeter() {
        startTimer(200);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRect(bounds);
        
        float diskLevel = diskUsage_.load();
        float barWidth = bounds.getWidth() * diskLevel;
        
        juce::Colour barColour = diskLevel > 0.8f ? juce::Colours::red :
                                diskLevel > 0.5f ? juce::Colours::yellow :
                                juce::Colours::cyan;
        
        g.setColour(barColour);
        g.fillRect(bounds.withWidth(barWidth));
        
        g.setColour(juce::Colours::white);
        g.drawRect(bounds, 1.0f);
        
        g.setFont(10.0f);
        juce::String text = juce::String((int)(diskLevel * 100)) + "%";
        g.drawText(text, bounds, juce::Justification::centred);
    }
    
    void setDiskUsage(float usage) {
        diskUsage_.store(juce::jlimit(0.0f, 1.0f, usage));
    }
    
private:
    void timerCallback() override {
        repaint();
    }
    
    std::atomic<float> diskUsage_ { 0.0f };
};

/**
 * @class TimeDisplay
 * @brief Display de tiempo (bars:beats:ticks / samples / time)
 */
class TimeDisplay : public juce::Component,
                    private juce::Timer {
public:
    enum class Format { BarsBeatsTicks, Samples, MinutesSeconds };
    
    TimeDisplay() {
        startTimer(50);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        
        juce::String text = formatTime();
        g.drawText(text, getLocalBounds(), juce::Justification::centred);
    }
    
    void setPosition(double samples, double sampleRate, double tempo) {
        positionSamples_ = samples;
        sampleRate_ = sampleRate;
        tempo_ = tempo;
    }
    
    void setFormat(Format fmt) { format_ = fmt; }
    
private:
    void timerCallback() override {
        repaint();
    }
    
    juce::String formatTime() const {
        switch (format_) {
            case Format::BarsBeatsTicks: {
                double beatsPerSample = tempo_ / (60.0 * sampleRate_);
                double totalBeats = positionSamples_ * beatsPerSample;
                
                int bars = (int)(totalBeats / 4.0) + 1;
                int beats = (int)std::fmod(totalBeats, 4.0) + 1;
                int ticks = (int)(std::fmod(totalBeats, 1.0) * 960.0);
                
                return juce::String::formatted("%03d:%d:%03d", bars, beats, ticks);
            }
            
            case Format::Samples:
                return juce::String((int64_t)positionSamples_);
            
            case Format::MinutesSeconds: {
                double seconds = positionSamples_ / sampleRate_;
                int minutes = (int)(seconds / 60.0);
                int secs = (int)std::fmod(seconds, 60.0);
                int ms = (int)(std::fmod(seconds, 1.0) * 100.0);
                
                return juce::String::formatted("%02d:%02d.%02d", minutes, secs, ms);
            }
        }
        
        return "00:00.00";
    }
    
    double positionSamples_ { 0.0 };
    double sampleRate_ { 44100.0 };
    double tempo_ { 120.0 };
    Format format_ { Format::BarsBeatsTicks };
};

/**
 * @class TransportBar
 * @brief Barra de transporte completa
 */
class TransportBar : public juce::Component {
public:
    TransportBar() {
        // Play button
        addAndMakeVisible(playButton_);
        playButton_.setButtonText("▶");
        playButton_.setToggleState(false, juce::dontSendNotification);
        playButton_.onClick = [this] {
            bool playing = playButton_.getToggleState();
            if (onPlayStateChanged) onPlayStateChanged(playing);
        };
        
        // Stop button
        addAndMakeVisible(stopButton_);
        stopButton_.setButtonText("■");
        stopButton_.onClick = [this] {
            playButton_.setToggleState(false, juce::dontSendNotification);
            if (onStop) onStop();
        };
        
        // Record button
        addAndMakeVisible(recordButton_);
        recordButton_.setButtonText("●");
        recordButton_.setToggleState(false, juce::dontSendNotification);
        recordButton_.onClick = [this] {
            bool recording = recordButton_.getToggleState();
            if (onRecordStateChanged) onRecordStateChanged(recording);
        };
        
        // Loop button
        addAndMakeVisible(loopButton_);
        loopButton_.setButtonText("⟲");
        loopButton_.setToggleState(false, juce::dontSendNotification);
        loopButton_.onClick = [this] {
            bool looping = loopButton_.getToggleState();
            if (onLoopStateChanged) onLoopStateChanged(looping);
        };
        
        // Metronome button
        addAndMakeVisible(metronomeButton_);
        metronomeButton_.setButtonText("♪");
        metronomeButton_.setToggleState(false, juce::dontSendNotification);
        metronomeButton_.onClick = [this] {
            bool enabled = metronomeButton_.getToggleState();
            if (onMetronomeStateChanged) onMetronomeStateChanged(enabled);
        };
        
        // Tempo
        addAndMakeVisible(tempoSlider_);
        tempoSlider_.setRange(20.0, 300.0, 0.1);
        tempoSlider_.setValue(120.0);
        tempoSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
        tempoSlider_.onValueChange = [this] {
            if (onTempoChanged) onTempoChanged(tempoSlider_.getValue());
        };
        
        addAndMakeVisible(tempoLabel_);
        tempoLabel_.setText("BPM:", juce::dontSendNotification);
        
        // Time signature
        addAndMakeVisible(timeSigCombo_);
        timeSigCombo_.addItem("4/4", 1);
        timeSigCombo_.addItem("3/4", 2);
        timeSigCombo_.addItem("5/4", 3);
        timeSigCombo_.addItem("6/8", 4);
        timeSigCombo_.addItem("7/8", 5);
        timeSigCombo_.setSelectedId(1);
        
        // Time display
        addAndMakeVisible(timeDisplay_);
        
        // CPU meter
        addAndMakeVisible(cpuLabel_);
        cpuLabel_.setText("CPU:", juce::dontSendNotification);
        addAndMakeVisible(cpuMeter_);
        
        // Disk meter
        addAndMakeVisible(diskLabel_);
        diskLabel_.setText("DISK:", juce::dontSendNotification);
        addAndMakeVisible(diskMeter_);
        
        // Pre-roll
        addAndMakeVisible(preRollCombo_);
        preRollCombo_.addItem("Off", 1);
        preRollCombo_.addItem("1 Bar", 2);
        preRollCombo_.addItem("2 Bars", 3);
        preRollCombo_.addItem("4 Bars", 4);
        preRollCombo_.setSelectedId(1);
        
        addAndMakeVisible(preRollLabel_);
        preRollLabel_.setText("Pre-roll:", juce::dontSendNotification);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(getLocalBounds(), 2);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(5);
        
        // Left section: transport controls
        auto leftSection = bounds.removeFromLeft(300);
        auto buttonRow = leftSection.removeFromTop(40);
        
        playButton_.setBounds(buttonRow.removeFromLeft(50).reduced(5));
        stopButton_.setBounds(buttonRow.removeFromLeft(50).reduced(5));
        recordButton_.setBounds(buttonRow.removeFromLeft(50).reduced(5));
        loopButton_.setBounds(buttonRow.removeFromLeft(50).reduced(5));
        metronomeButton_.setBounds(buttonRow.removeFromLeft(50).reduced(5));
        
        // Center section: time and tempo
        auto centerSection = bounds.removeFromLeft(400);
        
        auto timeRow = centerSection.removeFromTop(40);
        timeDisplay_.setBounds(timeRow.removeFromLeft(150).reduced(5));
        
        timeRow.removeFromLeft(10);
        tempoLabel_.setBounds(timeRow.removeFromLeft(40).reduced(5));
        tempoSlider_.setBounds(timeRow.removeFromLeft(150).reduced(5));
        
        timeRow.removeFromLeft(10);
        timeSigCombo_.setBounds(timeRow.removeFromLeft(60).reduced(5));
        
        // Right section: meters and pre-roll
        auto rightSection = bounds;
        
        auto metersRow = rightSection.removeFromTop(40);
        
        cpuLabel_.setBounds(metersRow.removeFromLeft(40).reduced(5));
        cpuMeter_.setBounds(metersRow.removeFromLeft(80).reduced(5));
        
        metersRow.removeFromLeft(10);
        diskLabel_.setBounds(metersRow.removeFromLeft(40).reduced(5));
        diskMeter_.setBounds(metersRow.removeFromLeft(80).reduced(5));
        
        metersRow.removeFromLeft(10);
        preRollLabel_.setBounds(metersRow.removeFromLeft(60).reduced(5));
        preRollCombo_.setBounds(metersRow.removeFromLeft(80).reduced(5));
    }
    
    void updatePosition(double samples, double sampleRate, double tempo) {
        timeDisplay_.setPosition(samples, sampleRate, tempo);
    }
    
    void setCPUUsage(float usage) {
        cpuMeter_.setCPUUsage(usage);
    }
    
    void setDiskUsage(float usage) {
        diskMeter_.setDiskUsage(usage);
    }
    
    bool isPlaying() const { return playButton_.getToggleState(); }
    bool isRecording() const { return recordButton_.getToggleState(); }
    bool isLooping() const { return loopButton_.getToggleState(); }
    bool isMetronomeEnabled() const { return metronomeButton_.getToggleState(); }
    
    double getTempo() const { return tempoSlider_.getValue(); }
    int getPreRollBars() const {
        int id = preRollCombo_.getSelectedId();
        switch (id) {
            case 2: return 1;
            case 3: return 2;
            case 4: return 4;
            default: return 0;
        }
    }
    
    std::function<void(bool)> onPlayStateChanged;
    std::function<void()> onStop;
    std::function<void(bool)> onRecordStateChanged;
    std::function<void(bool)> onLoopStateChanged;
    std::function<void(bool)> onMetronomeStateChanged;
    std::function<void(double)> onTempoChanged;
    
private:
    juce::TextButton playButton_;
    juce::TextButton stopButton_;
    juce::TextButton recordButton_;
    juce::TextButton loopButton_;
    juce::TextButton metronomeButton_;
    
    juce::Slider tempoSlider_;
    juce::Label tempoLabel_;
    
    juce::ComboBox timeSigCombo_;
    juce::ComboBox preRollCombo_;
    juce::Label preRollLabel_;
    
    TimeDisplay timeDisplay_;
    
    CPUMeter cpuMeter_;
    juce::Label cpuLabel_;
    
    DiskMeter diskMeter_;
    juce::Label diskLabel_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};

} // namespace GUI
} // namespace OmegaStudio
