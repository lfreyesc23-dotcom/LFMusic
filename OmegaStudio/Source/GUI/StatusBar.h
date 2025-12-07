//==============================================================================
// StatusBar.h
// FL Studio 2025 Professional Status Bar
// CPU/RAM/Disk monitoring + Hint Panel
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Performance Meter - CPU/RAM visual meter
//==============================================================================
class PerformanceMeter : public juce::Component,
                        private juce::Timer {
public:
    PerformanceMeter(const juce::String& label) : labelText(label) {
        startTimer(100); // Update 10x/segundo
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Label
        g.setColour(juce::Colour(0xffb0b0b0));
        g.setFont(10.0f);
        g.drawText(labelText, bounds.removeFromTop(12), juce::Justification::centred);
        
        bounds.reduce(4, 2);
        
        // Meter background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 2.0f);
        
        // Meter fill con color según nivel
        auto fillWidth = bounds.getWidth() * currentValue;
        auto fillBounds = bounds.withWidth(fillWidth);
        
        juce::Colour meterColour;
        if (currentValue < 0.7f)
            meterColour = juce::Colour(0xff36ff8c); // Verde
        else if (currentValue < 0.85f)
            meterColour = juce::Colour(0xfffff036); // Amarillo
        else
            meterColour = juce::Colour(0xffff3636); // Rojo
        
        g.setColour(meterColour);
        g.fillRoundedRectangle(fillBounds, 2.0f);
        
        // Peak hold indicator
        if (peakValue > currentValue) {
            float peakX = bounds.getX() + bounds.getWidth() * peakValue;
            g.setColour(meterColour.brighter(0.3f));
            g.fillRect(peakX - 1, bounds.getY(), 2.0f, bounds.getHeight());
        }
        
        // Percentage text
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        juce::String text = juce::String((int)(currentValue * 100)) + "%";
        g.drawText(text, bounds, juce::Justification::centred);
        
        // Border
        g.setColour(juce::Colour(0xff0a0a0a));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 3.0f, 1.0f);
    }
    
    void setValue(float newValue) {
        newValue = juce::jlimit(0.0f, 1.0f, newValue);
        currentValue = newValue;
        
        // Update peak hold
        if (newValue > peakValue) {
            peakValue = newValue;
            peakHoldCounter = 0;
        }
        
        repaint();
    }
    
    float getValue() const { return currentValue; }
    
private:
    juce::String labelText;
    float currentValue = 0.0f;
    float peakValue = 0.0f;
    int peakHoldCounter = 0;
    
    void timerCallback() override {
        // Peak hold decay
        if (++peakHoldCounter > 20) { // 2 segundos
            peakValue *= 0.95f;
            if (peakValue < 0.01f) peakValue = 0.0f;
            repaint();
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceMeter)
};

//==============================================================================
// Hint Panel - Tooltips contextuales expandidos
//==============================================================================
class HintPanel : public juce::Component {
public:
    HintPanel() {
        setOpaque(true);
    }
    
    void paint(juce::Graphics& g) override {
        // Background con gradiente
        juce::ColourGradient gradient(
            juce::Colour(0xff3a3a3a), 0, 0,
            juce::Colour(0xff2d2d2d), 0, (float)getHeight(),
            false);
        g.setGradientFill(gradient);
        g.fillAll();
        
        // Border superior
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawLine(0, 0, (float)getWidth(), 0, 2.0f);
        
        auto bounds = getLocalBounds().reduced(8, 4);
        
        // Icon
        if (!currentIcon.isEmpty()) {
            auto iconArea = bounds.removeFromLeft(24);
            g.setColour(juce::Colour(0xffff8736));
            g.setFont(juce::Font(16.0f));
            g.drawText(currentIcon, iconArea, juce::Justification::centred);
            bounds.removeFromLeft(4);
        }
        
        // Hint text
        g.setColour(juce::Colour(0xffdddddd));
        g.setFont(11.0f);
        g.drawText(currentHint, bounds, juce::Justification::centredLeft);
        
        // Shortcut (si existe)
        if (!currentShortcut.isEmpty()) {
            g.setColour(juce::Colour(0xff9a9a9a));
            g.setFont(juce::Font(10.0f, juce::Font::italic));
            auto shortcutArea = bounds.removeFromRight(80);
            g.drawText(currentShortcut, shortcutArea, juce::Justification::centredRight);
        }
    }
    
    void setHint(const juce::String& hint, 
                 const juce::String& icon = "ℹ️",
                 const juce::String& shortcut = "") {
        currentHint = hint;
        currentIcon = icon;
        currentShortcut = shortcut;
        repaint();
    }
    
    void clearHint() {
        currentHint = "Ready";
        currentIcon = "✓";
        currentShortcut = "";
        repaint();
    }
    
private:
    juce::String currentHint = "Ready";
    juce::String currentIcon = "✓";
    juce::String currentShortcut;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintPanel)
};

//==============================================================================
// Status Bar - Barra de estado completa profesional
//==============================================================================
class StatusBar : public juce::Component,
                  private juce::Timer {
public:
    StatusBar() {
        // CPU Meter
        cpuMeter = std::make_unique<PerformanceMeter>("CPU");
        addAndMakeVisible(cpuMeter.get());
        
        // RAM Meter
        ramMeter = std::make_unique<PerformanceMeter>("RAM");
        addAndMakeVisible(ramMeter.get());
        
        // Disk Buffer Indicator
        diskLabel.setText("DISK", juce::dontSendNotification);
        diskLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0b0b0));
        diskLabel.setFont(juce::Font(10.0f, juce::Font::bold));
        diskLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(diskLabel);
        
        diskIndicator.setColour(juce::Label::backgroundColourId, juce::Colour(0xff36ff8c));
        addAndMakeVisible(diskIndicator);
        
        // Buffer Size Display
        bufferLabel.setText("BUFFER: 512", juce::dontSendNotification);
        bufferLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0b0b0));
        bufferLabel.setFont(juce::Font(10.0f));
        addAndMakeVisible(bufferLabel);
        
        // Sample Rate Display
        sampleRateLabel.setText("44.1 kHz", juce::dontSendNotification);
        sampleRateLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0b0b0));
        sampleRateLabel.setFont(juce::Font(10.0f));
        addAndMakeVisible(sampleRateLabel);
        
        // Bit Depth Display
        bitDepthLabel.setText("24-bit", juce::dontSendNotification);
        bitDepthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffb0b0b0));
        bitDepthLabel.setFont(juce::Font(10.0f));
        addAndMakeVisible(bitDepthLabel);
        
        // Time Display
        timeLabel.setText("00:00:00", juce::dontSendNotification);
        timeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffdddddd));
        timeLabel.setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(timeLabel);
        
        // Hint Panel
        hintPanel = std::make_unique<HintPanel>();
        addAndMakeVisible(hintPanel.get());
        
        startTimer(250); // Update 4x/segundo
    }
    
    void paint(juce::Graphics& g) override {
        // Background
        g.fillAll(juce::Colour(0xff232323));
        
        // Top border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawLine(0, 0, (float)getWidth(), 0, 1.0f);
        
        // Separadores verticales
        g.setColour(juce::Colour(0xff1a1a1a));
        float separatorX = 220;
        g.drawLine(separatorX, 4, separatorX, (float)getHeight() - 4, 1.0f);
    }
    
    void resized() override {
        auto area = getLocalBounds().reduced(4, 2);
        
        // Performance meters (izquierda)
        cpuMeter->setBounds(area.removeFromLeft(70));
        area.removeFromLeft(4);
        ramMeter->setBounds(area.removeFromLeft(70));
        area.removeFromLeft(4);
        
        // Disk indicator
        auto diskArea = area.removeFromLeft(50);
        diskLabel.setBounds(diskArea.removeFromTop(12));
        diskIndicator.setBounds(diskArea.reduced(4, 2));
        area.removeFromLeft(8);
        
        // Audio info (centro-izquierda)
        sampleRateLabel.setBounds(area.removeFromLeft(60));
        area.removeFromLeft(4);
        bitDepthLabel.setBounds(area.removeFromLeft(50));
        area.removeFromLeft(4);
        bufferLabel.setBounds(area.removeFromLeft(80));
        area.removeFromLeft(8);
        
        // Time (derecha)
        timeLabel.setBounds(area.removeFromRight(70));
        area.removeFromRight(8);
        
        // Hint panel (resto del espacio)
        hintPanel->setBounds(area);
    }
    
    // API para actualizar métricas
    void updateCPUUsage(float usage) {
        cpuMeter->setValue(usage);
    }
    
    void updateRAMUsage(float usage) {
        ramMeter->setValue(usage);
    }
    
    void setDiskBufferOK(bool ok) {
        diskIndicator.setColour(juce::Label::backgroundColourId,
            ok ? juce::Colour(0xff36ff8c) : juce::Colour(0xffff3636));
    }
    
    void setBufferSize(int samples) {
        bufferLabel.setText("BUFFER: " + juce::String(samples), juce::dontSendNotification);
    }
    
    void setSampleRate(double rate) {
        sampleRateLabel.setText(juce::String(rate / 1000.0, 1) + " kHz", 
                               juce::dontSendNotification);
    }
    
    void setBitDepth(int bits) {
        bitDepthLabel.setText(juce::String(bits) + "-bit", juce::dontSendNotification);
    }
    
    void setTime(double seconds) {
        int hours = (int)(seconds / 3600);
        int minutes = ((int)seconds % 3600) / 60;
        int secs = (int)seconds % 60;
        
        juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", hours, minutes, secs);
        timeLabel.setText(timeStr, juce::dontSendNotification);
    }
    
    void showHint(const juce::String& hint, 
                  const juce::String& icon = "ℹ️",
                  const juce::String& shortcut = "") {
        hintPanel->setHint(hint, icon, shortcut);
    }
    
    void clearHint() {
        hintPanel->clearHint();
    }
    
private:
    std::unique_ptr<PerformanceMeter> cpuMeter;
    std::unique_ptr<PerformanceMeter> ramMeter;
    juce::Label diskLabel;
    juce::Label diskIndicator;
    juce::Label bufferLabel;
    juce::Label sampleRateLabel;
    juce::Label bitDepthLabel;
    juce::Label timeLabel;
    std::unique_ptr<HintPanel> hintPanel;
    
    void timerCallback() override {
        // Actualizar CPU usage (simulado - en producción usar valores reales)
        float cpuUsage = juce::Random::getSystemRandom().nextFloat() * 0.6f;
        updateCPUUsage(cpuUsage);
        
        // Actualizar RAM usage
        auto memStats = juce::SystemStats::getMemoryUsageDetails();
        float ramUsage = (float)memStats.usedRAM / (float)memStats.totalRAM;
        updateRAMUsage(ramUsage);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBar)
};

} // namespace GUI
} // namespace OmegaStudio
